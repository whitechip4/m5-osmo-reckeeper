/* SPDX-License-Identifier: MIT */
/*
 * UI State Management Implementation
 * Single source of truth for all UI display data
 */

#include "ui/ui_state.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char *TAG = "ui_state";

/* Global display state
 * Single source of truth for all UI data */
static display_state_t s_display_state = {
    .main_state = DJI_STATE_IDLE,
    .device_id = 0,
    .recording_time = 0,
    .status = {
        .gps_status = GPS_STATUS_NOGPS,
        .gps_satellites = 0,
        .device_battery = -1,
        .camera_battery = 0,
        .sd_remaining_time = 0,
        .rec_keep_enabled = false
    },
    .poweroff = {
        .active = false,
        .countdown = 0,
        .goodbye = false
    }
};

/* ========== Public Interface ========== */

void ui_state_init(void) {
    /* Reset to initial state */
    s_display_state.main_state = DJI_STATE_IDLE;
    s_display_state.device_id = 0;
    s_display_state.recording_time = 0;
    s_display_state.status.gps_status = GPS_STATUS_NOGPS;
    s_display_state.status.gps_satellites = 0;
    s_display_state.status.device_battery = -1;
    s_display_state.status.camera_battery = 0;
    s_display_state.status.sd_remaining_time = 0;
    s_display_state.status.rec_keep_enabled = false;
    s_display_state.poweroff.active = false;
    s_display_state.poweroff.countdown = 0;
    s_display_state.poweroff.goodbye = false;

    ESP_LOGI(TAG, "UI state initialized");
}

void ui_state_set_main(dji_state_t state, uint32_t device_id, uint16_t rec_time) {
    s_display_state.main_state = state;
    s_display_state.device_id = device_id;
    s_display_state.recording_time = rec_time;
}

void ui_state_set_gps(gps_status_t status, uint8_t satellites) {
    s_display_state.status.gps_status = status;
    s_display_state.status.gps_satellites = satellites;
}

void ui_state_set_battery(int device_batt, int camera_batt) {
    s_display_state.status.device_battery = device_batt;
    s_display_state.status.camera_battery = camera_batt;
}

void ui_state_set_sd_time(uint32_t remaining_seconds) {
    s_display_state.status.sd_remaining_time = remaining_seconds;
}

void ui_state_set_rec_keep(bool enabled) {
    s_display_state.status.rec_keep_enabled = enabled;
}

void ui_state_set_poweroff(bool active, int countdown, bool goodbye) {
    s_display_state.poweroff.active = active;
    s_display_state.poweroff.countdown = countdown;
    s_display_state.poweroff.goodbye = goodbye;
}

const display_state_t* ui_state_get(void) {
    return &s_display_state;
}

#ifdef __cplusplus
}
#endif
