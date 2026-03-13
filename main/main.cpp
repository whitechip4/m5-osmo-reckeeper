/* SPDX-License-Identifier: MIT */
/*
 * M5StickC Plus2 DJI Osmo360 BLE Remote
 * Development Step 1: LCD Display Test
 */

#include <M5Unified.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Osmo360 BLE Remote - LCD Test Starting...");

    // Initialize M5StickC Plus2 (including LCD, power management, etc.)
    M5.begin();

    ESP_LOGI(TAG, "M5Unified initialized");

    // Clear screen
    M5.Display.fillScreen(TFT_BLACK);

    // Display "STOP" as initial state (white text, centered)
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setTextDatum(top_center);
    M5.Display.setTextSize(3);
    M5.Display.drawString("STOP", M5.Display.width() / 2, M5.Display.height() / 2);

    ESP_LOGI(TAG, "Display test: 'STOP' should be visible on LCD");

    // Keep program running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGD(TAG, "LCD test running...");
    }
}
