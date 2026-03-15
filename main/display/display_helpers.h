/* SPDX-License-Identifier: MIT */
/*
 * Display Helper Functions
 * Utility functions for color determination and text formatting
 */

#ifndef DISPLAY_HELPERS_H
#define DISPLAY_HELPERS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Color determination functions */
/* 色決定関数 */

/* Get battery color based on level
 * Args:
 *   battery_level: Battery percentage (0-100)
 *   is_device: true for device battery, false for camera battery
 * Returns: RGB565 color value */
uint32_t dh_get_battery_color(uint8_t battery_level, bool is_device);

/* Formatting functions */
/* フォーマット関数 */

/* Format recording time as MM:SS
 * Args:
 *   seconds: Recording time in seconds
 *   buffer: Output buffer
 *   size: Buffer size */
void dh_format_recording_time(uint16_t seconds, char *buffer, size_t size);

/* Format device ID as 4-character hex string
 * Args:
 *   device_id: Device ID (32-bit)
 *   buffer: Output buffer
 *   size: Buffer size */
void dh_format_device_id(uint32_t device_id, char *buffer, size_t size);

/* Format battery text
 * Args:
 *   battery: Battery percentage (0-100)
 *   prefix: Text prefix (e.g., "DEV", "CAM", "SD")
 *   buffer: Output buffer
 *   size: Buffer size */
void dh_format_battery_text(int battery, const char *prefix, char *buffer, size_t size);

/* Format SD card remaining time as "SD:XhYm" or "SD:Ymin"
 * Args:
 *   remaining_seconds: Remaining recording time in seconds
 *   buffer: Output buffer
 *   size: Buffer size */
void dh_format_sd_time(uint32_t remaining_seconds, char *buffer, size_t size);

/* Get SD card color based on remaining time
 * Args:
 *   remaining_seconds: Remaining recording time in seconds
 * Returns: RGB565 color value */
uint32_t dh_get_sd_time_color(uint32_t remaining_seconds);

#endif /* DISPLAY_HELPERS_H */
