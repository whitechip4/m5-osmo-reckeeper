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

/**
 * @brief Initialize UI state
 * @note Resets all UI state to initial values
 */
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

/**
 * @brief Set main UI state
 * @param state DJI protocol state
 * @param device_id Camera device ID
 * @param rec_time Recording time in seconds
 */
void ui_state_set_main(dji_state_t state, uint32_t device_id, uint16_t rec_time) {
    s_display_state.main_state = state;
    s_display_state.device_id = device_id;
    s_display_state.recording_time = rec_time;
}

/**
 * @brief Set GPS status
 * @param status GPS status
 * @param satellites Number of satellites
 */
void ui_state_set_gps(gps_status_t status, uint8_t satellites) {
    s_display_state.status.gps_status = status;
    s_display_state.status.gps_satellites = satellites;
}

/**
 * @brief Set battery levels
 * @param device_batt Device battery level (0-100%, -1 for error)
 * @param camera_batt Camera battery level (0-100%, 0 for unavailable)
 */
void ui_state_set_battery(int device_batt, int camera_batt) {
    s_display_state.status.device_battery = device_batt;
    s_display_state.status.camera_battery = camera_batt;
}

/**
 * @brief Set camera storage remaining time
 * @param remaining_seconds Remaining recording time in seconds
 */
void ui_state_set_storage_time(uint32_t remaining_seconds) {
    s_display_state.status.sd_remaining_time = remaining_seconds;
}

/**
 * @brief Set Rec Keep mode status
 * @param enabled true if Rec Keep mode enabled, false otherwise
 */
void ui_state_set_rec_keep(bool enabled) {
    s_display_state.status.rec_keep_enabled = enabled;
}

/**
 * @brief Set power off screen state
 * @param active true to show power off screen, false for normal display
 * @param countdown Countdown value (3, 2, 1), 0 for guide screen
 * @param goodbye true to show goodbye message, false otherwise
 */
void ui_state_set_poweroff(bool active, int countdown, bool goodbye) {
    s_display_state.poweroff.active = active;
    s_display_state.poweroff.countdown = countdown;
    s_display_state.poweroff.goodbye = goodbye;
}

/**
 * @brief Get current UI state
 * @return Pointer to current display state
 */
const display_state_t* ui_state_get(void) {
    return &s_display_state;
}

#ifdef __cplusplus
}
#endif
