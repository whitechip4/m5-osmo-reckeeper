/* SPDX-License-Identifier: MIT */
/*
 * M5StickC Plus2 DJI Osmo360 BLE Remote
 * M5Unified C++ Wrapper for C Interface
 *
 * Provides C interface to M5Unified C++ API
 */

#include "m5_wrapper.h"
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

void M5_begin(void) {
    ::M5.begin();
    /* Set display rotation to landscape (right 90 degrees) */
    /* 画面向きを横向き（右に90度回転）に設定 */
    ::M5.Display.setRotation(3);

    /* Disable IMU to save power (not used in this project) */
    /* IMUを無効化して省電力（本プロジェクトでは使用しない） */
    ::M5.Imu.sleep();
}

void M5_update(void) {
    ::M5.update();
}

int M5_display_width(void) {
    return ::M5.Display.width();
}

int M5_display_height(void) {
    return ::M5.Display.height();
}

void M5_display_fillScreen(int color) {
    ::M5.Display.fillScreen((uint32_t)color);
}

void M5_display_setTextColor(int textcolor, int textbgcolor) {
    ::M5.Display.setTextColor((uint32_t)textcolor, (uint32_t)textbgcolor);
}

void M5Display_setTextDatum(int datum) {
    ::M5.Display.setTextDatum((textdatum_t)datum);
}

void M5_display_setTextSize(float size) {
    ::M5.Display.setTextSize(size);
}

void M5_display_drawString(const char *string, int x, int y) {
    ::M5.Display.drawString(string, x, y);
}

/* Graphics drawing functions */
/* グラフィック描画関数 */
void M5_display_fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    ::M5.Display.fillRect(x, y, w, h, color);
}

void M5_display_fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color) {
    ::M5.Display.fillCircle(x, y, r, color);
}

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
/* PWRボタン検出 */
static bool last_btnpwr_state = false;

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

int M5_BtnPWR_isPressed(void) {
    return ::M5.BtnPWR.isPressed() ? 1 : 0;
}

/* Power management */
/* 電源管理 */
void M5_Power_restart(void) {
    esp_restart();
}

void M5_Power_off(void) {
    ::M5.Power.powerOff();
}

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

/* Create sprite buffer */
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

/* Destroy sprite buffer */
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

/* Fill sprite with color */
void M5Sprite_fillScreen(sprite_buffer_t* sprite, uint32_t color) {
    if (sprite && sprite->sprite) {
        sprite->sprite->fillScreen(color);
    }
}

/* Set text color */
void M5Sprite_setTextColor(sprite_buffer_t* sprite, uint32_t textcolor, uint32_t textbgcolor) {
    if (sprite && sprite->sprite) {
        sprite->sprite->setTextColor(textcolor, textbgcolor);
    }
}

/* Set text datum */
void M5Sprite_setTextDatum(sprite_buffer_t* sprite, int datum) {
    if (sprite && sprite->sprite) {
        sprite->sprite->setTextDatum((textdatum_t)datum);
    }
}

/* Set text size */
void M5Sprite_setTextSize(sprite_buffer_t* sprite, float size) {
    if (sprite && sprite->sprite) {
        sprite->sprite->setTextSize(size);
    }
}

/* Draw string */
void M5Sprite_drawString(sprite_buffer_t* sprite, const char *string, int x, int y) {
    if (sprite && sprite->sprite) {
        sprite->sprite->drawString(string, x, y);
    }
}

/* Fill rectangle */
void M5Sprite_fillRect(sprite_buffer_t* sprite, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if (sprite && sprite->sprite) {
        sprite->sprite->fillRect(x, y, w, h, color);
    }
}

/* Fill circle */
void M5Sprite_fillCircle(sprite_buffer_t* sprite, int32_t x, int32_t y, int32_t r, uint32_t color) {
    if (sprite && sprite->sprite) {
        sprite->sprite->fillCircle(x, y, r, color);
    }
}

/* Push sprite to display (atomic update) */
void M5Sprite_push(sprite_buffer_t* sprite, int32_t x, int32_t y) {
    if (sprite && sprite->sprite) {
        sprite->sprite->pushSprite(&::M5.Display, x, y);
    }
}

} /* extern "C" */
