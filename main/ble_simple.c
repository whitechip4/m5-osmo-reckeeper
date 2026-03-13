/* SPDX-License-Identifier: MIT */
/*
 * M5StickC Plus2 DJI Osmo360 BLE Remote
 * Development Step 2: BLE Connection (Simplified)
 *
 * Based on reference/OsmoDemo/ble/ble.c
 * Simplified for basic BLE scanning and connection to DJI Osmo360
 */

#include <string.h>
#include "ble_simple.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_timer.h"

static const char *TAG = "BLE";

/* BLE State Machine Variables */
/* BLE 状態マシン変数 */
static ble_state_t s_current_state = BLE_STATE_IDLE;
static void (*s_state_callback)(ble_state_t) = NULL;

/* Connection Information */
/* 接続情報 */
static ble_connection_t s_connection = {
    .is_connected = false,
    .conn_id = 0,
    .gattc_if = ESP_GATT_IF_NONE,
    .notify_handle = 0,
    .write_handle = 0,
    .remote_bda = {0}
};

/* Scanning Variables */
/* スキャン変数 */
static bool s_is_connecting = false;
static esp_bd_addr_t s_target_addr = {0};
static int8_t s_best_rssi = -128;
#define MIN_RSSI_THRESHOLD -80

/* Service UUIDs for DJI Osmo360 */
/* DJI Osmo360のサービスUUID */
#define REMOTE_TARGET_SERVICE_UUID   0xFFF0
#define REMOTE_NOTIFY_CHAR_UUID      0xFFF4
#define REMOTE_WRITE_CHAR_UUID       0xFFF5

static esp_bt_uuid_t s_filter_notify_char_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid.uuid16 = REMOTE_NOTIFY_CHAR_UUID,
};

static esp_bt_uuid_t s_filter_write_char_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid.uuid16 = REMOTE_WRITE_CHAR_UUID,
};

/* Scan parameters */
/* スキャンパラメータ */
static esp_ble_scan_params_t s_ble_scan_params = {
    .scan_type          = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval      = 0x50,
    .scan_window        = 0x30,
    .scan_duplicate     = BLE_SCAN_DUPLICATE_DISABLE
};

/* Callback function declarations */
/* コールバック関数宣言 */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void gattc_event_handler(esp_gattc_cb_event_t event,
                                esp_gatt_if_t gattc_if,
                                esp_ble_gattc_cb_param_t *param);

/* State management helper */
/* 状態管理ヘルパー関数 */
static void set_state(ble_state_t new_state) {
    if (s_current_state != new_state) {
        ESP_LOGI(TAG, "State change: %d -> %d", s_current_state, new_state);
        s_current_state = new_state;
        if (s_state_callback) {
            s_state_callback(new_state);
        }
    }
}

/* DJI device detection from advertising data */
/* 広告データからのDJIデバイス検出 */
static bool is_dji_camera_adv(esp_ble_gap_cb_param_t *scan_result) {
    const uint8_t *ble_adv = scan_result->scan_rst.ble_adv;
    const uint8_t adv_len = scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len;

    for (int i = 0; i < adv_len; ) {
        const uint8_t len = ble_adv[i];

        if (len == 0 || (i + len + 1) > adv_len) break;

        const uint8_t type = ble_adv[i+1];
        const uint8_t *data = &ble_adv[i+2];
        const uint8_t data_len = len - 1;

        /* Check for DJI manufacturer specific data */
        /* DJIメーカー固有データのチェック */
        if (type == ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE) {
            if (data_len >= 5) {
                /* DJI signature: 0xAA, 0x08, ..., 0xFA */
                if (data[0] == 0xAA && data[1] == 0x08 && data[4] == 0xFA) {
                    return true;
                }
            }
        }
        i += (len + 1);
    }
    return false;
}

/* Timer callback to stop scanning */
/* スキャン停止タイマーコールバック */
static TimerHandle_t s_scan_timer;

static void scan_stop_timer_callback(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "Scan timeout, stopping...");
    esp_ble_gap_stop_scanning();
}

/* -------------------------
 *  Public API Implementation
 *  パブリックAPI実装
 * ------------------------- */

esp_err_t ble_init(void) {
    ESP_LOGI(TAG, "Initializing BLE stack...");

    /* Initialize NVS */
    /* NVS初期化 */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Release classic Bluetooth memory */
    /* クラシック Bluetooth メモリ解放 */
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    /* Initialize Bluetooth controller */
    /* Bluetooth コントローラ初期化 */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "Controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Enable BLE */
    /* BLE有効化 */
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "Controller enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Initialize Bluedroid stack */
    /* Bluedroid スタック初期化 */
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Enable Bluedroid */
    /* Bluedroid有効化 */
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Register callbacks */
    /* コールバック登録 */
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "GAP register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gattc_register_callback(gattc_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "GATTC register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Register GATTC application */
    /* GATTC アプリケーション登録 */
    ret = esp_ble_gattc_app_register(0);
    if (ret) {
        ESP_LOGE(TAG, "GATTC app register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Set local MTU */
    /* ローカルMTU設定 */
    esp_ble_gatt_set_local_mtu(500);

    ESP_LOGI(TAG, "BLE stack initialized successfully");
    return ESP_OK;
}

esp_err_t ble_start_scanning_and_connect(void) {
    if (s_current_state != BLE_STATE_IDLE) {
        ESP_LOGW(TAG, "Cannot start scan: not in IDLE state (current=%d)", s_current_state);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Starting BLE scan for DJI Osmo360...");

    /* Reset scan variables */
    /* スキャン変数リセット */
    memset(s_target_addr, 0, sizeof(esp_bd_addr_t));
    s_best_rssi = -128;

    /* Set scan parameters */
    /* スキャンパラメータ設定 */
    esp_err_t ret = esp_ble_gap_set_scan_params(&s_ble_scan_params);
    if (ret) {
        ESP_LOGE(TAG, "Set scan params failed: %s", esp_err_to_name(ret));
        set_state(BLE_STATE_FAILED);
        return ret;
    }

    set_state(BLE_STATE_SCANNING);
    return ESP_OK;
}

esp_err_t ble_disconnect(void) {
    if (s_connection.is_connected) {
        ESP_LOGI(TAG, "Disconnecting...");
        esp_ble_gattc_close(s_connection.gattc_if, s_connection.conn_id);
    }
    return ESP_OK;
}

ble_state_t ble_get_state(void) {
    return s_current_state;
}

void ble_set_state_callback(void (*callback)(ble_state_t)) {
    s_state_callback = callback;
}

const ble_connection_t* ble_get_connection(void) {
    if (s_connection.is_connected) {
        return &s_connection;
    }
    return NULL;
}

/* -------------------------
 *  GAP Event Handler
 *  GAP イベントハンドラ
 * ------------------------- */

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "Scan params set complete, starting scan...");
        /* Start scanning with 4 second timeout */
        /* 4秒タイムアウトでスキャン開始 */
        esp_ble_gap_start_scanning(4);

        /* Create timer to stop scan after timeout */
        /* タイムアウト後にスキャン停止するタイマー作成 */
        s_scan_timer = xTimerCreate("scan_timer", pdMS_TO_TICKS(4000), pdFALSE,
                                    (void *)0, scan_stop_timer_callback);
        if (s_scan_timer != NULL) {
            xTimerStart(s_scan_timer, 0);
        }
        break;

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        ESP_LOGI(TAG, "Scan stopped");

        /* Check if we found a suitable device */
        /* 適切なデバイスが見つかったか確認 */
        if (s_best_rssi > -128) {
            ESP_LOGI(TAG, "Found DJI device with RSSI %d, connecting...", s_best_rssi);
            set_state(BLE_STATE_CONNECTING);

            /* Initiate connection */
            /* 接続開始 */
            if (!s_is_connecting) {
                s_is_connecting = true;
                esp_ble_gattc_open(s_connection.gattc_if,
                                   s_target_addr,
                                   BLE_ADDR_TYPE_PUBLIC,
                                   true);
            }
        } else {
            ESP_LOGW(TAG, "No DJI device found");
            set_state(BLE_STATE_FAILED);
        }
        break;

    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *r = param;

        if (r->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            /* Check if this is a DJI camera */
            /* DJIカメラかどうかチェック */
            if (!is_dji_camera_adv(r)) {
                break;
            }

            /* Get device name from advertising data */
            /* 広告データからデバイス名取得 */
            uint8_t *adv_name = NULL;
            uint8_t adv_name_len = 0;
            adv_name = esp_ble_resolve_adv_data_by_type(
                                r->scan_rst.ble_adv,
                                r->scan_rst.adv_data_len + r->scan_rst.scan_rsp_len,
                                ESP_BLE_AD_TYPE_NAME_CMPL,
                                &adv_name_len);

            /* Prepare device name for logging */
            /* ログ用にデバイス名準備 */
            const char *device_name = "Unknown";
            char name_buf[64] = {0};
            if (adv_name && adv_name_len > 0) {
                size_t copy_len = adv_name_len < sizeof(name_buf) - 1 ? adv_name_len : sizeof(name_buf) - 1;
                memcpy(name_buf, adv_name, copy_len);
                name_buf[copy_len] = '\0';
                device_name = name_buf;
            }

            ESP_LOGI(TAG, "Found DJI device: %s, RSSI: %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                     device_name,
                     r->scan_rst.rssi,
                     r->scan_rst.bda[0], r->scan_rst.bda[1], r->scan_rst.bda[2],
                     r->scan_rst.bda[3], r->scan_rst.bda[4], r->scan_rst.bda[5]);

            /* Track device with best signal strength */
            /* 最も信号強度の強いデバイスを記録 */
            if (r->scan_rst.rssi > s_best_rssi && r->scan_rst.rssi >= MIN_RSSI_THRESHOLD) {
                s_best_rssi = r->scan_rst.rssi;
                memcpy(s_target_addr, r->scan_rst.bda, sizeof(esp_bd_addr_t));
            }
        }
        break;
    }

    default:
        break;
    }
}

/* -------------------------
 *  GATTC Event Handler
 *  GATTC イベントハンドラ
 * ------------------------- */

static void gattc_event_handler(esp_gattc_cb_event_t event,
                                esp_gatt_if_t gattc_if,
                                esp_ble_gattc_cb_param_t *param) {
    switch (event) {
    case ESP_GATTC_REG_EVT:
        /* GATTC registration complete */
        /* GATTC登録完了 */
        if (param->reg.status == ESP_GATT_OK) {
            s_connection.gattc_if = gattc_if;
            ESP_LOGI(TAG, "GATTC registered successfully, gattc_if=%d", gattc_if);
        } else {
            ESP_LOGE(TAG, "GATTC registration failed, status=%d", param->reg.status);
        }
        break;

    case ESP_GATTC_CONNECT_EVT:
        /* Connection established */
        /* 接続確立 */
        s_connection.conn_id = param->connect.conn_id;
        s_connection.is_connected = true;
        memcpy(s_connection.remote_bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        s_is_connecting = false;

        ESP_LOGI(TAG, "Connected! conn_id=%d, MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                 s_connection.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1],
                 param->connect.remote_bda[2], param->connect.remote_bda[3],
                 param->connect.remote_bda[4], param->connect.remote_bda[5]);

        /* Request MTU exchange */
        /* MTU交換リクエスト */
        esp_ble_gattc_send_mtu_req(gattc_if, param->connect.conn_id);
        break;

    case ESP_GATTC_OPEN_EVT:
        /* Connection open event */
        /* 接続オープンイベント */
        if (param->open.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "Connection open failed, status=%d", param->open.status);
            s_is_connecting = false;
            set_state(BLE_STATE_FAILED);
        } else {
            ESP_LOGI(TAG, "Connection open successful, MTU=%u", param->open.mtu);
        }
        break;

    case ESP_GATTC_CFG_MTU_EVT:
        /* MTU configuration complete */
        /* MTU設定完了 */
        if (param->cfg_mtu.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "MTU config failed, status=%d", param->cfg_mtu.status);
        } else {
            ESP_LOGI(TAG, "MTU configured: %d", param->cfg_mtu.mtu);

            /* Start service discovery */
            /* サービスディスカバリー開始 */
            esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, NULL);
        }
        break;

    case ESP_GATTC_SEARCH_RES_EVT:
        /* Service search result */
        /* サービス検索結果 */
        if ((param->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16) &&
            (param->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_TARGET_SERVICE_UUID)) {
            ESP_LOGI(TAG, "Found target service 0x%04X, handles: %d-%d",
                     REMOTE_TARGET_SERVICE_UUID,
                     param->search_res.start_handle,
                     param->search_res.end_handle);
        }
        break;

    case ESP_GATTC_SEARCH_CMPL_EVT:
        /* Service search complete */
        /* サービス検索完了 */
        if (param->search_cmpl.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "Service search failed, status=%d", param->search_cmpl.status);
            set_state(BLE_STATE_FAILED);
            break;
        }

        ESP_LOGI(TAG, "Service search complete, discovering characteristics...");

        /* Find notify and write characteristics */
        /* 通知・書き込みキャラクタリスティック検索 */

        /* Find notify characteristic */
        /* 通知キャラクタリスティック検索 */
        uint16_t count = 1;
        esp_gattc_char_elem_t char_result;
        esp_ble_gattc_get_char_by_uuid(gattc_if,
                                       s_connection.conn_id,
                                       1, 0xFFFF,
                                       s_filter_notify_char_uuid,
                                       &char_result,
                                       &count);
        if (count > 0) {
            s_connection.notify_handle = char_result.char_handle;
            ESP_LOGI(TAG, "Found notify characteristic, handle=0x%04X", s_connection.notify_handle);
        }

        /* Find write characteristic */
        /* 書き込みキャラクタリスティック検索 */
        count = 1;
        esp_ble_gattc_get_char_by_uuid(gattc_if,
                                       s_connection.conn_id,
                                       1, 0xFFFF,
                                       s_filter_write_char_uuid,
                                       &char_result,
                                       &count);
        if (count > 0) {
            s_connection.write_handle = char_result.char_handle;
            ESP_LOGI(TAG, "Found write characteristic, handle=0x%04X", s_connection.write_handle);
        }

        /* Connection complete! */
        /* 接続完了！ */
        set_state(BLE_STATE_CONNECTED);
        break;

    case ESP_GATTC_DISCONNECT_EVT:
        /* Disconnection event */
        /* 切断イベント */
        s_connection.is_connected = false;
        s_is_connecting = false;
        ESP_LOGI(TAG, "Disconnected, reason=0x%x", param->disconnect.reason);
        set_state(BLE_STATE_IDLE);
        break;

    default:
        break;
    }
}
