/* SPDX-License-Identifier: MIT */
/*
 * Button Handlers Implementation
 * Handles all button press events for the application
 */

#include "handlers/button_handlers.h"
#include "ui/ui.h"
#include "ui/ui_layout.h"
#include "ble_simple.h"
#include "dji_protocol.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration for M5Unified C interface */
struct M5UnifiedImpl;
extern struct M5UnifiedImpl M5;

void M5_update(void);
void M5_display_fillScreen(int color);
int M5_BtnPWR_isPressed(void);
void M5_Power_restart(void);
void M5_Power_off(void);

#ifdef __cplusplus
}
#endif

/* Color definitions from M5GFX (RGB888 format) */
/* M5GFXからの色定義（RGB888形式） */
#define TFT_BLACK 0x000000
#define TFT_WHITE 0xFFFFFF
#define TFT_RED   0xFF0000
#define TFT_YELLOW 0xFFFF00

/* Text datum constants */
/* テキスト基準点定数 */
#define top_center 1

static const char *TAG = "button_handlers";

/* ========== Internal Helper Functions ========== */

/* Try to initiate BLE connection
 * Returns: true if connection initiated successfully */
static bool btn_try_connect(void) {
    ESP_LOGI(TAG, "Button pressed, connecting...");
    esp_err_t ret = ble_connect_or_scan();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

/* Toggle recording state
 * Returns: true if toggle initiated successfully */
static bool btn_toggle_recording(void) {
    ESP_LOGI(TAG, "Button pressed, toggling recording...");
    esp_err_t ret = dji_toggle_recording();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to toggle recording: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

/* Disconnect BLE
 * Returns: true if disconnection initiated successfully */
static bool btn_disconnect(void) {
    ESP_LOGI(TAG, "Button pressed, disconnecting...");
    ble_disconnect();
    return true;
}

/* PWR button countdown loop
 * Blocks until button released or 3 seconds elapse
 * Returns: BTN_ACTION_RESET or BTN_ACTION_POWER_OFF */
static button_action_t btn_pwr_countdown_loop(void) {
    const ui_layout_t *layout = ui_get_layout();
    bool button_released = false;

    /* 3 second countdown */
    for (int i = 3; i > 0; i--) {
        M5_display_fillScreen(TFT_BLACK);
        ui_show_countdown(i, layout->center_x, layout->y_line1, layout->y_line1 + 45);
        ui_update_battery_indicator();

        /* Check if button is still pressed during countdown */
        for (int j = 0; j < 20; j++) {  /* 50ms * 20 = 1000ms */
            M5_update();
            if (!M5_BtnPWR_isPressed()) {
                button_released = true;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        if (button_released) {
            /* Button released: Reset device */
            ESP_LOGI(TAG, "PWR button released, restarting device...");
            return BTN_ACTION_RESET;
        }
    }

    /* Button held for 3 seconds: Power off */
    ESP_LOGI(TAG, "PWR button held for 3 seconds, powering off...");
    return BTN_ACTION_POWER_OFF;
}

/* ========== Public Interface ========== */

/* Handle button A press */
button_action_t btn_handle_a_press(void) {
    ble_state_t ble_state = ble_get_state();
    dji_state_t dji_state = dji_get_state();

    if (ble_state == BLE_STATE_IDLE) {
        /* Not connected: Start BLE connection */
        return btn_try_connect() ? BTN_ACTION_CONNECT : BTN_ACTION_NONE;
    }

    if (ble_state == BLE_STATE_CONNECTED) {
        if (dji_state == DJI_STATE_PAIRED || dji_state == DJI_STATE_RECORDING) {
            /* Paired: Toggle recording */
            return btn_toggle_recording() ? BTN_ACTION_TOGGLE_RECORDING : BTN_ACTION_NONE;
        } else {
            /* Not paired: Disconnect */
            return btn_disconnect() ? BTN_ACTION_DISCONNECT : BTN_ACTION_NONE;
        }
    }

    ESP_LOGW(TAG, "Button A pressed but not in appropriate state (BLE=%d, DJI=%d)",
             ble_state, dji_state);
    return BTN_ACTION_NONE;
}

/* Handle button B press */
button_action_t btn_handle_b_press(void) {
    dji_state_t dji_state = dji_get_state();

    if (dji_state == DJI_STATE_PAIRED || dji_state == DJI_STATE_RECORDING) {
        /* Toggle Rec Keep mode */
        bool current = dji_is_rec_keep_mode_enabled();
        esp_err_t ret = dji_set_rec_keep_mode(!current);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set Rec Keep mode: %s", esp_err_to_name(ret));
            return BTN_ACTION_NONE;
        }
        return BTN_ACTION_TOGGLE_REC_KEEP;
    }

    return BTN_ACTION_NONE;
}

/* Handle PWR button press (blocking call with countdown) */
button_action_t btn_handle_pwr_press(void) {
    ESP_LOGI(TAG, "PWR button pressed, starting countdown...");
    const ui_layout_t *layout = ui_get_layout();

    /* Show power off/reset guide */
    M5_display_fillScreen(TFT_BLACK);
    ui_show_poweroff_guide(layout->center_x, layout->y_line1, layout->y_line1 + 45);
    ui_update_battery_indicator();

    /* Small delay to show guide */
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Run countdown loop */
    button_action_t result = btn_pwr_countdown_loop();

    /* Handle result */
    if (result == BTN_ACTION_POWER_OFF) {
        /* Show bye message */
        M5_display_fillScreen(TFT_BLACK);
        ui_show_poweroff_goodbye(layout->center_x, layout->y_line1);
        ui_update_battery_indicator();

        /* Ensure minimum display time */
        vTaskDelay(pdMS_TO_TICKS(500));
        M5_Power_off();
    } else {
        M5_Power_restart();
    }

    /* Not reached, but for completeness */
    return result;
}

#ifdef __cplusplus
}
#endif
