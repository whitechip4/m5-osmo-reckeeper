/* SPDX-License-Identifier: MIT */
/*
 * UI Module Interface
 * Provides all LCD display functions for the application
 */

#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* State display functions */
/* 状態表示関数 */

/* Show idle state (waiting for pairing) */
void ui_show_idle_state(void);

/* Show pairing state with device ID
 * Args:
 *   device_id: Osmo device ID to display */
void ui_show_pairing_state(uint32_t device_id);

/* Show paired state (ready to record) */
void ui_show_paired_state(void);

/* Show recording state with current time
 * Args:
 *   rec_time: Recording time in seconds */
void ui_show_recording_state(uint16_t rec_time);

/* Show restarting state (Rec Keep mode re-recording) */
void ui_show_restarting_state(void);

/* Show failed state (pairing error) */
void ui_show_failed_state(void);

/* Status update functions (incremental updates) */
/* ステータス更新関数（差分更新） */

/* Update recording time display (recording state only)
 * Args:
 *   rec_time: Recording time in seconds */
void ui_update_recording_time(uint16_t rec_time);

/* Update camera battery display (when connected)
 * Args:
 *   battery_level: Camera battery percentage (0-100) */
void ui_update_camera_battery(uint8_t battery_level);

/* Update SD card display (when connected)
 * Args:
 *   remaining_mb: Remaining capacity in megabytes */
void ui_update_sd_card(uint32_t remaining_mb);

/* Update GPS status display */
void ui_update_gps_status(void);

/* Update battery indicator (device + camera) */
void ui_update_battery_indicator(void);

/* Update Rec Keep mode indicator (top-right) */
void ui_update_rec_keep_indicator(bool enabled);

/* Power off display functions */
/* 電源OFF表示関数 */

/* Show power off/reset guide
 * Args:
 *   x: Center X coordinate
 *   y1: First line Y coordinate
 *   y2: Second line Y coordinate */
void ui_show_poweroff_guide(int x, int y1, int y2);

/* Show countdown number
 * Args:
 *   count: Countdown number (3, 2, 1)
 *   x: Center X coordinate
 *   y1: First line Y coordinate
 *   y2: Second line Y coordinate */
void ui_show_countdown(int count, int x, int y1, int y2);

/* Show power off goodbye message
 * Args:
 *   x: Center X coordinate
 *   y: Y coordinate */
void ui_show_poweroff_goodbye(int x, int y);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */
