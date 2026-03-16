/* SPDX-License-Identifier: MIT */
/*
 * M5StickC Plus2 DJI Osmo360 BLE Remote
 * M5Unified C++ Wrapper for C Interface
 *
 * Provides C interface to M5Unified C++ API
 */

#include "utils/m5_wrapper.h"
#include <M5Unified.h>
#include <lgfx/v1/LGFX_Sprite.hpp>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

/* Button state tracking */
/* ボタン状態追跡 */
static bool last_btna_state = false;
static bool last_btnb_state = false;

static const char *TAG = "M5Wrapper";

/* C wrapper functions */
/* Cラッパー関数 */
extern "C" {

/**
 * @brief Initialize M5StickC Plus2
 * @note Initializes M5Unified, sets display rotation to landscape (90° right), disables IMU for power saving
 */
void M5_begin(void) {
    ::M5.begin();
    /* Set display rotation to landscape (right 90 degrees) */
    /* 画面向きを横向き（右に90度回転）に設定 */
    ::M5.Display.setRotation(3);

    /* Disable IMU to save power (not used in this project) */
    /* IMUを無効化して省電力（本プロジェクトでは使用しない） */
    ::M5.Imu.sleep();
}

/**
 * @brief Update button states
 * @note Must be called periodically to detect button presses
 */
void M5_update(void) {
    ::M5.update();
}

/**
 * @brief Get display width
 * @return Display width in pixels
 */
int M5_display_width(void) {
    return ::M5.Display.width();
}

/**
 * @brief Get display height
 * @return Display height in pixels
 */
int M5_display_height(void) {
    return ::M5.Display.height();
}

/**
 * @brief Fill entire display with color
 * @param color Color value in RGB888 format
 */
void M5_display_fillScreen(int color) {
    ::M5.Display.fillScreen((uint32_t)color);
}

/**
 * @brief Set text color
 * @param textcolor Text color in RGB888 format
 * @param textbgcolor Background color in RGB888 format
 */
void M5_display_setTextColor(int textcolor, int textbgcolor) {
    ::M5.Display.setTextColor((uint32_t)textcolor, (uint32_t)textbgcolor);
}

/**
 * @brief Set text datum (alignment reference point)
 * @param datum Text datum value (0=top_left, 1=top_center, 2=top_right, etc.)
 */
void M5Display_setTextDatum(int datum) {
    ::M5.Display.setTextDatum((textdatum_t)datum);
}

/**
 * @brief Set text size
 * @param size Text size multiplier
 */
void M5_display_setTextSize(float size) {
    ::M5.Display.setTextSize(size);
}

/**
 * @brief Draw string at specified position
 * @param string String to draw
 * @param x X coordinate
 * @param y Y coordinate
 */
void M5_display_drawString(const char *string, int x, int y) {
    ::M5.Display.drawString(string, x, y);
}

/**
 * @brief Draw filled rectangle
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width in pixels
 * @param h Height in pixels
 * @param color Color in RGB888 format
 */
void M5_display_fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    ::M5.Display.fillRect(x, y, w, h, color);
}

/**
 * @brief Draw filled circle
 * @param x Center X coordinate
 * @param y Center Y coordinate
 * @param r Radius in pixels
 * @param color Color in RGB888 format
 */
void M5_display_fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color) {
    ::M5.Display.fillCircle(x, y, r, color);
}

/**
 * @brief Check if button A was just pressed (rising edge)
 * @return 1 if button was just pressed, 0 otherwise
 * @note Detects rising edge (false -> true), requires M5_update() to be called periodically
 */
int M5_BtnA_wasPressed(void) {
    bool current_state = ::M5.BtnA.isPressed();

    /* Detect rising edge (false -> true) */
    /* 立ち上がりエッジ検出 */
    if (!last_btna_state && current_state) {
        last_btna_state = current_state;
        return 1;
    }

    last_btna_state = current_state;
    return 0;
}

/**
 * @brief Check if button B was just pressed (rising edge)
 * @return 1 if button was just pressed, 0 otherwise
 * @note Detects rising edge (false -> true), requires M5_update() to be called periodically
 */
int M5_BtnB_wasPressed(void) {
    bool current_state = ::M5.BtnB.isPressed();

    /* Detect rising edge (false -> true) */
    /* 立ち上がりエッジ検出 */
    if (!last_btnb_state && current_state) {
        last_btnb_state = current_state;
        return 1;
    }

    last_btnb_state = current_state;
    return 0;
}

/* PWR button detection */
static bool last_btnpwr_state = false;

/**
 * @brief Check if PWR button was just pressed (rising edge)
 * @return 1 if button was just pressed, 0 otherwise
 * @note Detects rising edge (false -> true), requires M5_update() to be called periodically
 */
int M5_BtnPWR_wasPressed(void) {
    bool current_state = ::M5.BtnPWR.isPressed();

    /* Detect rising edge (false -> true) */
    /* 立ち上がりエッジ検出 */
    if (!last_btnpwr_state && current_state) {
        last_btnpwr_state = current_state;
        return 1;
    }

    last_btnpwr_state = current_state;
    return 0;
}

/**
 * @brief Check if PWR button is currently pressed
 * @return 1 if button is pressed, 0 otherwise
 */
int M5_BtnPWR_isPressed(void) {
    return ::M5.BtnPWR.isPressed() ? 1 : 0;
}

/**
 * @brief Restart the device
 * @note Performs a system restart (ESP32 restart)
 */
void M5_Power_restart(void) {
    esp_restart();
}

/**
 * @brief Power off the device
 * @note Performs a clean power off
 */
void M5_Power_off(void) {
    ::M5.Power.powerOff();
}

/**
 * @brief Get device battery level
 * @return Battery level percentage (0-100)
 */
int M5_Power_getBatteryLevel(void) {
    return (int)::M5.Power.getBatteryLevel();
}

/* ========== Sprite Buffer (Double Buffering) ========== */
/* スプライトバッファ（ダブルバッファ） */

/* Sprite buffer structure (opaque to C) */
/* スプライトバッファ構造体（Cからは不透明） */
struct sprite_buffer {
    LGFX_Sprite *sprite;
};

/**
 * @brief Create sprite buffer for double buffering
 * @param width Buffer width in pixels
 * @param height Buffer height in pixels
 * @return Pointer to sprite buffer, NULL on failure
 * @note Creates off-screen sprite buffer for flicker-free rendering
 */
sprite_buffer_t* M5Sprite_create(int32_t width, int32_t height) {
    sprite_buffer_t *buffer = (sprite_buffer_t *)malloc(sizeof(sprite_buffer_t));
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate sprite buffer structure");
        return nullptr;
    }

    buffer->sprite = new LGFX_Sprite();
    if (!buffer->sprite) {
        ESP_LOGE(TAG, "Failed to create LGFX_Sprite");
        free(buffer);
        return nullptr;
    }

    /* Create sprite in internal memory (M5StickC Plus2 has no PSRAM) */
    /* 内部メモリにスプライトを作成（M5StickC Plus2はPSRAMなし） */
    if (!buffer->sprite->createSprite(width, height)) {
        ESP_LOGE(TAG, "Failed to create sprite %dx%d", width, height);
        delete buffer->sprite;
        free(buffer);
        return nullptr;
    }

    ESP_LOGI(TAG, "Sprite buffer created %dx%d (approx %d KB)",
             width, height, (width * height * 2) / 1024);
    return buffer;
}

/**
 * @brief Destroy sprite buffer
 * @param sprite Pointer to sprite buffer (safe to pass NULL)
 */
void M5Sprite_destroy(sprite_buffer_t* sprite) {
    if (!sprite) {
        return;
    }

    if (sprite->sprite) {
        sprite->sprite->deleteSprite();
        delete sprite->sprite;
    }

    free(sprite);
    ESP_LOGI(TAG, "Sprite buffer destroyed");
}

/**
 * @brief Fill sprite buffer with color
 * @param sprite Pointer to sprite buffer
 * @param color Color in RGB888 format
 */
void M5Sprite_fillScreen(sprite_buffer_t* sprite, uint32_t color) {
    if (sprite && sprite->sprite) {
        sprite->sprite->fillScreen(color);
    }
}

/**
 * @brief Set sprite text color
 *
 * @param sprite Pointer to sprite buffer
 * @param textcolor Text color in RGB888 format
 * @param textbgcolor Background color in RGB888 format
 */
void M5Sprite_setTextColor(sprite_buffer_t* sprite, uint32_t textcolor, uint32_t textbgcolor) {
    if (sprite && sprite->sprite) {
        sprite->sprite->setTextColor(textcolor, textbgcolor);
    }
}

/**
 * @brief Set sprite text datum (alignment reference point)
 *
 * @param sprite Pointer to sprite buffer
 * @param datum Text datum value (0=top_left, 1=top_center, 2=top_right, etc.)
 */
void M5Sprite_setTextDatum(sprite_buffer_t* sprite, int datum) {
    if (sprite && sprite->sprite) {
        sprite->sprite->setTextDatum((textdatum_t)datum);
    }
}

/**
 * @brief Set sprite text size
 *
 * @param sprite Pointer to sprite buffer
 * @param size Text size multiplier
 */
void M5Sprite_setTextSize(sprite_buffer_t* sprite, float size) {
    if (sprite && sprite->sprite) {
        sprite->sprite->setTextSize(size);
    }
}

/**
 * @brief Draw string on sprite buffer
 *
 * @param sprite Pointer to sprite buffer
 * @param string String to draw
 * @param x X coordinate
 * @param y Y coordinate
 */
void M5Sprite_drawString(sprite_buffer_t* sprite, const char *string, int x, int y) {
    if (sprite && sprite->sprite) {
        sprite->sprite->drawString(string, x, y);
    }
}

/**
 * @brief Draw filled rectangle on sprite buffer
 *
 * @param sprite Pointer to sprite buffer
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width in pixels
 * @param h Height in pixels
 * @param color Color in RGB888 format
 */
void M5Sprite_fillRect(sprite_buffer_t* sprite, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if (sprite && sprite->sprite) {
        sprite->sprite->fillRect(x, y, w, h, color);
    }
}

/**
 * @brief Draw filled circle on sprite buffer
 *
 * @param sprite Pointer to sprite buffer
 * @param x Center X coordinate
 * @param y Center Y coordinate
 * @param r Radius in pixels
 * @param color Color in RGB888 format
 */
void M5Sprite_fillCircle(sprite_buffer_t* sprite, int32_t x, int32_t y, int32_t r, uint32_t color) {
    if (sprite && sprite->sprite) {
        sprite->sprite->fillCircle(x, y, r, color);
    }
}

/**
 * @brief Push sprite buffer to display (atomic update)
 *
 * Copies the entire sprite buffer to the display in a single operation.
 *
 * @param sprite Pointer to sprite buffer
 * @param x Destination X coordinate
 * @param y Destination Y coordinate
 */
void M5Sprite_push(sprite_buffer_t* sprite, int32_t x, int32_t y) {
    if (sprite && sprite->sprite) {
        sprite->sprite->pushSprite(&::M5.Display, x, y);
    }
}

} /* extern "C" */
