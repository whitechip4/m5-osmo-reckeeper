/* SPDX-License-Identifier: MIT */
/*
 * Status Updater Implementation
 * Handles periodic status updates in the event loop
 * Updates UI state only, no drawing (rendering is done in main loop)
 */

#include "handlers/status_updater.h"
#include "ui/ui_state.h"
#include "dji_protocol.h"
#include "gps_module.h"
#include "m5_wrapper.h"
#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char *TAG = "status_upd";

/* Update recording time state
 * Only updates when recording and time changes */
static void update_recording_time(void) {
    static uint16_t last_rec_time = 0xFFFF;
    uint16_t rec_time = dji_get_recording_time();

    if (rec_time != last_rec_time) {
        dji_state_t state = dji_get_state();
        uint32_t device_id = dji_get_device_id();
        ui_state_set_main(state, device_id, rec_time);
        last_rec_time = rec_time;
    }
}

/* Update battery state (device + camera)
 * Only updates when value changes */
static void update_battery(void) {
    static int last_device_battery = -2;  /* Initialize to impossible value */
    static int last_camera_battery = -1;

    int current_device_battery = M5_Power_getBatteryLevel();
    int current_camera_battery = dji_get_camera_battery_level();

    if (current_device_battery != last_device_battery ||
        current_camera_battery != last_camera_battery) {
        ui_state_set_battery(current_device_battery, current_camera_battery);
        last_device_battery = current_device_battery;
        last_camera_battery = current_camera_battery;
    }
}

/* Update SD card state
 * Only updates when value changes */
static void update_sd_card(void) {
    static uint32_t last_sd_remaining_mb = 0xFFFFFFFF;
    uint32_t current_sd_mb = dji_get_sd_remaining_mb();

    if (current_sd_mb != last_sd_remaining_mb && current_sd_mb > 0) {
        ui_state_set_sd(current_sd_mb);
        last_sd_remaining_mb = current_sd_mb;
    }
}

/* Update GPS polling and state
 * Polls GPS at configured interval
 * Sends GPS data to camera when paired and GPS is available */
static void update_gps(dji_state_t dji_state) {
    static gps_status_t last_gps_status = GPS_STATUS_STANDBY;
    static uint8_t last_siv = 0xFF;
    static uint32_t last_gps_poll_time = 0;
    static uint32_t last_state_update = 0;

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

    /* Update GPS state continuously (every 200ms) */
    /* GPS状態を常時更新（200msごと） */
    if (current_time - last_state_update >= 200) {
        gps_status_t current_gps_status = gps_get_status();

        /* Check if status or satellite count changed */
        /* 状態または衛星数が変化したかを確認 */
        gps_data_t gps_data;
        uint8_t current_siv = 0xFF;
        if (gps_get_data(&gps_data) == ESP_OK) {
            current_siv = gps_data.num_satellites;
        }

        if (current_gps_status != last_gps_status || current_siv != last_siv) {
            ui_state_set_gps(current_gps_status, current_siv);
            last_gps_status = current_gps_status;
            last_siv = current_siv;
        }
        last_state_update = current_time;
    }
}

/* ========== Public Interface ========== */

void status_update_all(void) {
    dji_state_t dji_state = dji_get_state();

    /* Update recording time when recording */
    if (dji_state == DJI_STATE_RECORDING) {
        update_recording_time();
    }

    /* Update battery (always) */
    update_battery();

    /* Update camera battery and SD card when connected */
    if (dji_state == DJI_STATE_PAIRED || dji_state == DJI_STATE_RECORDING) {
        update_sd_card();
    }

    /* Update GPS polling and state */
    update_gps(dji_state);
}

#ifdef __cplusplus
}
#endif
