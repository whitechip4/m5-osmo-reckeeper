/* SPDX-License-Identifier: MIT */
/*
 * Display Helper Functions Implementation
 * Utility functions for color determination and text formatting
 */

#include "display/display_helpers.h"
#include <stdio.h>

/* Color definitions from M5GFX (RGB888 format) */
/* M5GFXからの色定義（RGB888形式） */
#define TFT_BLACK   0x000000
#define TFT_RED     0xFF0000
#define TFT_GREEN   0x00FF00
#define TFT_YELLOW  0xFFFF00

/**
 * @brief Get battery color based on level
 * @param battery_level Battery level percentage (0-100)
 * @param is_device true for device battery, false for camera battery
 * @return Color value in RGB888 format
 * @note Device battery: <20% red, >=20% green; Camera battery: <10% red, 10-19% yellow, >=20% green
 */
uint32_t dh_get_battery_color(uint8_t battery_level, bool is_device) {
    if (is_device) {
        return (battery_level < 20) ? TFT_RED : TFT_GREEN;
    } else {
        if (battery_level < 10) {
            return TFT_RED;       /* Critical / 危険 */
        } else if (battery_level < 20) {
            return TFT_YELLOW;    /* Low / 低残量 */
        } else {
            return TFT_GREEN;     /* Normal / 通常 */
        }
    }
}

/**
 * @brief Format recording time as HH:MM:SS
 * @param seconds Total recording time in seconds
 * @param buffer Output buffer for formatted string
 * @param size Size of output buffer
 */
void dh_format_recording_time(uint16_t seconds, char *buffer, size_t size) {
    uint16_t hours = seconds / 3600;
    uint16_t mins = (seconds % 3600) / 60;
    uint16_t secs = seconds % 60;
    snprintf(buffer, size, "%02u:%02u:%02u", hours, mins, secs);
}

/**
 * @brief Format device ID as 4-character hex string
 * @param device_id Device ID value
 * @param buffer Output buffer for formatted string
 * @param size Size of output buffer
 * @note Returns empty string if device_id is 0
 */
void dh_format_device_id(uint32_t device_id, char *buffer, size_t size) {
    if (device_id == 0) {
        buffer[0] = '\0';  /* Return empty string / 空文字を返す */
    } else {
        snprintf(buffer, size, "%04lX", (unsigned long)(device_id & 0xFFFF));
    }
}

/**
 * @brief Format battery text
 * @param battery Battery level (0-100), -1 for error
 * @param prefix Text prefix (e.g., "Bat", "CAMBat")
 * @param buffer Output buffer for formatted string
 * @param size Size of output buffer
 */
void dh_format_battery_text(int battery, const char *prefix, char *buffer, size_t size) {
    if (battery < 0) {
        snprintf(buffer, size, "%s:ERR", prefix);
    } else {
        snprintf(buffer, size, "%s:%d%%", prefix, battery);
    }
}

/**
 * @brief Format SD card remaining time
 * @param remaining_seconds Remaining recording time in seconds
 * @param buffer Output buffer for formatted string
 * @param size Size of output buffer
 */
void dh_format_sd_time(uint32_t remaining_seconds, char *buffer, size_t size) {
    uint32_t hours = remaining_seconds / 3600;
    uint32_t mins = (remaining_seconds % 3600) / 60;

    if (hours > 0) {
        /* 1時間以上: "      SD:XhYm"形式 */
        snprintf(buffer, size, "      SD:%luh%lum", (unsigned long)hours, (unsigned long)mins);
    } else {
        /* 1時間未満: "      SD:Ymin"形式 */
        snprintf(buffer, size, "      SD:%lum", (unsigned long)mins);
    }
}

/**
 * @brief Get camera storage color based on remaining time
 * @param remaining_seconds Remaining recording time in seconds
 * @return Color value in RGB888 format
 * @note <10min: red, 10-30min: yellow, >=30min: green
 */
uint32_t dh_get_storage_time_color(uint32_t remaining_seconds) {
    uint32_t mins = remaining_seconds / 60;
    if (mins < 10) {           /* < 10分 */
        return TFT_RED;         /* Critical / 危険 */
    } else if (mins < 30) {     /* < 30分 */
        return TFT_YELLOW;      /* Low / 低残量 */
    } else {
        return TFT_GREEN;       /* Normal / 通常 */
    }
}
