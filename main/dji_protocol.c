/* SPDX-License-Identifier: MIT */
/*
 * DJI Protocol High-Level API
 * DJIプロトコル高レベルAPI（イベントループ方式）
 */

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "dji_protocol.h"
#include "storage.h"
#include "protocol/dji_protocol_parser.h"
#include "protocol/dji_protocol_data_structures.h"
#include "ble_simple.h"

static const char *TAG = "DJI";

/* State machine variables */
/* 状態マシン変数 */
static dji_state_t s_dji_state = DJI_STATE_IDLE;
static void (*s_state_callback)(dji_state_t) = NULL;
static uint16_t s_seq = 0;

/* Recording status tracking */
/* 録画状態追跡 */
static bool s_is_recording = false;

/* Pairing state variables */
/* ペアリング状態変数 */
static bool s_waiting_for_response = false;
static bool s_waiting_for_camera_request = false;
static uint16_t s_expected_seq = 0;

/* State management helper */
/* 状態管理ヘルパー */
static void set_dji_state(dji_state_t new_state) {
    if (s_dji_state != new_state) {
        ESP_LOGI(TAG, "State change: %d -> %d", s_dji_state, new_state);
        s_dji_state = new_state;
        if (s_state_callback) {
            s_state_callback(new_state);
        }
    }
}

/* Get next sequence number */
/* 次のシーケンス番号取得 */
static uint16_t get_next_seq(void) {
    return ++s_seq;
}

esp_err_t dji_protocol_init(void) {
    ESP_LOGI(TAG, "Initializing DJI protocol...");
    s_dji_state = DJI_STATE_IDLE;
    s_seq = 0;
    s_is_recording = false;
    s_waiting_for_response = false;
    s_waiting_for_camera_request = false;
    s_expected_seq = 0;
    return ESP_OK;
}

esp_err_t dji_start_pairing(bool is_first_pairing) {
    const ble_connection_t *conn = ble_get_connection();
    if (conn == NULL || !conn->is_connected) {
        ESP_LOGE(TAG, "Not connected to BLE device");
        set_dji_state(DJI_STATE_FAILED);
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting pairing... (is_first_pairing=%d)", is_first_pairing);
    set_dji_state(DJI_STATE_PAIRING);

    uint16_t seq = get_next_seq();
    s_expected_seq = seq;  /* Save sequence number for response matching */
    esp_err_t ret;

    /* STEP1: Send connection request to camera */
    /* ステップ1：カメラに接続リクエスト送信 */
    connection_request_command_frame_t conn_req = {
        .device_id = 0x00000001,
        .mac_addr_len = 6,
        .fw_version = 0,
        .conidx = 0,
        .verify_mode = is_first_pairing ? 1 : 0,  /* 1=first pairing, 0=reconnection */
        .verify_data = 0,
        .reserved = {0}
    };

    ESP_LOGI(TAG, "STEP1: Sending connection request with verify_mode=%u", conn_req.verify_mode);

    /* Copy MAC address from BLE connection */
    /* BLE接続からMACアドレスコピー */
    memcpy(conn_req.mac_addr, conn->remote_bda, 6);

    /* Create and send frame */
    /* フレーム作成と送信 */
    size_t frame_length;
    uint8_t *frame = protocol_create_frame(0x00, 0x19, 0x01, &conn_req, seq, &frame_length);
    if (frame == NULL) {
        ESP_LOGE(TAG, "Failed to create connection request frame");
        set_dji_state(DJI_STATE_FAILED);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "STEP1: Sending connection request (SEQ=%u)...", seq);
    ret = ble_write(frame, frame_length);
    free(frame);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send connection request");
        set_dji_state(DJI_STATE_FAILED);
        return ret;
    }

    /* Set flag to wait for response */
    /* 応答待ちフラグ設定 */
    s_waiting_for_response = true;

    ESP_LOGI(TAG, "Waiting for camera response (will process in notification handler)...");
    return ESP_OK;
}

esp_err_t dji_subscribe_status(void) {
    const ble_connection_t *conn = ble_get_connection();
    if (conn == NULL || !conn->is_connected) {
        ESP_LOGE(TAG, "Not connected to BLE device");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Subscribing to camera status...");

    /* Build status subscription request */
    /* 状態購読リクエスト構築 */
    camera_status_subscription_command_frame_t sub_req = {
        .push_mode = 3,  /* Periodic + status change / 定期 + 状態変化時 */
        .push_freq = 20, /* 2Hz (20 * 0.1Hz) */
        .reserved = {0}
    };

    /* Create protocol frame */
    /* プロトコルフレーム作成 */
    size_t frame_length;
    uint8_t *frame = protocol_create_frame(0x1D, 0x05, 0x01, &sub_req,
                                           get_next_seq(), &frame_length);
    if (frame == NULL) {
        ESP_LOGE(TAG, "Failed to create status subscription frame");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sending status subscription (%zu bytes)", frame_length);

    /* Send via BLE write */
    /* BLE書き込みで送信 */
    esp_err_t ret = ble_write(frame, frame_length);
    free(frame);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send status subscription: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

void dji_handle_notification(const uint8_t *data, uint16_t length) {
    /* Quick filter: Check if data starts with 0xAA (DJI protocol SOF) */
    /* クイックフィルタ：0xAA（DJIプロトコルSOF）で始まるか確認 */
    if (length < 2 || data[0] != 0xAA) {
        /* Not a DJI protocol frame, discard silently */
        /* DJIプロトコルフレームではない、サイレントに破棄 */
        return;
    }

    /* Parse frame */
    /* フレーム解析 */
    protocol_frame_t frame;
    int ret = protocol_parse_notification(data, length, &frame);
    if (ret != 0) {
        /* Parse error - discard silently to reduce log spam */
        /* 解析エラー - ログ出力を抑制して破棄 */
        return;
    }

    /* Check DATA segment exists */
    /* データセグメント存在確認 */
    if (frame.data == NULL || frame.data_length < 2) {
        return;
    }

    uint8_t cmd_set = frame.data[0];
    uint8_t cmd_id = frame.data[1];

    /* Handle connection pairing (0x00, 0x19) */
    /* 接続ペアリング処理 (0x00, 0x19) */
    if (cmd_set == 0x00 && cmd_id == 0x19) {
        if (frame.cmd_type & 0x20) {
            /* Response frame */
            /* レスポンスフレーム */
            if (frame.data_length >= sizeof(connection_request_response_frame_t)) {
                connection_request_response_frame_t *resp =
                    (connection_request_response_frame_t *)&frame.data[2];

                ESP_LOGI(TAG, "Received response: ret_code=%u, SEQ=%u", resp->ret_code, frame.seq);

                if (s_waiting_for_response && frame.seq == s_expected_seq) {
                    s_waiting_for_response = false;

                    if (resp->ret_code == 0) {
                        ESP_LOGI(TAG, "Response OK, waiting for camera connection request...");
                        s_waiting_for_camera_request = true;
                    } else {
                        ESP_LOGE(TAG, "Connection rejected: ret_code=%u", resp->ret_code);
                        set_dji_state(DJI_STATE_FAILED);
                        s_waiting_for_camera_request = false;
                    }
                }
            }
        } else {
            /* Command frame from camera */
            /* カメラからのコマンドフレーム */
            if (frame.data_length >= sizeof(connection_request_command_frame_t)) {
                connection_request_command_frame_t *cam_req =
                    (connection_request_command_frame_t *)&frame.data[2];

                ESP_LOGI(TAG, "Camera request: verify_mode=%u, verify_data=%u, SEQ=%u",
                         cam_req->verify_mode, cam_req->verify_data, frame.seq);

                if (s_waiting_for_camera_request) {
                    s_waiting_for_camera_request = false;

                    if (cam_req->verify_mode == 2) {
                        /* STEP4: Send connection response to camera */
                        /* ステップ4：カメラに接続レスポンス送信 */
                        if (cam_req->verify_data == 0) {
                            ESP_LOGI(TAG, "Camera approved connection, sending response...");

                            connection_request_response_frame_t conn_resp = {
                                .device_id = 0x00000001,
                                .ret_code = 0,
                                .reserved = {0}
                            };

                            /* Create and send response frame (use camera's SEQ) */
                            /* レスポンスフレーム作成と送信（カメラのSEQを使用） */
                            size_t resp_length;
                            uint8_t *resp_frame = protocol_create_frame(0x00, 0x19, 0x21, &conn_resp,
                                                               frame.seq, &resp_length);
                            if (resp_frame != NULL) {
                                esp_err_t write_ret = ble_write(resp_frame, resp_length);
                                free(resp_frame);

                                if (write_ret == ESP_OK) {
                                    ESP_LOGI(TAG, "Pairing successful!");
                                    set_dji_state(DJI_STATE_PAIRED);

                                    /* Save paired device MAC address */
                                    /* ペアリング済みデバイスのMACアドレスを保存 */
                                    const ble_connection_t *conn = ble_get_connection();
                                    if (conn != NULL) {
                                        storage_save_paired_device(conn->remote_bda);
                                    }

                                    /* Auto-subscribe to status */
                                    /* 状態購読を自動開始 */
                                    dji_subscribe_status();
                                } else {
                                    ESP_LOGE(TAG, "Failed to send response");
                                    set_dji_state(DJI_STATE_FAILED);
                                }
                            } else {
                                ESP_LOGE(TAG, "Failed to create response frame");
                                set_dji_state(DJI_STATE_FAILED);
                            }
                        } else {
                            ESP_LOGW(TAG, "Camera rejected connection");
                            set_dji_state(DJI_STATE_FAILED);
                        }
                    } else {
                        ESP_LOGE(TAG, "Unexpected verify_mode: %u", cam_req->verify_mode);
                        set_dji_state(DJI_STATE_FAILED);
                    }
                }
            }
        }
        return;  /* Connection frames handled, don't process further */
    }

    /* Handle status push (0x1D, 0x02) */
    /* 状態プッシュ処理 (0x1D, 0x02) */
    if (cmd_set == 0x1D && cmd_id == 0x02) {
        if (frame.data_length >= sizeof(camera_status_push_command_frame_t)) {
            camera_status_push_command_frame_t *status =
                (camera_status_push_command_frame_t *)&frame.data[2];

            /* Log status periodically (not every time to reduce spam) */
            /* 定期的に状態をログ（スパム抑制） */
            static uint32_t last_log_time = 0;
            uint32_t current_time = esp_timer_get_time() / 1000;
            if (current_time - last_log_time > 5 || status->camera_status != s_is_recording) {
                ESP_LOGI(TAG, "Status: camera_status=0x%02X, mode=0x%02X",
                         status->camera_status, status->camera_mode);
                last_log_time = current_time;
            }

            /* Update recording state */
            /* 録画状態更新 */
            bool was_recording = s_is_recording;
            s_is_recording = (status->camera_status == 0x03);

            if (s_is_recording && !was_recording) {
                ESP_LOGI(TAG, "Camera started recording");
                set_dji_state(DJI_STATE_RECORDING);
            } else if (!s_is_recording && was_recording) {
                ESP_LOGI(TAG, "Camera stopped recording");
                if (s_dji_state == DJI_STATE_RECORDING) {
                    set_dji_state(DJI_STATE_PAIRED);
                }
            }
        }
    }
}

dji_state_t dji_get_state(void) {
    return s_dji_state;
}

void dji_set_state_callback(void (*callback)(dji_state_t)) {
    s_state_callback = callback;
}

/* Internal helper: Send recording command */
/* 内部ヘルパー: 録画コマンド送信 */
static esp_err_t dji_send_record_command(bool start) {
    const ble_connection_t *conn = ble_get_connection();
    if (conn == NULL || !conn->is_connected) {
        ESP_LOGE(TAG, "Not connected to BLE device");
        return ESP_ERR_INVALID_STATE;
    }

    /* Check if paired */
    /* ペアリング確認 */
    if (s_dji_state != DJI_STATE_PAIRED && s_dji_state != DJI_STATE_RECORDING) {
        ESP_LOGE(TAG, "Not paired with camera (state=%d)", s_dji_state);
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Sending record command: %s", start ? "START" : "STOP");

    /* Build record control command */
    /* 録画制御コマンド構築 */
    record_control_command_frame_t record_cmd = {
        .device_id = 0x33FF0000,
        .record_ctrl = start ? 0x00 : 0x01,
        .reserved = {0}
    };

    /* Create protocol frame */
    /* プロトコルフレーム作成 */
    size_t frame_length;
    uint8_t *frame = protocol_create_frame(0x1D, 0x03, 0x01, &record_cmd,
                                           get_next_seq(), &frame_length);
    if (frame == NULL) {
        ESP_LOGE(TAG, "Failed to create record control frame");
        return ESP_FAIL;
    }

    /* Send via BLE */
    /* BLE経由で送信 */
    esp_err_t ret = ble_write(frame, frame_length);
    free(frame);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send record command: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Record command sent successfully");
    return ESP_OK;
}

/* Public API: Toggle recording */
/* パブリックAPI: 録画切り替え */
esp_err_t dji_toggle_recording(void) {
    /* Start recording if stopped, stop if recording */
    /* 録画停止中なら開始、録画中なら停止 */
    bool should_start = !s_is_recording;
    return dji_send_record_command(should_start);
}

void dji_reset_state(void) {
    ESP_LOGI(TAG, "Resetting DJI protocol state...");
    s_dji_state = DJI_STATE_IDLE;
    s_seq = 0;
    s_is_recording = false;
    s_waiting_for_response = false;
    s_waiting_for_camera_request = false;
    s_expected_seq = 0;
}
