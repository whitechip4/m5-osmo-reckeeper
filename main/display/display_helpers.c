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

/* Get battery color based on level
 * Device battery: <20% red, >=20% green
 * Camera battery: <10% red, 10-19% yellow, >=20% green */
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

/* Get SD card color based on remaining capacity
 * <1GB: red, 1-4GB: yellow, >=4GB: green */
uint32_t dh_get_sd_card_color(uint32_t remaining_mb) {
    if (remaining_mb < 1024) {        /* < 1 GB */
        return TFT_RED;               /* Critical / 危険 */
    } else if (remaining_mb < 4096) { /* < 4 GB */
        return TFT_YELLOW;            /* Low / 低残量 */
    } else {
        return TFT_GREEN;             /* Normal / 通常 */
    }
}

/* Format recording time as MM:SS */
void dh_format_recording_time(uint16_t seconds, char *buffer, size_t size) {
    uint16_t mins = seconds / 60;
    uint16_t secs = seconds % 60;
    snprintf(buffer, size, "%02u:%02u", mins, secs);
}

/* Format device ID as 4-character hex string */
void dh_format_device_id(uint32_t device_id, char *buffer, size_t size) {
    if (device_id == 0) {
        buffer[0] = '\0';  /* Return empty string / 空文字を返す */
    } else {
        snprintf(buffer, size, "%04lX", (unsigned long)(device_id & 0xFFFF));
    }
}

/* Format battery text */
void dh_format_battery_text(int battery, const char *prefix, char *buffer, size_t size) {
    if (battery < 0) {
        snprintf(buffer, size, "%s:ERR", prefix);
    } else {
        snprintf(buffer, size, "%s:%d%%", prefix, battery);
    }
}

/* Format SD card text (auto-select MB or GB) */
void dh_format_sd_text(uint32_t remaining_mb, char *buffer, size_t size) {
    if (remaining_mb >= 1024) {
        snprintf(buffer, size, "SD:%.1fGB", remaining_mb / 1024.0);
    } else {
        snprintf(buffer, size, "SD:%luMB", (unsigned long)remaining_mb);
    }
}
