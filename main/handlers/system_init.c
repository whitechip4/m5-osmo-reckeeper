/* SPDX-License-Identifier: MIT */
/*
 * System Initialization Implementation
 * Handles all system initialization sequences
 */

#include "handlers/system_init.h"
#include "ui/ui_state.h"
#include "ui/ui_renderer.h"
#include "ui/ui_layout.h"
#include "ble_simple.h"
#include "dji_protocol.h"
#include "storage.h"
#include "gps_module.h"
#include "m5_wrapper.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * @brief Display error message on LCD
 * @param text Error message to display
 * @note Shows error message in red text on black background
 */
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

/**
 * @brief BLE state change callback
 * @param new_state New BLE state
 * @note Updates UI state when BLE state changes. Auto-starts DJI pairing when connected.
 */
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

    /* Update UI state for BLE states when DJI is idle/failed */
    dji_state_t dji_state = dji_get_state();
    if (dji_state == DJI_STATE_IDLE || dji_state == DJI_STATE_FAILED) {
        if (new_state == BLE_STATE_IDLE) {
            ui_state_set_main(DJI_STATE_IDLE, 0, 0);
        }
        /* For SCANNING/CONNECTING/CONNECTED/FAILED, DJI state takes precedence in display */
    }

    /* Auto-start pairing when BLE connected */
    if (new_state == BLE_STATE_CONNECTED) {
        ESP_LOGI(TAG, "BLE connected, starting DJI pairing...");
        bool is_first_pairing = !storage_is_paired();
        dji_start_pairing(is_first_pairing);
    }

    /* Update battery state */
    int device_battery = M5_Power_getBatteryLevel();
    ui_state_set_battery(device_battery, 0);
}

/**
 * @brief DJI state change callback
 * @param new_state New DJI protocol state
 * @note Updates UI state when DJI protocol state changes
 */
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

    /* Update main display state */
    ui_state_set_main(new_state, device_id, rec_time);

    /* Update Rec Keep state */
    bool rec_keep_enabled = dji_is_rec_keep_mode_enabled();
    ui_state_set_rec_keep(rec_keep_enabled);

    /* Update battery state */
    int device_battery = M5_Power_getBatteryLevel();
    int camera_battery = dji_get_camera_battery_level();
    ui_state_set_battery(device_battery, camera_battery);
}

/**
 * @brief Rec Keep mode change callback
 * @param enabled true if Rec Keep mode enabled, false otherwise
 * @note Updates UI state when Rec Keep mode is toggled
 */
static void rec_keep_mode_callback(bool enabled) {
    ESP_LOGI(TAG, "Rec Keep mode changed: %s", enabled ? "ON" : "OFF");
    ui_state_set_rec_keep(enabled);
}

/**
 * @brief Register all system callbacks
 * @note Registers callbacks for BLE state, DJI state, Rec Keep mode, and BLE notifications
 */
static void register_callbacks(void) {
    ble_set_state_callback(ble_state_callback);
    dji_set_state_callback(dji_state_callback);

    extern void dji_set_rec_keep_callback(void (*callback)(bool));
    dji_set_rec_keep_callback(rec_keep_mode_callback);

    ble_set_notify_callback(dji_handle_notification);
}

/* ========== Public Interface ========== */

/**
 * @brief Initialize all system components
 * @return true on success, false on fatal error
 * @details Initializes M5Unified, UI, storage, BLE, DJI protocol, and GPS module.
 *          Registers callbacks and performs initial UI render.
 */
bool system_initialize(void) {
    ESP_LOGI(TAG, "Osmo360 BLE Remote Starting...");

    /* Initialize M5StickC Plus2 */
    M5_begin();
    ESP_LOGI(TAG, "M5Unified initialized");
    M5_display_fillScreen(TFT_BLACK);

    /* Initialize UI state management */
    ui_state_init();

    /* Initialize UI renderer (creates double buffer) */
    if (!ui_renderer_init()) {
        ESP_LOGW(TAG, "UI renderer initialization failed, falling back to direct drawing");
    }

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

    /* Set initial UI state */
    ui_state_set_main(DJI_STATE_IDLE, 0, 0);
    int device_battery = M5_Power_getBatteryLevel();
    ui_state_set_battery(device_battery, 0);

    /* Initial render */
    ui_renderer_render();

    ESP_LOGI(TAG, "System ready. Press BtnA to scan for DJI Osmo360.");
    return true;
}

#ifdef __cplusplus
}
#endif
