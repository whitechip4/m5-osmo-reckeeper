/* SPDX-License-Identifier: MIT */
/*
 * M5StickC Plus2 DJI Osmo360 BLE Remote
 * M5Unified C++ Wrapper for C Interface
 *
 * Provides C interface to M5Unified C++ API
 */

#include <M5Unified.h>
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

} /* extern "C" */
