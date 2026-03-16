/* SPDX-License-Identifier: MIT */
/*
 * UI Renderer Implementation
 * Single render pass that draws all UI elements and pushes to display
 */

#include "ui/ui_renderer.h"
#include "ui/ui_state.h"
#include "ui/ui_layout.h"
#include "display/display_helpers.h"
#include "m5_wrapper.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
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

static const char *TAG = "ui_renderer";

/* Sprite buffer for double buffering */
static sprite_buffer_t *s_buffer = NULL;

/* GPS status display strings and colors */
typedef struct {
    const char *text;
    uint32_t color;
} gps_display_info_t;

static const gps_display_info_t gps_status_display[] = {
    [GPS_STATUS_NOGPS]    = {"GPS:NOGPS", TFT_RED},
    [GPS_STATUS_STANDBY]  = {"GPS:STBY", TFT_YELLOW},
    [GPS_STATUS_SEARCHING] = {"GPS:SEARCH", TFT_CYAN},
    [GPS_STATUS_OK]       = {"GPS:OK", TFT_GREEN}
};

/* ========== Rendering Helper Functions ========== */

/**
 * @brief Draw GPS status at top-center
 * @note Displays GPS status text and satellite count (when available)
 */
static void render_gps_status(void) {
    const display_state_t *state = ui_state_get();
    const ui_layout_t *layout = ui_get_layout();
    const gps_display_info_t *info = &gps_status_display[state->status.gps_status];

    /* Draw GPS status text */
    M5Sprite_setTextColor(s_buffer, info->color, TFT_BLACK);
    M5Sprite_setTextDatum(s_buffer, top_center);
    M5Sprite_setTextSize(s_buffer, 1);
    M5Sprite_drawString(s_buffer, info->text, layout->center_x, layout->y_gps);

    /* Draw satellite count for SEARCHING and OK statuses */
    if (state->status.gps_status == GPS_STATUS_SEARCHING ||
        state->status.gps_status == GPS_STATUS_OK) {
        char siv_buf[16];
        snprintf(siv_buf, sizeof(siv_buf), "SiV:%d", state->status.gps_satellites);

        M5Sprite_setTextColor(s_buffer, TFT_WHITE, TFT_BLACK);
        M5Sprite_setTextDatum(s_buffer, top_center);
        M5Sprite_setTextSize(s_buffer, 1);
        M5Sprite_drawString(s_buffer, siv_buf, layout->center_x, layout->y_gps_siv);
    }
}

/**
 * @brief Draw battery indicators at top-left
 * @note Displays device battery, camera battery, and SD card remaining time
 */
static void render_battery_indicators(void) {
    const display_state_t *state = ui_state_get();
    const ui_layout_t *layout = ui_get_layout();

    M5Sprite_setTextSize(s_buffer, 1);
    M5Sprite_setTextDatum(s_buffer, top_left);

    /* Device battery */
    if (state->status.device_battery < 0) {
        M5Sprite_setTextColor(s_buffer, TFT_RED, TFT_BLACK);
        M5Sprite_drawString(s_buffer, "DEV:ERR", 2, layout->y_battery_dev);
    } else {
        char dev_str[16];
        dh_format_battery_text(state->status.device_battery, "     Bat",
                               dev_str, sizeof(dev_str));
        uint32_t color = dh_get_battery_color(state->status.device_battery, true);
        M5Sprite_setTextColor(s_buffer, color, TFT_BLACK);
        M5Sprite_drawString(s_buffer, dev_str, 2, layout->y_battery_dev);
    }

    /* Camera battery and SD card (only when connected) */
    if (state->main_state == DJI_STATE_PAIRED ||
        state->main_state == DJI_STATE_RECORDING) {
        /* Camera battery */
        if (state->status.camera_battery > 0) {
            char cam_str[16];
            dh_format_battery_text(state->status.camera_battery, "  CAMBat",
                                   cam_str, sizeof(cam_str));
            uint32_t cam_color = dh_get_battery_color(state->status.camera_battery, false);
            M5Sprite_setTextColor(s_buffer, cam_color, TFT_BLACK);
            M5Sprite_drawString(s_buffer, cam_str, 2, layout->y_battery_cam);
        }

        /* Camera storage */
        if (state->status.sd_remaining_time > 0) {
            char sd_str[32];
            dh_format_sd_time(state->status.sd_remaining_time, sd_str, sizeof(sd_str));
            uint32_t sd_color = dh_get_storage_time_color(state->status.sd_remaining_time);
            M5Sprite_setTextColor(s_buffer, sd_color, TFT_BLACK);
            M5Sprite_drawString(s_buffer, sd_str, 2, layout->y_battery_sd);
        }
    }
}

/**
 * @brief Draw Rec Keep indicator at top-right
 * @note Displays Rec Keep mode status (ON in red, OFF in green)
 */
static void render_rec_keep_indicator(void) {
    const display_state_t *state = ui_state_get();
    int display_width = M5_display_width();

    M5Sprite_setTextSize(s_buffer, 1);
    M5Sprite_setTextDatum(s_buffer, top_right);

    if (state->status.rec_keep_enabled) {
        M5Sprite_setTextColor(s_buffer, TFT_RED, TFT_BLACK);
        M5Sprite_drawString(s_buffer, "RecKeep:ON", display_width - 2, 2);
    } else {
        M5Sprite_setTextColor(s_buffer, TFT_GREEN, TFT_BLACK);
        M5Sprite_drawString(s_buffer, "RecKeep:OFF", display_width - 2, 2);
    }
}

/**
 * @brief Draw device ID line at bottom
 * @note Displays Osmo360 device ID when connected
 */
static void render_device_id(void) {
    const display_state_t *state = ui_state_get();
    const ui_layout_t *layout = ui_get_layout();

    if (state->device_id == 0) {
        return;
    }

    char device_id_buf[16];
    dh_format_device_id(state->device_id, device_id_buf, sizeof(device_id_buf));
    char osmo_id_buf[32];
    snprintf(osmo_id_buf, sizeof(osmo_id_buf), "Osmo: %s", device_id_buf);

    M5Sprite_setTextColor(s_buffer, TFT_CYAN, TFT_BLACK);
    M5Sprite_setTextDatum(s_buffer, top_center);
    M5Sprite_setTextSize(s_buffer, 1);
    M5Sprite_drawString(s_buffer, osmo_id_buf, layout->center_x, layout->y_line3);
}

/**
 * @brief Draw recording icon
 * @param is_recording true to draw REC icon, false for STOP icon
 * @param center_x Center X coordinate
 * @param y Y coordinate
 * @note Draws red circle for REC, green square for STOP
 */
static void render_recording_icon(bool is_recording, int center_x, int y) {
    int text_x_start = center_x - 25;

    if (is_recording) {
        /* REC icon (circle) */
        int icon_radius = 6;
        M5Sprite_fillCircle(s_buffer, text_x_start, y, icon_radius, TFT_RED);

        M5Sprite_setTextColor(s_buffer, TFT_RED, TFT_BLACK);
        M5Sprite_setTextDatum(s_buffer, top_left);
        M5Sprite_setTextSize(s_buffer, 2);
        M5Sprite_drawString(s_buffer, "REC", text_x_start + icon_radius + 8, y - 8);
    } else {
        /* STOP icon (square) */
        int icon_size = 10;
        M5Sprite_fillRect(s_buffer, text_x_start, y - icon_size/2,
                          icon_size, icon_size, TFT_GREEN);

        M5Sprite_setTextColor(s_buffer, TFT_GREEN, TFT_BLACK);
        M5Sprite_setTextDatum(s_buffer, top_left);
        M5Sprite_setTextSize(s_buffer, 2);
        M5Sprite_drawString(s_buffer, "STOP", text_x_start + icon_size + 5, y - 8);
    }
}

/**
 * @brief Draw main state display
 * @note Renders the appropriate UI based on current DJI protocol state
 */
static void render_main_state(void) {
    const display_state_t *state = ui_state_get();
    const ui_layout_t *layout = ui_get_layout();

    switch (state->main_state) {
        case DJI_STATE_IDLE:
            /* PUSH TO / Pairing */
            M5Sprite_setTextColor(s_buffer, TFT_WHITE, TFT_BLACK);
            M5Sprite_setTextDatum(s_buffer, top_center);
            M5Sprite_setTextSize(s_buffer, 2);
            M5Sprite_drawString(s_buffer, "PUSH TO", layout->center_x, layout->y_line1);
            M5Sprite_drawString(s_buffer, "Pairing", layout->center_x, layout->y_line1 + 25);
            break;

        case DJI_STATE_PAIRING: {
            /* Finding Device... / device_id */
            char device_id_buf[16];
            dh_format_device_id(state->device_id, device_id_buf, sizeof(device_id_buf));

            M5Sprite_setTextColor(s_buffer, TFT_CYAN, TFT_BLACK);
            M5Sprite_setTextDatum(s_buffer, top_center);
            M5Sprite_setTextSize(s_buffer, 1);
            M5Sprite_drawString(s_buffer, "Finding Device...", layout->center_x, layout->y_line1);
            M5Sprite_setTextSize(s_buffer, 2);
            M5Sprite_drawString(s_buffer, device_id_buf, layout->center_x, layout->y_line1 + 20);
            break;
        }

        case DJI_STATE_PAIRED: {
            /* STOP icon / Press to start */
            render_recording_icon(false, layout->center_x, layout->y_line1);

            M5Sprite_setTextColor(s_buffer, TFT_WHITE, TFT_BLACK);
            M5Sprite_setTextDatum(s_buffer, top_center);
            M5Sprite_setTextSize(s_buffer, 1);
            M5Sprite_drawString(s_buffer, "Press to start", layout->center_x, layout->y_line2);
            render_device_id();
            break;
        }

        case DJI_STATE_RECORDING: {
            /* REC icon / recording time */
            render_recording_icon(true, layout->center_x, layout->y_line1);

            char time_buf[16];
            dh_format_recording_time(state->recording_time, time_buf, sizeof(time_buf));

            M5Sprite_setTextColor(s_buffer, TFT_RED, TFT_BLACK);
            M5Sprite_setTextDatum(s_buffer, top_center);
            M5Sprite_setTextSize(s_buffer, 2);
            M5Sprite_drawString(s_buffer, time_buf, layout->center_x, layout->y_line2);
            render_device_id();
            break;
        }

        case DJI_STATE_RESTARTING:
            /* Restart / Recording... */
            M5Sprite_setTextColor(s_buffer, TFT_YELLOW, TFT_BLACK);
            M5Sprite_setTextDatum(s_buffer, top_center);
            M5Sprite_setTextSize(s_buffer, 2);
            M5Sprite_drawString(s_buffer, "Restart", layout->center_x, layout->y_line1);
            M5Sprite_drawString(s_buffer, "Recording...", layout->center_x, layout->y_line1 + 25);
            break;

        case DJI_STATE_FAILED:
            /* PAIR ERR */
            M5Sprite_setTextColor(s_buffer, TFT_RED, TFT_BLACK);
            M5Sprite_setTextDatum(s_buffer, top_center);
            M5Sprite_setTextSize(s_buffer, 2);
            M5Sprite_drawString(s_buffer, "PAIR ERR", layout->center_x, layout->y_line1);
            break;

        default:
            break;
    }
}

/**
 * @brief Draw power off screen
 * @note Renders power off/reset guide, countdown, or goodbye message
 */
static void render_poweroff_screen(void) {
    const display_state_t *state = ui_state_get();
    const ui_layout_t *layout = ui_get_layout();

    if (state->poweroff.goodbye) {
        /* "BYE" message */
        M5Sprite_setTextColor(s_buffer, TFT_WHITE, TFT_BLACK);
        M5Sprite_setTextDatum(s_buffer, top_center);
        M5Sprite_setTextSize(s_buffer, 2);
        M5Sprite_drawString(s_buffer, "BYE", layout->center_x, layout->y_line1);
    } else if (state->poweroff.countdown > 0) {
        /* Countdown number (3, 2, 1) */
        char countdown[16];
        snprintf(countdown, sizeof(countdown), "%d", state->poweroff.countdown);

        M5Sprite_setTextColor(s_buffer, TFT_RED, TFT_BLACK);
        M5Sprite_setTextDatum(s_buffer, top_center);
        M5Sprite_setTextSize(s_buffer, 3);
        M5Sprite_drawString(s_buffer, countdown, layout->center_x, layout->y_line1);

        /* "Release to reset" */
        M5Sprite_setTextSize(s_buffer, 1);
        M5Sprite_setTextColor(s_buffer, TFT_WHITE, TFT_BLACK);
        M5Sprite_drawString(s_buffer, "Release to reset", layout->center_x, layout->y_line1 + 45);
    } else {
        /* "Press 3s to PWR OFF" */
        M5Sprite_setTextColor(s_buffer, TFT_YELLOW, TFT_BLACK);
        M5Sprite_setTextDatum(s_buffer, top_center);
        M5Sprite_setTextSize(s_buffer, 2);
        M5Sprite_drawString(s_buffer, "Press 3s", layout->center_x, layout->y_line1);
        M5Sprite_drawString(s_buffer, "to PWR OFF", layout->center_x, layout->y_line1 + 16);

        /* "Release to reset" */
        M5Sprite_setTextSize(s_buffer, 1);
        M5Sprite_setTextColor(s_buffer, TFT_WHITE, TFT_BLACK);
        M5Sprite_drawString(s_buffer, "Release to reset", layout->center_x, layout->y_line1 + 45);
    }
}

/* ========== Public Interface ========== */

/**
 * @brief Initialize UI renderer
 * @return true on success, false on failure
 * @note Creates sprite buffer for double buffering
 */
bool ui_renderer_init(void) {
    if (s_buffer != NULL) {
        ESP_LOGW(TAG, "Renderer already initialized");
        return true;
    }

    int width = M5_display_width();
    int height = M5_display_height();

    s_buffer = M5Sprite_create(width, height);
    if (!s_buffer) {
        ESP_LOGE(TAG, "Failed to create sprite buffer");
        return false;
    }

    ESP_LOGI(TAG, "Renderer initialized (%dx%d)", width, height);
    return true;
}

/**
 * @brief Render UI to display
 * @note Clears buffer, draws all UI elements, and pushes to display in single operation
 */
void ui_renderer_render(void) {
    if (!s_buffer) {
        ESP_LOGW(TAG, "Renderer not initialized");
        return;
    }

    const display_state_t *state = ui_state_get();

    /* Clear buffer to black */
    M5Sprite_fillScreen(s_buffer, TFT_BLACK);

    /* Check if power off screen is active */
    if (state->poweroff.active) {
        render_poweroff_screen();
    } else {
        /* Draw main state display */
        render_main_state();

        /* Draw status indicators */
        render_gps_status();
        render_battery_indicators();
        render_rec_keep_indicator();
    }

    /* Push buffer to display in single operation */
    M5Sprite_push(s_buffer, 0, 0);
}

/**
 * @brief Clean up UI renderer
 * @note Releases sprite buffer memory
 */
void ui_renderer_cleanup(void) {
    if (s_buffer) {
        M5Sprite_destroy(s_buffer);
        s_buffer = NULL;
        ESP_LOGI(TAG, "Renderer cleaned up");
    }
}

#ifdef __cplusplus
}
#endif
