/* SPDX-License-Identifier: MIT */
/*
 * System Initialization Implementation
 * Handles all system initialization sequences
 */

#include "handlers/system_init.h"
#include "ui/ui.h"
#include "ui/ui_layout.h"
#include "ble_simple.h"
#include "dji_protocol.h"
#include "storage.h"
#include "gps_module.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration for M5Unified C interface */
struct M5UnifiedImpl;
extern struct M5UnifiedImpl M5;

void M5_begin(void);
void M5_display_fillScreen(int color);
void M5_display_setTextColor(int textcolor, int textbgcolor);
void M5Display_setTextDatum(int datum);
void M5_display_setTextSize(float size);
void M5_display_drawString(const char *string, int x, int y);
int M5_display_width(void);
int M5_display_height(void);

static const char *TAG = "sys_init";

/* Color definitions from M5GFX (RGB888 format) */
/* M5GFXからの色定義（RGB888形式） */
#define TFT_BLACK   0x000000
#define TFT_RED     0xFF0000
#define TFT_WHITE   0xFFFFFF
#define TFT_GREEN   0x00FF00
#define TFT_YELLOW  0xFFFF00
#define TFT_CYAN    0x00FFFF

/* Text datum constants */
/* テキスト基準点定数 */
#define top_center 1

/* LCD error display helper */
static void show_error(const char *text) {
    M5_display_fillScreen(TFT_BLACK);
    M5_display_setTextColor(TFT_RED, TFT_BLACK);
    M5Display_setTextDatum(top_center);
    M5_display_setTextSize(2);
    int x = M5_display_width() / 2;
    int y = M5_display_height() / 2 - 10;
    M5_display_drawString(text, x, y);
}

/* ========== Callbacks ========== */

/* BLE state change callback */
static void ble_state_callback(ble_state_t new_state) {
    if (new_state < BLE_STATE_IDLE || new_state > BLE_STATE_FAILED) {
        ESP_LOGW(TAG, "Invalid BLE state: %d", new_state);
        return;
    }

    const char *state_text = (new_state == BLE_STATE_IDLE) ? "IDLE" :
                             (new_state == BLE_STATE_SCANNING) ? "SCANNING" :
                             (new_state == BLE_STATE_CONNECTING) ? "CONNECTING" :
                             (new_state == BLE_STATE_CONNECTED) ? "CONNECTED" : "ERROR";
    ESP_LOGI(TAG, "BLE state changed: %s", state_text);

    /* Only update LCD for BLE states when DJI is idle/failed */
    dji_state_t dji_state = dji_get_state();
    if (dji_state == DJI_STATE_IDLE || dji_state == DJI_STATE_FAILED) {
        if (new_state == BLE_STATE_IDLE) {
            ui_show_idle_state();
        } else {
            uint32_t color = (new_state == BLE_STATE_CONNECTED) ? TFT_GREEN :
                            (new_state == BLE_STATE_FAILED) ? TFT_RED : TFT_YELLOW;
            M5_display_fillScreen(TFT_BLACK);
            M5_display_setTextColor(color, TFT_BLACK);
            M5Display_setTextDatum(top_center);
            M5_display_setTextSize(2);
            int x = M5_display_width() / 2;
            int y = M5_display_height() / 2 - 10;
            M5_display_drawString(state_text, x, y);
        }
    }

    /* Auto-start pairing when BLE connected */
    if (new_state == BLE_STATE_CONNECTED) {
        ESP_LOGI(TAG, "BLE connected, starting DJI pairing...");
        bool is_first_pairing = !storage_is_paired();
        dji_start_pairing(is_first_pairing);
    }

    ui_update_battery_indicator();
}

/* DJI state change callback */
static void dji_state_callback(dji_state_t new_state) {
    if (new_state < DJI_STATE_IDLE || new_state > DJI_STATE_FAILED) {
        ESP_LOGW(TAG, "Invalid DJI state: %d", new_state);
        return;
    }

    const char *state_text = (new_state == DJI_STATE_IDLE) ? "IDLE" :
                             (new_state == DJI_STATE_PAIRING) ? "PAIRING" :
                             (new_state == DJI_STATE_PAIRED) ? "PAIRED" :
                             (new_state == DJI_STATE_RECORDING) ? "RECORDING" :
                             (new_state == DJI_STATE_RESTARTING) ? "RESTARTING" : "FAILED";
    ESP_LOGI(TAG, "DJI state changed: %s", state_text);

    /* Get current recording time and device ID */
    uint16_t rec_time = dji_get_recording_time();
    uint32_t device_id = dji_get_device_id();
    bool rec_keep_enabled = dji_is_rec_keep_mode_enabled();

    /* Display based on state */
    switch (new_state) {
        case DJI_STATE_IDLE:
            ui_show_idle_state();
            break;

        case DJI_STATE_PAIRING:
            ui_show_pairing_state(device_id);
            break;

        case DJI_STATE_PAIRED:
            ui_show_paired_state();
            break;

        case DJI_STATE_RECORDING:
            ui_show_recording_state(rec_time);
            break;

        case DJI_STATE_RESTARTING:
            ui_show_restarting_state();
            break;

        case DJI_STATE_FAILED:
            ui_show_failed_state();
            break;

        default:
            break;
    }

    /* Update Rec Keep indicator */
    ui_update_rec_keep_indicator(rec_keep_enabled);

    /* Update battery indicator */
    ui_update_battery_indicator();
}

/* Rec Keep mode change callback */
static void rec_keep_mode_callback(bool enabled) {
    ESP_LOGI(TAG, "Rec Keep mode changed: %s", enabled ? "ON" : "OFF");
    ui_update_rec_keep_indicator(enabled);
}

/* Register all callbacks */
static void register_callbacks(void) {
    ble_set_state_callback(ble_state_callback);
    dji_set_state_callback(dji_state_callback);

    extern void dji_set_rec_keep_callback(void (*callback)(bool));
    dji_set_rec_keep_callback(rec_keep_mode_callback);

    ble_set_notify_callback(dji_handle_notification);
}

/* ========== Public Interface ========== */

bool system_initialize(void) {
    ESP_LOGI(TAG, "Osmo360 BLE Remote Starting...");

    /* Initialize M5StickC Plus2 */
    M5_begin();
    ESP_LOGI(TAG, "M5Unified initialized");
    M5_display_fillScreen(TFT_BLACK);

    /* Initialize storage */
    esp_err_t ret = storage_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Storage initialization failed: %s", esp_err_to_name(ret));
        show_error("STG ERROR");
        return false;
    }
    ESP_LOGI(TAG, "Storage initialized");

    /* Initialize BLE stack */
    ret = ble_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BLE initialization failed: %s", esp_err_to_name(ret));
        show_error("BLE ERROR");
        return false;
    }
    ESP_LOGI(TAG, "BLE stack initialized");

    /* Initialize DJI protocol */
    ret = dji_protocol_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "DJI protocol initialization failed: %s", esp_err_to_name(ret));
        show_error("DJI ERROR");
        return false;
    }
    ESP_LOGI(TAG, "DJI protocol initialized");

    /* Register callbacks */
    register_callbacks();

    /* Initialize GPS module */
    ret = gps_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "GPS initialization failed: %s", esp_err_to_name(ret));
    }
    ESP_LOGI(TAG, "GPS module initialized");

    /* Display initial state */
    ui_show_idle_state();
    ui_update_battery_indicator();
    ui_update_gps_status();

    ESP_LOGI(TAG, "System ready. Press BtnA to scan for DJI Osmo360.");
    return true;
}

#ifdef __cplusplus
}
#endif
