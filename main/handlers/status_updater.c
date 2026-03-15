/* SPDX-License-Identifier: MIT */
/*
 * Status Updater Implementation
 * Handles periodic status updates in the event loop
 */

#include "handlers/status_updater.h"
#include "ui/ui.h"
#include "dji_protocol.h"
#include "gps_module.h"
#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char *TAG = "status_upd";

/* Update recording time display
 * Only updates when recording and time changes */
static void update_recording_time(void) {
    static uint16_t last_rec_time = 0xFFFF;
    uint16_t rec_time = dji_get_recording_time();

    if (rec_time != last_rec_time) {
        ui_update_recording_time(rec_time);
        last_rec_time = rec_time;
    }
}

/* Update camera battery display
 * Only updates when connected and value changes */
static void update_camera_battery(void) {
    static uint8_t last_camera_battery = 0xFF;
    uint8_t current_camera_battery = dji_get_camera_battery_level();

    if (current_camera_battery != last_camera_battery && current_camera_battery > 0) {
        ui_update_camera_battery(current_camera_battery);
        last_camera_battery = current_camera_battery;
    }
}

/* Update SD card display
 * Only updates when connected and value changes */
static void update_sd_card(void) {
    static uint32_t last_sd_remaining_mb = 0xFFFFFFFF;
    uint32_t current_sd_mb = dji_get_sd_remaining_mb();

    if (current_sd_mb != last_sd_remaining_mb && current_sd_mb > 0) {
        ui_update_sd_card(current_sd_mb);
        last_sd_remaining_mb = current_sd_mb;
    }
}

/* Update GPS polling and display
 * Polls GPS at configured interval
 * Sends GPS data to camera when paired and GPS is available */
static void update_gps(dji_state_t dji_state) {
    static gps_status_t last_gps_status = GPS_STATUS_STANDBY;
    static uint8_t last_siv = 0xFF;
    static uint32_t last_gps_poll_time = 0;
    static uint32_t last_display_update = 0;

    /* Poll GPS at configured interval */
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (current_time - last_gps_poll_time >= GPS_POLL_INTERVAL_MS) {
        gps_poll();
        last_gps_poll_time = current_time;

        /* Send GPS data to camera when paired (continuous sending) */
        if ((dji_state == DJI_STATE_PAIRED || dji_state == DJI_STATE_RECORDING) && gps_is_found()) {
            esp_err_t ret = dji_send_gps_module_data();
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to send GPS data: %s", esp_err_to_name(ret));
            }
        }
    }

    /* Update GPS status display continuously (every 200ms) */
    /* GPS状態表示を常時更新（200msごと） */
    if (current_time - last_display_update >= 200) {
        gps_status_t current_gps_status = gps_get_status();

        /* Check if status or satellite count changed */
        /* 状態または衛星数が変化したかを確認 */
        gps_data_t gps_data;
        uint8_t current_siv = 0xFF;
        if (gps_get_data(&gps_data) == ESP_OK) {
            current_siv = gps_data.num_satellites;
        }

        if (current_gps_status != last_gps_status || current_siv != last_siv) {
            ui_update_gps_status();
            last_gps_status = current_gps_status;
            last_siv = current_siv;
        }
        last_display_update = current_time;
    }
}

/* ========== Public Interface ========== */

void status_update_all(void) {
    dji_state_t dji_state = dji_get_state();

    /* Update recording time when recording */
    if (dji_state == DJI_STATE_RECORDING) {
        update_recording_time();
    }

    /* Update camera battery when connected */
    if (dji_state == DJI_STATE_PAIRED || dji_state == DJI_STATE_RECORDING) {
        update_camera_battery();
        update_sd_card();
    }

    /* Update GPS polling and display */
    update_gps(dji_state);
}

#ifdef __cplusplus
}
#endif
