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
#include "config.h"
#include "protocol/dji_protocol_parser.h"
#include "protocol/dji_protocol_data_structures.h"
#include "ble_simple.h"
#include "gps_module.h"

static const char *TAG = "DJI";

/* State machine variables */
/* 状態マシン変数 */
static dji_state_t s_dji_state = DJI_STATE_IDLE;
static void (*s_state_callback)(dji_state_t) = NULL;
static uint16_t s_seq = 0;

/* Recording status tracking */
/* 録画状態追跡 */
static bool s_is_recording = false;
static uint16_t s_recording_time = 0;      /* Recording time in seconds / 録画時間（秒） */
static int64_t s_recording_start_time = 0; /* Recording start time (microseconds) / 録画開始時刻 */
static bool s_use_internal_timer = false;  /* Use internal timer flag / 内部タイマー使用フラグ */
static uint32_t s_camera_device_id = 0;    /* Camera device ID / カメラデバイスID */

/* Camera battery tracking */
/* カメラバッテリー追跡 */
static uint8_t s_camera_battery_level = 0; /* 0-100%, 0 means unavailable / 0-100%, 0は利用不可 */

/* SD card tracking */
/* SDカード追跡 */
static uint32_t s_sd_remaining_mb = 0;      /* SD card remaining capacity in MB / SDカード残容量 (MB) */
static uint32_t s_sd_total_mb = 0;          /* SD card total capacity in MB (calculated) / SDカード総容量 (MB) */
static uint32_t s_sd_remaining_photos = 0;  /* Remaining photo count / 残り写真枚数 */
static uint32_t s_sd_remaining_time = 0;    /* Remaining recording time (seconds) / 残り録画時間 (秒) */

/* Pairing state variables */
/* ペアリング状態変数 */
static bool s_waiting_for_response = false;
static bool s_waiting_for_camera_request = false;
static uint16_t s_expected_seq = 0;

/* Rec Keep mode state */
/* Rec Keepモード状態 */
static bool s_rec_keep_enabled = false;
static bool s_local_stop_pending = false;
static esp_timer_handle_t s_rec_keep_timer = NULL;
static void (*s_rec_keep_callback)(bool) = NULL;

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

/* Forward declaration */
/* 前方宣言 */
static esp_err_t dji_send_record_command(bool start);

/* Rec Keep timer callback */
/* Rec Keepタイマーコールバック */
static void rec_keep_timer_callback(void* arg) {
    ESP_LOGI(TAG, "Rec Keep timer triggered, restarting recording...");
    if (!s_is_recording && s_rec_keep_enabled) {
        set_dji_state(DJI_STATE_RESTARTING);
        dji_send_record_command(true);
    }
    s_local_stop_pending = false;
}

esp_err_t dji_protocol_init(void) {
    ESP_LOGI(TAG, "Initializing DJI protocol...");
    s_dji_state = DJI_STATE_IDLE;
    s_seq = 0;
    s_is_recording = false;
    s_recording_start_time = 0;
    s_use_internal_timer = false;
    s_waiting_for_response = false;
    s_waiting_for_camera_request = false;
    s_expected_seq = 0;
    s_local_stop_pending = false;

    /* Restore Rec Keep mode from NVS */
    /* NVSからRec Keepモード復元 */
    bool is_found = false;
    bool saved_mode = false;
    if (storage_get_rec_keep_mode(&saved_mode, &is_found) == ESP_OK && is_found) {
        s_rec_keep_enabled = saved_mode;
        ESP_LOGI(TAG, "Restored Rec Keep mode: %s", s_rec_keep_enabled ? "ON" : "OFF");
    } else {
        ESP_LOGI(TAG, "No saved Rec Keep mode found, default: OFF");
    }

    /* Create Rec Keep timer (3 seconds one-shot) */
    /* Rec Keepタイマー作成（3秒ワンショット） */
    const esp_timer_create_args_t timer_args = {
        .callback = &rec_keep_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "rec_keep_timer"
    };

    esp_err_t ret = esp_timer_create(&timer_args, &s_rec_keep_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create Rec Keep timer: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Rec Keep timer created");
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

                /* Extract camera device ID */
                s_camera_device_id = resp->device_id;

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

            /* Extract recording time */
            s_recording_time = status->record_time;

            /* Extract camera battery level */
            /* カメラバッテリー残量を抽出 */
            s_camera_battery_level = status->camera_bat_percentage;

            /* Extract SD card information */
            /* SDカード情報抽出 */
            s_sd_remaining_mb = status->remain_capacity;
            s_sd_remaining_photos = status->remain_photo_num;
            s_sd_remaining_time = status->remain_time;

            /* Log status periodically (not every time to reduce spam) */
            /* 定期的に状態をログ（スパム抑制） */
            static uint32_t last_log_time = 0;
            uint32_t current_time = esp_timer_get_time() / 1000;
            if (current_time - last_log_time > 5 || status->camera_status != s_is_recording) {
                ESP_LOGI(TAG, "Status: camera_status=0x%02X, mode=0x%02X, time=%u",
                         status->camera_status, status->camera_mode, s_recording_time);
                last_log_time = current_time;
            }

            /* Update recording state */
            /* 録画状態更新 */
            bool was_recording = s_is_recording;
            s_is_recording = (status->camera_status == 0x03);

            if (s_is_recording && !was_recording) {
                ESP_LOGI(TAG, "Camera started recording");
                set_dji_state(DJI_STATE_RECORDING);

                /* Start internal timer for recording time */
                /* 内部タイマーを開始 */
                s_recording_start_time = esp_timer_get_time();

                /* Clear local stop flag and cancel Rec Keep timer */
                /* ローカル停止フラグクリア、Rec Keepタイマーキャンセル */
                s_local_stop_pending = false;
                if (s_rec_keep_timer != NULL) {
                    esp_timer_stop(s_rec_keep_timer);
                }

                /* Save Rec Keep mode state */
                /* Rec Keepモード状態保存 */
                storage_save_rec_keep_mode(s_rec_keep_enabled);

            } else if (!s_is_recording && was_recording) {
                ESP_LOGI(TAG, "Camera stopped recording");
                if (s_dji_state == DJI_STATE_RECORDING) {
                    set_dji_state(DJI_STATE_PAIRED);
                }

                /* Reset internal timer */
                /* 内部タイマーリセット */
                s_recording_start_time = 0;

                /* Handle Rec Keep mode */
                ESP_LOGI(TAG, "Camera stopped recording");
                if (s_dji_state == DJI_STATE_RECORDING) {
                    set_dji_state(DJI_STATE_PAIRED);
                }

                /* Handle Rec Keep mode */
                /* Rec Keepモード処理 */
                if (s_rec_keep_enabled && !s_local_stop_pending) {
                    /* External stop detected: start 3 second timer */
                    /* 外部停止検知: 3秒タイマー開始 */
                    ESP_LOGI(TAG, "External stop detected, starting Rec Keep timer...");
                    if (s_rec_keep_timer != NULL) {
                        esp_timer_start_once(s_rec_keep_timer, REC_KEEP_DELAY_MS * 1000); /* config.hの定数使用 */
                    }
                } else {
                    /* Local stop or Rec Keep disabled */
                    /* ローカル停止またはRec Keep無効 */
                    s_local_stop_pending = false;
                }

                /* Save Rec Keep mode state */
                /* Rec Keepモード状態保存 */
                storage_save_rec_keep_mode(s_rec_keep_enabled);
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

    /* Check if paired (allow PAIRED, RECORDING, and RESTARTING states) */
    /* ペアリング確認（PAIRED、RECORDING、RESTARTING状態を許可） */
    if (s_dji_state != DJI_STATE_PAIRED &&
        s_dji_state != DJI_STATE_RECORDING &&
        s_dji_state != DJI_STATE_RESTARTING) {
        ESP_LOGE(TAG, "Not paired with camera (state=%d)", s_dji_state);
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Sending record command: %s", start ? "START" : "STOP");

    /* Update Rec Keep mode state */
    /* Rec Keepモード状態更新 */
    if (start) {
        /* Starting recording: clear local stop flag, cancel timer */
        /* 録画開始: ローカル停止フラグクリア、タイマーキャンセル */
        s_local_stop_pending = false;
        if (s_rec_keep_timer != NULL) {
            esp_timer_stop(s_rec_keep_timer);
        }
    } else {
        /* Stopping recording: set local stop flag */
        /* 録画停止: ローカル停止フラグ設定 */
        s_local_stop_pending = true;
    }

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
    s_local_stop_pending = false;

    /* Cancel Rec Keep timer */
    /* Rec Keepタイマーキャンセル */
    if (s_rec_keep_timer != NULL) {
        esp_timer_stop(s_rec_keep_timer);
    }
}

bool dji_is_recording(void) {
    return s_is_recording;
}

esp_err_t dji_set_rec_keep_mode(bool enabled) {
    s_rec_keep_enabled = enabled;
    ESP_LOGI(TAG, "Rec Keep mode: %s", enabled ? "ON" : "OFF");

    /* Notify callback if registered */
    /* コールバック登録済みなら通知 */
    if (s_rec_keep_callback != NULL) {
        s_rec_keep_callback(enabled);
    }

    return ESP_OK;
}

bool dji_is_rec_keep_mode_enabled(void) {
    return s_rec_keep_enabled;
}

void dji_set_rec_keep_callback(void (*callback)(bool)) {
    s_rec_keep_callback = callback;
}

uint16_t dji_get_recording_time(void) {
    /* Priority: Use BLE notification record_time if available (includes pre-recording) */
    /* 優先度: BLE通知のrecord_timeを使用（プリレコーディング時間を含む） */
    if (s_recording_time > 0) {
        return s_recording_time;
    }

    /* Fallback: Calculate from internal timer if BLE data not yet received */
    /* フォールバック: BLEデータ未受信時は内部タイマーから計算 */
    if (s_is_recording && s_recording_start_time > 0) {
        int64_t current_time = esp_timer_get_time();
        int64_t elapsed_us = current_time - s_recording_start_time;
        uint16_t elapsed_sec = (uint16_t)(elapsed_us / 1000000);  /* Convert to seconds */
        return elapsed_sec;
    }

    return 0;  /* Not recording / 録画していない */
}

uint32_t dji_get_device_id(void) {
    return s_camera_device_id;
}

/* Get camera battery level */
/* カメラバッテリー残量を取得 */
uint8_t dji_get_camera_battery_level(void) {
    return s_camera_battery_level;
}

/* Get SD card remaining capacity in MB */
/* SDカード残り容量を取得 (MB) */
uint32_t dji_get_sd_remaining_mb(void) {
    return s_sd_remaining_mb;
}

/* Get SD card remaining capacity percentage (0-100) */
/* SDカード残り容量パーセンテージを取得 (0-100) */
uint8_t dji_get_sd_capacity_percentage(void) {
    /* Calculate percentage if total capacity is known */
    /* 総容量がわかる場合はパーセンテージ計算 */
    if (s_sd_total_mb > 0) {
        return (uint8_t)((s_sd_remaining_mb * 100) / s_sd_total_mb);
    }
    return 0;
}

/* Get estimated remaining photos */
/* 撮影可能残り枚数を取得 */
uint32_t dji_get_sd_remaining_photos(void) {
    return s_sd_remaining_photos;
}

/* Get estimated remaining recording time (seconds) */
/* 残り録画時間を取得 (秒) */
uint32_t dji_get_sd_remaining_time(void) {
    return s_sd_remaining_time;
}

/**
 * @brief Send GPS module data to camera
 *        GPSモジュールデータをカメラに送信
 *
 * Converts gps_data_t to DJI format and sends via BLE.
 * gps_data_tをDJI形式に変換してBLE送信。
 *
 * Note: GPS data should be sent continuously when paired (not just during recording).
 *       The camera will use GPS data during recording.
 * GPSデータはペアリング済みなら常に送信する必要がある（録画中のみではない）。
 * カメラ側で録画中のGPSデータを使用する。
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t dji_send_gps_module_data(void) {
    const ble_connection_t *conn = ble_get_connection();
    if (conn == NULL || !conn->is_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Check if paired (send GPS data continuously when paired) */
    /* ペアリング済みか確認（ペアリング済みならGPSデータを常時送信） */
    if (s_dji_state != DJI_STATE_PAIRED &&
        s_dji_state != DJI_STATE_RECORDING) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Get GPS data from module */
    /* GPSモジュールからデータ取得 */
    gps_data_t gps_data;
    esp_err_t ret = gps_get_data(&gps_data);
    if (ret != ESP_OK) {
        return ret;
    }

    /* Validate GPS data */
    /* GPSデータ検証 */
    if (!gps_is_found()) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Convert to DJI format (following reference implementation) */
    /* DJI形式に変換（Reference実装に準拠） */
    gps_data_push_command_frame_t gps_frame = {
        .year_month_day = (gps_data.year + 2000) * 10000 +
                          gps_data.month * 100 + gps_data.day,
        .hour_minute_second = (gps_data.hour + 8) * 10000 +  /* UTC+8 */
                              gps_data.minute * 100 + (int32_t)gps_data.second,
        .gps_longitude = (int32_t)(gps_data.longitude * 1e7),
        .gps_latitude = (int32_t)(gps_data.latitude * 1e7),
        .height = (int32_t)(gps_data.altitude * 1000),  /* meters to mm */
        .speed_to_north = gps_data.velocity_north * 100.0f,  /* m/s to cm/s */
        .speed_to_east = gps_data.velocity_east * 100.0f,
        .speed_to_wnward = gps_data.velocity_descend * 100.0f,
        .vertical_accuracy = 1000,    /* Default 1000mm */
        .horizontal_accuracy = 1000,  /* Default 1000mm */
        .speed_accuracy = 10,         /* Default 10cm/s */
        .satellite_number = gps_data.num_satellites
    };

    /* Create and send protocol frame (CmdSet 0x00, CmdID 0x17) */
    /* プロトコルフレーム作成と送信 (CmdSet 0x00, CmdID 0x17) */
    size_t frame_length;
    uint8_t *frame = protocol_create_frame(0x00, 0x17, 0x01, &gps_frame,
                                           get_next_seq(), &frame_length);
    if (frame == NULL) {
        ESP_LOGE(TAG, "Failed to create GPS data frame");
        return ESP_FAIL;
    }

    ret = ble_write(frame, frame_length);
    free(frame);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send GPS data: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}
