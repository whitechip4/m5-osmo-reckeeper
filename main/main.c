/* SPDX-License-Identifier: MIT */
/*
 * M5StickC Plus2 DJI Osmo360 BLE Remote
 * Main Application Entry Point
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* M5Unified is C++, but we can use it from C with extern "C" linkage */
#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration for M5Unified C interface */
struct M5UnifiedImpl;
extern struct M5UnifiedImpl M5;

/* M5Unified C++ API wrapper functions */
void M5_update(void);
int M5_BtnA_wasPressed(void);
int M5_BtnB_wasPressed(void);
int M5_BtnPWR_wasPressed(void);

#ifdef __cplusplus
}
#endif

#include "handlers/button_handlers.h"
#include "handlers/status_updater.h"
#include "handlers/system_init.h"
#include "ui/ui_renderer.h"

static const char *TAG = "main";

/* Display refresh interval (10Hz) */
#define DISPLAY_REFRESH_MS 100

/* ========== Main Entry Point ========== */

void app_main(void) {
    /* Initialize all systems */
    if (!system_initialize()) {
        ESP_LOGE(TAG, "System initialization failed");
        return;
    }

    /* Main event loop */
    static uint32_t last_display_update = 0;

    while (1) {
        M5_update();

        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

        /* Update all status states (no drawing) */
        status_update_all();

        /* Handle button A press */
        if (M5_BtnA_wasPressed()) {
            btn_handle_a_press();
        }

        /* Handle button B press */
        if (M5_BtnB_wasPressed()) {
            btn_handle_b_press();
        }

        /* Handle PWR button press */
        if (M5_BtnPWR_wasPressed()) {
            btn_handle_pwr_press();
            break;
        }

        /* Periodic rendering (10Hz) */
        if (current_time - last_display_update >= DISPLAY_REFRESH_MS) {
            ui_renderer_render();
            last_display_update = current_time;
        }

        /* Small delay to prevent watchdog trigger */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
