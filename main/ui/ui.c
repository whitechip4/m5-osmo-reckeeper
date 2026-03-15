/* SPDX-License-Identifier: MIT */
/*
 * UI Module Implementation
 * Provides all LCD display functions for the application
 */

#include "ui/ui.h"
#include "ui/ui_layout.h"
#include "display/display_helpers.h"
#include "ble_simple.h"
#include "dji_protocol.h"
#include "gps_module.h"
#include "esp_log.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration for M5Unified C interface */
struct M5UnifiedImpl;
extern struct M5UnifiedImpl M5;

void M5_display_fillScreen(int color);
void M5_display_setTextColor(int textcolor, int textbgcolor);
void M5Display_setTextDatum(int datum);
void M5_display_setTextSize(float size);
void M5_display_drawString(const char *string, int x, int y);
void M5_display_fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void M5_display_fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color);
int M5_display_width(void);
int M5_display_height(void);
int M5_Power_getBatteryLevel(void);

#ifdef __cplusplus
}
#endif

/* Color definitions from M5GFX (RGB888 format) */
/* M5GFXからの色定義（RGB888形式） */
#define TFT_BLACK       0x000000
#define TFT_WHITE       0xFFFFFF
#define TFT_RED         0xFF0000
#define TFT_GREEN       0x00FF00
#define TFT_YELLOW      0xFFFF00
#define TFT_CYAN        0x00FFFF

/* Text datum constants */
/* テキスト基準点定数 */
#define top_left   0
#define top_center 1
#define top_right  2

/* GPS status display strings and colors */
/* GPS状態表示文字列と色 */
typedef struct {
    const char *text;
    uint32_t color;
} state_display_t;

static const state_display_t gps_status_display[] = {
    [GPS_STATUS_NOGPS]    = {"GPS:NOGPS", TFT_RED},
    [GPS_STATUS_STANDBY]  = {"GPS:STBY", TFT_YELLOW},
    [GPS_STATUS_SEARCHING] = {"GPS:SEARCH", TFT_CYAN},
    [GPS_STATUS_OK]       = {"GPS:OK", TFT_GREEN}
};

/* Draw GPS status in top-center area */
/* 上部中央にGPS状態を描画 */
static void draw_gps_status(void) {
    const ui_layout_t *layout = ui_get_layout();
    gps_status_t current_gps_status = gps_get_status();

    /* Clear GPS status area (2 lines for status + satellite count) */
    /* GPS状態エリアをクリア（状態+衛星数の2行分） */
    M5_display_fillRect(layout->center_x - 40, layout->y_gps, 80, 20, TFT_BLACK);

    /* Draw GPS status */
    /* GPS状態を描画 */
    const char *gps_text = gps_status_display[current_gps_status].text;
    uint32_t gps_color = gps_status_display[current_gps_status].color;

    M5_display_setTextColor(gps_color, TFT_BLACK);
    M5Display_setTextDatum(top_center);
    M5_display_setTextSize(1);
    M5_display_drawString(gps_text, layout->center_x, layout->y_gps);

    /* Draw satellite count for SEARCHING and OK statuses */
    /* SEARCHINGとOK状態の場合のみ衛星数を表示 */
    if (current_gps_status == GPS_STATUS_SEARCHING || current_gps_status == GPS_STATUS_OK) {
        gps_data_t gps_data;
        if (gps_get_data(&gps_data) == ESP_OK) {
            char siv_buf[16];
            snprintf(siv_buf, sizeof(siv_buf), "SiV:%d", gps_data.num_satellites);

            M5_display_setTextColor(TFT_WHITE, TFT_BLACK);
            M5Display_setTextDatum(top_center);
            M5_display_setTextSize(1);
            M5_display_drawString(siv_buf, layout->center_x, layout->y_gps_siv);
        }
    }
}

/* Multiline display function */
/* マルチライン表示関数 */
static void display_multiline(const char *line1, const char *line2,
                              uint32_t color1, uint32_t color2,
                              float size1, float size2) {
    M5_display_fillScreen(TFT_BLACK);
    const ui_layout_t *layout = ui_get_layout();

    if (line1) {
        M5_display_setTextColor(color1, TFT_BLACK);
        M5Display_setTextDatum(top_center);
        M5_display_setTextSize(size1);
        M5_display_drawString(line1, layout->center_x, layout->y_line1);
    }

    if (line2) {
        M5_display_setTextColor(color2, TFT_BLACK);
        M5Display_setTextDatum(top_center);
        M5_display_setTextSize(size2);
        M5_display_drawString(line2, layout->center_x, layout->y_line1 + 25);
    }

    /* Draw GPS status after screen update */
    draw_gps_status();
}

/* Draw recording icon and text
 * Args:
 *   is_recording: true for REC (circle), false for STOP (square)
 *   center_x: Center X coordinate
 *   y: Y coordinate */
static void draw_recording_icon_with_text(bool is_recording, int center_x, int y) {
    int text_x_start = center_x - 25;

    if (is_recording) {
        /* Draw REC icon (circle) + "REC" text */
        /* RECアイコン（円）+ "REC"テキスト描画 */
        int icon_radius = 6;
        M5_display_fillCircle(text_x_start, y, icon_radius, TFT_RED);
        M5_display_setTextColor(TFT_RED, TFT_BLACK);
        M5Display_setTextDatum(top_left);
        M5_display_setTextSize(2);
        M5_display_drawString("REC", text_x_start + icon_radius + 8, y - 8);
    } else {
        /* Draw STOP icon (square) + "STOP" text */
        /* STOPアイコン（四角）+ "STOP"テキスト描画 */
        int icon_size = 10;
        M5_display_fillRect(text_x_start, y - icon_size/2,
                            icon_size, icon_size, TFT_GREEN);
        M5_display_setTextColor(TFT_GREEN, TFT_BLACK);
        M5Display_setTextDatum(top_left);
        M5_display_setTextSize(2);
        M5_display_drawString("STOP", text_x_start + icon_size + 5, y - 8);
    }
}

/* Draw device ID line at bottom
 * Args:
 *   center_x: Center X coordinate
 *   y: Y coordinate */
static void draw_device_id_line(int center_x, int y) {
    uint32_t device_id = dji_get_device_id();
    if (device_id == 0) {
        return;
    }

    char device_id_buf[16];
    dh_format_device_id(device_id, device_id_buf, sizeof(device_id_buf));
    char osmo_id_buf[32];
    snprintf(osmo_id_buf, sizeof(osmo_id_buf), "Osmo: %s", device_id_buf);

    M5_display_setTextColor(TFT_CYAN, TFT_BLACK);
    M5Display_setTextDatum(top_center);
    M5_display_setTextSize(1);
    M5_display_drawString(osmo_id_buf, center_x, y);
}

/* Draw battery indicators in top-left corner */
/* 左上にバッテリー表示を描画（デバイス・カメラ・SD） */
static void draw_battery_indicator_internal(void) {
    const ui_layout_t *layout = ui_get_layout();
    int device_battery = M5_Power_getBatteryLevel();

    /* Draw device battery (top line) */
    /* デバイスバッテリーを描画（上段） */
    M5_display_setTextSize(1);
    M5Display_setTextDatum(top_left);

    if (device_battery < 0) {
        M5_display_setTextColor(TFT_RED, TFT_BLACK);
        M5_display_drawString("DEV:ERR", 2, layout->y_battery_dev);
    } else {
        char dev_str[16];
        dh_format_battery_text(device_battery, "DEV", dev_str, sizeof(dev_str));
        uint32_t color = dh_get_battery_color(device_battery, true);
        M5_display_setTextColor(color, TFT_BLACK);
        M5_display_drawString(dev_str, 2, layout->y_battery_dev);
    }

    /* Draw camera battery (middle line, only when connected and valid) */
    /* カメラバッテリーを描画（中段、接続時かつ有効な場合のみ） */
    dji_state_t dji_state = dji_get_state();
    if (dji_state == DJI_STATE_PAIRED || dji_state == DJI_STATE_RECORDING) {
        int camera_battery = dji_get_camera_battery_level();
        if (camera_battery > 0) {
            char cam_str[16];
            dh_format_battery_text(camera_battery, "CAM", cam_str, sizeof(cam_str));
            uint32_t cam_color = dh_get_battery_color(camera_battery, false);
            M5_display_setTextColor(cam_color, TFT_BLACK);
            M5_display_drawString(cam_str, 2, layout->y_battery_cam);
        }

        /* Draw SD card (bottom line) */
        uint32_t sd_mb = dji_get_sd_remaining_mb();
        if (sd_mb > 0) {
            char sd_str[32];
            dh_format_sd_text(sd_mb, sd_str, sizeof(sd_str));
            uint32_t sd_color = dh_get_sd_card_color(sd_mb);
            M5_display_setTextColor(sd_color, TFT_BLACK);
            M5_display_drawString(sd_str, 2, layout->y_battery_sd);
        }
    }
}

/* ========== Public Interface ========== */

/* Show idle state (waiting for pairing) */
void ui_show_idle_state(void) {
    display_multiline("PUSH TO", "Pairing", TFT_WHITE, TFT_WHITE, 2, 2);
}

/* Show pairing state with device ID */
void ui_show_pairing_state(uint32_t device_id) {
    char device_id_buf[16];
    dh_format_device_id(device_id, device_id_buf, sizeof(device_id_buf));
    display_multiline("Finding Device...", device_id_buf, TFT_CYAN, TFT_CYAN, 1, 2);
}

/* Show paired state (ready to record) */
void ui_show_paired_state(void) {
    const ui_layout_t *layout = ui_get_layout();

    M5_display_fillScreen(TFT_BLACK);
    draw_recording_icon_with_text(false, layout->center_x, layout->y_line1);

    M5_display_setTextColor(TFT_WHITE, TFT_BLACK);
    M5Display_setTextDatum(top_center);
    M5_display_setTextSize(1);
    M5_display_drawString("Press to start", layout->center_x, layout->y_line2);

    draw_device_id_line(layout->center_x, layout->y_line3);
    draw_gps_status();
}

/* Show recording state with current time */
void ui_show_recording_state(uint16_t rec_time) {
    const ui_layout_t *layout = ui_get_layout();

    M5_display_fillScreen(TFT_BLACK);
    draw_recording_icon_with_text(true, layout->center_x, layout->y_line1);

    char time_buf[16];
    dh_format_recording_time(rec_time, time_buf, sizeof(time_buf));
    M5_display_setTextColor(TFT_RED, TFT_BLACK);
    M5Display_setTextDatum(top_center);
    M5_display_setTextSize(2);
    M5_display_drawString(time_buf, layout->center_x, layout->y_line2);

    draw_device_id_line(layout->center_x, layout->y_line3);
    draw_gps_status();
}

/* Show restarting state (Rec Keep mode re-recording) */
void ui_show_restarting_state(void) {
    display_multiline("Restart", "Recording...", TFT_YELLOW, TFT_YELLOW, 2, 2);
}

/* Show failed state (pairing error) */
void ui_show_failed_state(void) {
    display_multiline("PAIR ERR", NULL, TFT_RED, TFT_BLACK, 2, 1);
}

/* Update recording time display (recording state only) */
void ui_update_recording_time(uint16_t rec_time) {
    const ui_layout_t *layout = ui_get_layout();

    /* Clear only the time area (bottom line) */
    M5_display_fillRect(0, layout->y_line2 - 2, M5_display_width(), 20, TFT_BLACK);

    /* Redraw time */
    char time_buf[16];
    dh_format_recording_time(rec_time, time_buf, sizeof(time_buf));
    M5_display_setTextColor(TFT_RED, TFT_BLACK);
    M5Display_setTextDatum(top_center);
    M5_display_setTextSize(2);
    M5_display_drawString(time_buf, layout->center_x, layout->y_line2);

    /* Redraw device ID (cleared by time area clear) */
    draw_device_id_line(layout->center_x, layout->y_line3);
}

/* Update camera battery display (when connected) */
void ui_update_camera_battery(uint8_t battery_level) {
    const ui_layout_t *layout = ui_get_layout();

    /* Clear camera battery area */
    M5_display_fillRect(0, layout->y_battery_cam, M5_display_width(), 10, TFT_BLACK);

    if (battery_level > 0) {
        char cam_str[16];
        dh_format_battery_text(battery_level, "CAM", cam_str, sizeof(cam_str));
        uint32_t cam_color = dh_get_battery_color(battery_level, false);
        M5_display_setTextSize(1);
        M5Display_setTextDatum(top_left);
        M5_display_setTextColor(cam_color, TFT_BLACK);
        M5_display_drawString(cam_str, 2, layout->y_battery_cam);
    }
}

/* Update SD card display (when connected) */
void ui_update_sd_card(uint32_t remaining_mb) {
    const ui_layout_t *layout = ui_get_layout();

    /* Clear SD card area */
    M5_display_fillRect(0, layout->y_battery_sd, M5_display_width(), 10, TFT_BLACK);

    if (remaining_mb > 0) {
        char sd_str[32];
        dh_format_sd_text(remaining_mb, sd_str, sizeof(sd_str));
        uint32_t sd_color = dh_get_sd_card_color(remaining_mb);
        M5_display_setTextSize(1);
        M5Display_setTextDatum(top_left);
        M5_display_setTextColor(sd_color, TFT_BLACK);
        M5_display_drawString(sd_str, 2, layout->y_battery_sd);
    }
}

/* Update GPS status display */
void ui_update_gps_status(void) {
    draw_gps_status();
}

/* Update battery indicator (device + camera) */
void ui_update_battery_indicator(void) {
    draw_battery_indicator_internal();
}

/* Update Rec Keep mode indicator (top-right) */
void ui_update_rec_keep_indicator(bool enabled) {
    M5_display_setTextSize(1);
    M5Display_setTextDatum(top_right);

    if (enabled) {
        M5_display_setTextColor(TFT_RED, TFT_BLACK);
        M5_display_drawString("RecKeep:ON", M5_display_width() - 2, 2);
    } else {
        M5_display_setTextColor(TFT_GREEN, TFT_BLACK);
        M5_display_drawString("RecKeep:OFF", M5_display_width() - 2, 2);
    }
}

/* Show power off/reset guide */
void ui_show_poweroff_guide(int x, int y1, int y2) {
    /* Line 1: "Press 3s" + "to PWR OFF" (2-line, yellow, large) */
    M5_display_setTextColor(TFT_YELLOW, TFT_BLACK);
    M5Display_setTextDatum(top_center);
    M5_display_setTextSize(2);
    M5_display_drawString("Press 3s", x, y1);
    M5_display_drawString("to PWR OFF", x, y1 + 16);

    /* Line 2: "Release to reset" (1-line, white, small) */
    M5_display_setTextColor(TFT_WHITE, TFT_BLACK);
    M5_display_setTextSize(1);
    M5_display_drawString("Release to reset", x, y2);
}

/* Show countdown number */
void ui_show_countdown(int count, int x, int y1, int y2) {
    /* Show countdown number in red */
    M5_display_setTextColor(TFT_RED, TFT_BLACK);
    M5Display_setTextDatum(top_center);
    M5_display_setTextSize(3);

    char countdown[8];
    snprintf(countdown, sizeof(countdown), "%d", count);
    M5_display_drawString(countdown, x, y1);

    /* Show "Release to reset" message below countdown */
    M5_display_setTextSize(1);
    M5_display_setTextColor(TFT_WHITE, TFT_BLACK);
    M5_display_drawString("Release to reset", x, y2);
}

/* Show power off goodbye message */
void ui_show_poweroff_goodbye(int x, int y) {
    M5_display_setTextColor(TFT_WHITE, TFT_BLACK);
    M5Display_setTextDatum(top_center);
    M5_display_setTextSize(2);
    M5_display_drawString("BYE", x, y);
}

#ifdef __cplusplus
}
#endif
