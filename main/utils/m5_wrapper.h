/* SPDX-License-Identifier: MIT */
/*
 * M5StickC Plus2 DJI Osmo360 BLE Remote
 * M5Unified C++ Wrapper for C Interface
 *
 * Provides C interface to M5Unified C++ API
 */

#ifndef M5_WRAPPER_H
#define M5_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Basic functions */
/* 基本関数 */
void M5_begin(void);
void M5_update(void);

/* Display functions */
/* ディスプレイ関数 */
int M5_display_width(void);
int M5_display_height(void);
void M5_display_fillScreen(int color);
void M5_display_setTextColor(int textcolor, int textbgcolor);
void M5Display_setTextDatum(int datum);
void M5_display_setTextSize(float size);
void M5_display_drawString(const char *string, int x, int y);

/* Graphics drawing functions */
/* グラフィック描画関数 */
void M5_display_fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void M5_display_fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color);

/* Button functions */
/* ボタン関数 */
int M5_BtnA_wasPressed(void);
int M5_BtnB_wasPressed(void);

/* PWR button functions */
/* PWRボタン関数 */
int M5_BtnPWR_wasPressed(void);
int M5_BtnPWR_isPressed(void);

/* Power management functions */
/* 電源管理関数 */
void M5_Power_restart(void);
void M5_Power_off(void);
int M5_Power_getBatteryLevel(void);

/* Sprite buffer (double buffering) */
/* スプライトバッファ（ダブルバッファ） */

/* Opaque pointer to sprite buffer */
/* スプライトバッファへの不透明ポインタ */
typedef struct sprite_buffer sprite_buffer_t;

/* Create/Destroy sprite buffer */
/* スプライトバッファの作成/破棄 */
sprite_buffer_t* M5Sprite_create(int32_t width, int32_t height);
void M5Sprite_destroy(sprite_buffer_t* sprite);

/* Drawing functions (mirroring M5_display_* API) */
/* 描画関数（M5_display_* APIのミラー） */
void M5Sprite_fillScreen(sprite_buffer_t* sprite, uint32_t color);
void M5Sprite_setTextColor(sprite_buffer_t* sprite, uint32_t textcolor, uint32_t textbgcolor);
void M5Sprite_setTextDatum(sprite_buffer_t* sprite, int datum);
void M5Sprite_setTextSize(sprite_buffer_t* sprite, float size);
void M5Sprite_drawString(sprite_buffer_t* sprite, const char *string, int x, int y);
void M5Sprite_fillRect(sprite_buffer_t* sprite, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void M5Sprite_fillCircle(sprite_buffer_t* sprite, int32_t x, int32_t y, int32_t r, uint32_t color);

/* Push buffer to display (atomic update) */
/* バッファをディスプレイに転送（アトミック更新） */
void M5Sprite_push(sprite_buffer_t* sprite, int32_t x, int32_t y);

#ifdef __cplusplus
}
#endif

#endif /* M5_WRAPPER_H */
