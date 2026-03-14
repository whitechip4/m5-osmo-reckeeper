/* SPDX-License-Identifier: MIT */
/*
 * M5StickC Plus2 DJI Osmo360 BLE Remote
 * M5Unified C++ Wrapper for C Interface
 *
 * Provides C interface to M5Unified C++ API
 */

#include <M5Unified.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"

/* Button state tracking */
/* ボタン状態追跡 */
static bool last_btna_state = false;
static bool last_btnb_state = false;

/* C wrapper functions */
/* Cラッパー関数 */
extern "C" {

void M5_begin(void) {
    ::M5.begin();
    /* Set display rotation to landscape (right 90 degrees) */
    /* 画面向きを横向き（右に90度回転）に設定 */
    ::M5.Display.setRotation(3);
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

} /* extern "C" */
