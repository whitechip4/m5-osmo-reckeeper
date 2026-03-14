/* SPDX-License-Identifier: MIT */
/*
 * M5StickC Plus2 DJI Osmo360 BLE Remote
 * Development Step 2: BLE Connection Test
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

/* M5Unified is C++, but we can use it from C with extern "C" linkage */
/* M5UnifiedはC++ですが、extern "C"リンケージでCから使用可能 */
#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration for M5Unified C interface */
/* M5Unified Cインターフェースの前方宣言 */
struct M5UnifiedImpl;
extern struct M5UnifiedImpl M5;

/* M5Unified C++ API wrapper functions */
/* M5Unified C++ API ラッパー関数 */
void M5_begin(void);
void M5_update(void);
int M5_display_width(void);
int M5_display_height(void);
void M5_display_fillScreen(int color);
void M5_display_setTextColor(int textcolor, int textbgcolor);
void M5Display_setTextDatum(int datum);
void M5_display_setTextSize(float size);
void M5_display_drawString(const char *string, int x, int y);
void M5_display_fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void M5_display_fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color);

/* PWR button functions */
/* PWRボタン関連関数 */
int M5_BtnPWR_wasPressed(void);
int M5_BtnPWR_isPressed(void);
void M5_Power_restart(void);
void M5_Power_off(void);

#ifdef __cplusplus
}
#endif

#include "ble_simple.h"
#include "dji_protocol.h"
#include "storage.h"

static const char *TAG = "main";

/* Color definitions from M5GFX (RGB888 format) */
/* M5GFXからの色定義（RGB888形式） */
#define TFT_BLACK       0x000000  /*   0,   0,   0 */
#define TFT_WHITE       0xFFFFFF  /* 255, 255, 255 */
#define TFT_RED         0xFF0000  /* 255,   0,   0 */
#define TFT_GREEN       0x00FF00  /*   0, 255,   0 */
#define TFT_BLUE        0x0000FF  /*   0,   0, 255 */
#define TFT_YELLOW      0xFFFF00  /* 255, 255,   0 */
#define TFT_CYAN        0x00FFFF  /*   0, 255, 255 */
#define TFT_MAGENTA     0xFF00FF  /* 255,   0, 255 */

/* Text datum constants */
/* テキスト基準点定数 */
#define top_left 0
#define top_center 1
#define top_right 2

/* BLE state display strings and colors */
/* BLE状態表示文字列と色 */
typedef struct {
    const char *text;
    uint32_t color;
} state_display_t;

static const state_display_t ble_state_display[] = {
    [BLE_STATE_IDLE]       = {"PUSH TO Pairing", TFT_WHITE},
    [BLE_STATE_SCANNING]   = {"SCAN...", TFT_YELLOW},
    [BLE_STATE_CONNECTING] = {"CONNECTING...", TFT_CYAN},
    [BLE_STATE_CONNECTED]  = {"CONNECTED", TFT_GREEN},
    [BLE_STATE_FAILED]     = {"ERROR", TFT_RED}
};

/* DJI state display strings and colors */
/* DJI状態表示文字列と色 */
static const state_display_t dji_state_display[] = {
    [DJI_STATE_IDLE]       = {"IDLE", TFT_WHITE},
    [DJI_STATE_PAIRING]    = {"PAIRING", TFT_CYAN},
    [DJI_STATE_PAIRED]     = {"PAIRED", TFT_CYAN},
    [DJI_STATE_RECORDING]  = {"REC", TFT_RED},
    [DJI_STATE_RESTARTING] = {"RESTARTING", TFT_YELLOW},
    [DJI_STATE_FAILED]     = {"PAIR ERR", TFT_RED}
};

/* Multiline display function */
/* マルチライン表示関数 */
static void display_multiline(const char *line1, const char *line2,
                              uint32_t color1, uint32_t color2,
                              float size1, float size2) {
    M5_display_fillScreen(TFT_BLACK);
    int center_x = M5_display_width() / 2;
    int y1 = M5_display_height() / 2 - 15;
    int y2 = y1 + 25;

    if (line1) {
        M5_display_setTextColor(color1, TFT_BLACK);
        M5Display_setTextDatum(top_center);
        M5_display_setTextSize(size1);
        M5_display_drawString(line1, center_x, y1);
    }

    if (line2) {
        M5_display_setTextColor(color2, TFT_BLACK);
        M5Display_setTextDatum(top_center);
        M5_display_setTextSize(size2);
        M5_display_drawString(line2, center_x, y2);
    }
}

/* Recording time formatter (MM:SS) */
/* 録画時間フォーマッター (MM:SS) */
static void format_recording_time(uint16_t seconds, char *buffer, size_t size) {
    uint16_t mins = seconds / 60;
    uint16_t secs = seconds % 60;
    snprintf(buffer, size, "%02u:%02u", mins, secs);
}

/* Device ID formatter (0xFF66 -> "FF66") */
/* デバイスIDフォーマッター (0xFF66 -> "FF66") */
static void format_device_id(uint32_t device_id, char *buffer, size_t size) {
    if (device_id == 0) {
        buffer[0] = '\0';  /* Return empty string (don't display "????") / 空文字を返す（"????" を表示しない） */
    } else {
        snprintf(buffer, size, "%04lX", (unsigned long)(device_id & 0xFFFF));
    }
}

/* LCD update function */
/* LCD更新関数 */
static void update_lcd(const char *text, uint32_t color) {
    M5_display_fillScreen(TFT_BLACK);
    M5_display_setTextColor(color, TFT_BLACK);
    M5Display_setTextDatum(top_center);
    M5_display_setTextSize(2);

    int x = M5_display_width() / 2;
    int y = M5_display_height() / 2 - 10;
    M5_display_drawString(text, x, y);
}

/* BLE state change callback */
/* BLE状態変化コールバック */
static void ble_state_callback(ble_state_t new_state) {
    if (new_state < BLE_STATE_IDLE || new_state > BLE_STATE_FAILED) {
        ESP_LOGW(TAG, "Invalid BLE state: %d", new_state);
        return;
    }

    const state_display_t *display = &ble_state_display[new_state];
    ESP_LOGI(TAG, "BLE state changed: %s", display->text);

    /* Only update LCD for BLE states (not for DJI states) */
    /* BLE状態のみLCD更新 (DJI状態は別途更新) */
    dji_state_t dji_state = dji_get_state();
    if (dji_state == DJI_STATE_IDLE || dji_state == DJI_STATE_FAILED) {
        if (new_state == BLE_STATE_IDLE) {
            display_multiline("PUSH TO", "Pairing", TFT_WHITE, TFT_WHITE, 2, 2);
        } else {
            update_lcd(display->text, display->color);
        }
    }

    /* Auto-start pairing when BLE connected */
    /* BLE接続時に自動ペアリング開始 */
    if (new_state == BLE_STATE_CONNECTED) {
        ESP_LOGI(TAG, "BLE connected, starting DJI pairing...");
        bool is_first_pairing = !storage_is_paired();
        dji_start_pairing(is_first_pairing);
    }
}

/* DJI state change callback */
/* DJI状態変化コールバック */
static void dji_state_callback(dji_state_t new_state) {
    if (new_state < DJI_STATE_IDLE || new_state > DJI_STATE_FAILED) {
        ESP_LOGW(TAG, "Invalid DJI state: %d", new_state);
        return;
    }

    const state_display_t *display = &dji_state_display[new_state];
    ESP_LOGI(TAG, "DJI state changed: %s", display->text);

    /* Check if Rec Keep mode is enabled */
    bool rec_keep_enabled = dji_is_rec_keep_mode_enabled();

    /* Get recording time if available */
    uint16_t rec_time = dji_get_recording_time();
    char time_buf[16];
    format_recording_time(rec_time, time_buf, sizeof(time_buf));

    /* Get device ID if pairing */
    uint32_t device_id = dji_get_device_id();
    char device_id_buf[16];
    format_device_id(device_id, device_id_buf, sizeof(device_id_buf));

    /* Display based on state */
    switch (new_state) {
        case DJI_STATE_IDLE:
            display_multiline("PUSH TO", "Pairing", TFT_WHITE, TFT_WHITE, 2, 2);
            break;

        case DJI_STATE_PAIRING:
            display_multiline("Finding Device...", device_id_buf, TFT_CYAN, TFT_CYAN, 1, 2);
            break;

        case DJI_STATE_PAIRED:
            M5_display_fillScreen(TFT_BLACK);
            {
                int center_x = M5_display_width() / 2;
                int y_line1 = M5_display_height() / 2 - 20;  /* Adjusted 5 pixels up / 5ピクセル上に調整 */
                int y_line2 = y_line1 + 25;
                int y_line3 = y_line2 + 20;  /* Device ID display (3rd line) / デバイスID表示用（3行目） */

                /* Draw STOP icon (square) + "STOP" text */
                /* STOPアイコン（四角）+ "STOP"テキスト描画 */
                int icon_size = 10;
                int text_x_start = center_x - 25;
                M5_display_fillRect(text_x_start, y_line1 - icon_size/2,
                                    icon_size, icon_size, TFT_GREEN);

                M5_display_setTextColor(TFT_GREEN, TFT_BLACK);
                M5Display_setTextDatum(top_left);
                M5_display_setTextSize(2);
                M5_display_drawString("STOP", text_x_start + icon_size + 5, y_line1 - 8);

                /* Draw "Press to start" below */
                /* 下に "Press to start" を描画 */
                M5_display_setTextColor(TFT_WHITE, TFT_BLACK);
                M5Display_setTextDatum(top_center);
                M5_display_setTextSize(1);
                M5_display_drawString("Press to start", center_x, y_line2);

                /* Draw Osmo device ID at bottom (small) */
                /* 画面下部にOsmoデバイスIDを表示（小さく） */
                uint32_t device_id = dji_get_device_id();
                if (device_id != 0) {
                    char device_id_buf[16];
                    format_device_id(device_id, device_id_buf, sizeof(device_id_buf));
                    char osmo_id_buf[32];
                    snprintf(osmo_id_buf, sizeof(osmo_id_buf), "Osmo: %s", device_id_buf);

                    M5_display_setTextColor(TFT_CYAN, TFT_BLACK);  /* Cyan color, not too distracting / シアン色で目立たせず */
                    M5Display_setTextDatum(top_center);
                    M5_display_setTextSize(1);
                    M5_display_drawString(osmo_id_buf, center_x, y_line3);
                }
            }
            break;

        case DJI_STATE_RECORDING:
            M5_display_fillScreen(TFT_BLACK);
            {
                int center_x = M5_display_width() / 2;
                int y_line1 = M5_display_height() / 2 - 20;  /* Adjusted 5 pixels up / 5ピクセル上に調整 */
                int y_line2 = y_line1 + 25;
                int y_line3 = y_line2 + 20;  /* Device ID display (3rd line) / デバイスID表示用（3行目） */

                /* Draw REC icon (circle) + "REC" text */
                /* RECアイコン（円）+ "REC"テキスト描画 */
                int icon_radius = 6;
                int text_x_start = center_x - 25;
                M5_display_fillCircle(text_x_start, y_line1, icon_radius, TFT_RED);

                M5_display_setTextColor(TFT_RED, TFT_BLACK);
                M5Display_setTextDatum(top_left);
                M5_display_setTextSize(2);
                M5_display_drawString("REC", text_x_start + icon_radius + 8, y_line1 - 8);

                /* Draw recording time below */
                /* 下に録画時間を描画 */
                M5_display_setTextColor(TFT_RED, TFT_BLACK);
                M5Display_setTextDatum(top_center);
                M5_display_setTextSize(2);
                M5_display_drawString(time_buf, center_x, y_line2);

                /* Draw Osmo device ID at bottom (small) */
                /* 画面下部にOsmoデバイスIDを表示（小さく） */
                uint32_t device_id = dji_get_device_id();
                if (device_id != 0) {
                    char device_id_buf[16];
                    format_device_id(device_id, device_id_buf, sizeof(device_id_buf));
                    char osmo_id_buf[32];
                    snprintf(osmo_id_buf, sizeof(osmo_id_buf), "Osmo: %s", device_id_buf);

                    M5_display_setTextColor(TFT_CYAN, TFT_BLACK);  /* Cyan color, not too distracting / シアン色で目立たせず */
                    M5Display_setTextDatum(top_center);
                    M5_display_setTextSize(1);
                    M5_display_drawString(osmo_id_buf, center_x, y_line3);
                }
            }
            break;

        case DJI_STATE_RESTARTING:
            display_multiline("Restart", "Recording...", TFT_YELLOW, TFT_YELLOW, 2, 2);
            break;

        case DJI_STATE_FAILED:
            display_multiline("PAIR ERR", NULL, TFT_RED, TFT_BLACK, 2, 1);
            break;

        default:
            display_multiline("UNKNOWN", NULL, TFT_WHITE, TFT_BLACK, 2, 1);
            break;
    }

    /* Draw Rec Keep indicator in top-right corner (always displayed) */
    /* 右上にRec Keepモード状態を常時表示 */
    M5_display_setTextSize(1);
    M5Display_setTextDatum(top_right);

    if (rec_keep_enabled) {
        /* ON: Red color */
        /* ON: 赤色 */
        M5_display_setTextColor(TFT_RED, TFT_BLACK);
        M5_display_drawString("RecKeep:ON", M5_display_width() - 2, 2);
    } else {
        /* OFF: Green color */
        /* OFF: 緑色 */
        M5_display_setTextColor(TFT_GREEN, TFT_BLACK);
        M5_display_drawString("RecKeep:OFF", M5_display_width() - 2, 2);
    }
}

/* Rec Keep mode change callback */
/* Rec Keepモード変化コールバック */
static void rec_keep_mode_callback(bool enabled) {
    ESP_LOGI(TAG, "Rec Keep mode changed: %s", enabled ? "ON" : "OFF");

    /* Refresh LCD to update Rec Keep indicator */
    /* Rec Keep表示更新のためにLCDリフレッシュ */
    dji_state_t dji_state = dji_get_state();
    dji_state_callback(dji_state);
}

/* Main application entry point */
/* メインアプリケーションエントリーポイント */
void app_main(void) {
    ESP_LOGI(TAG, "Osmo360 BLE Remote Starting...");

    /* Initialize M5StickC Plus2 */
    /* M5StickC Plus2 初期化 */
    M5_begin();
    ESP_LOGI(TAG, "M5Unified initialized");

    /* Clear screen */
    /* 画面クリア */
    M5_display_fillScreen(TFT_BLACK);

    /* Initialize storage */
    /* ストレージ初期化 */
    esp_err_t ret = storage_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Storage initialization failed: %s", esp_err_to_name(ret));
        update_lcd("STG ERROR", TFT_RED);
        return;
    }
    ESP_LOGI(TAG, "Storage initialized");

    /* Initialize BLE stack */
    /* BLEスタック初期化 */
    ret = ble_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BLE initialization failed: %s", esp_err_to_name(ret));
        update_lcd("BLE ERROR", TFT_RED);
        return;
    }
    ESP_LOGI(TAG, "BLE stack initialized");

    /* Register BLE state callback */
    /* BLE状態コールバック登録 */
    ble_set_state_callback(ble_state_callback);

    /* Initialize DJI protocol */
    /* DJIプロトコル初期化 */
    ret = dji_protocol_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "DJI protocol initialization failed: %s", esp_err_to_name(ret));
        update_lcd("DJI ERROR", TFT_RED);
        return;
    }
    ESP_LOGI(TAG, "DJI protocol initialized");

    /* Register DJI state callback */
    /* DJI状態コールバック登録 */
    dji_set_state_callback(dji_state_callback);

    /* Register Rec Keep mode callback */
    /* Rec Keepモードコールバック登録 */
    extern void dji_set_rec_keep_callback(void (*callback)(bool));  /* Forward declaration */
    dji_set_rec_keep_callback(rec_keep_mode_callback);

    /* Register BLE notification callback for DJI protocol */
    /* DJIプロトコル用BLE通知コールバック登録 */
    ble_set_notify_callback(dji_handle_notification);

    /* Display initial state */
    /* 初期状態表示 */
    display_multiline("PUSH TO", "Pairing", TFT_WHITE, TFT_WHITE, 2, 2);
    ESP_LOGI(TAG, "System ready. Press BtnA to scan for DJI Osmo360.");

    /* Main event loop */
    /* メインイベントループ */
    while (1) {
        /* Update button states */
        /* ボタン状態更新 */
        M5_update();

        /* Update recording time display when recording */
        /* 録画中に録画時間表示を更新 */
        dji_state_t dji_state = dji_get_state();
        if (dji_state == DJI_STATE_RECORDING) {
            static uint16_t last_rec_time = 0xFFFF;  /* Initialize to impossible value */

            uint16_t rec_time = dji_get_recording_time();
            if (rec_time != last_rec_time) {
                char time_buf[16];
                format_recording_time(rec_time, time_buf, sizeof(time_buf));

                int center_x = M5_display_width() / 2;
                int y_line1 = M5_display_height() / 2 - 20;  /* Match same coordinate calculation / 他と同じ座標計算に修正 */
                int y_line2 = y_line1 + 25;
                int y_line3 = y_line2 + 20;

                /* Clear only the time area (bottom line) */
                /* 時間表示エリアのみクリア（下段） */
                M5_display_fillRect(0, y_line2 - 2, M5_display_width(), 20, TFT_BLACK);

                /* Redraw time */
                /* 時間を再描画 */
                M5_display_setTextColor(TFT_RED, TFT_BLACK);
                M5Display_setTextDatum(top_center);
                M5_display_setTextSize(2);
                M5_display_drawString(time_buf, center_x, y_line2);

                /* Redraw device ID (cleared by time area clear) */
                /* デバイスIDを再描画（時間エリアクリアで消えたため） */
                uint32_t device_id = dji_get_device_id();
                if (device_id != 0) {
                    char device_id_buf[16];
                    format_device_id(device_id, device_id_buf, sizeof(device_id_buf));
                    char osmo_id_buf[32];
                    snprintf(osmo_id_buf, sizeof(osmo_id_buf), "Osmo: %s", device_id_buf);

                    M5_display_setTextColor(TFT_CYAN, TFT_BLACK);
                    M5Display_setTextDatum(top_center);
                    M5_display_setTextSize(1);
                    M5_display_drawString(osmo_id_buf, center_x, y_line3);
                }

                last_rec_time = rec_time;
            }
        }

        /* Check button A press */
        /* ボタンA押下チェック */
        extern int M5_BtnA_wasPressed(void);  /* Forward declaration */
        extern int M5_BtnB_wasPressed(void);  /* Forward declaration */
        if (M5_BtnA_wasPressed()) {
            ble_state_t ble_state = ble_get_state();
            dji_state_t dji_state = dji_get_state();

            if (ble_state == BLE_STATE_IDLE) {
                /* Not connected: Start BLE connection */
                /* 未接続: BLE接続開始 */
                ESP_LOGI(TAG, "Button pressed, connecting...");
                ret = ble_connect_or_scan();
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to connect: %s", esp_err_to_name(ret));
                }
            } else if (ble_state == BLE_STATE_CONNECTED) {
                /* Check DJI state for appropriate action */
                /* DJI状態に応じて適切なアクションを実行 */

                if (dji_state == DJI_STATE_PAIRED || dji_state == DJI_STATE_RECORDING) {
                    /* Paired: Toggle recording */
                    /* ペアリング済み: 録画開始/停止 */
                    ESP_LOGI(TAG, "Button pressed, toggling recording...");
                    ret = dji_toggle_recording();
                    if (ret != ESP_OK) {
                        ESP_LOGE(TAG, "Failed to toggle recording: %s", esp_err_to_name(ret));
                    }
                } else {
                    /* Not paired: Disconnect */
                    /* 未ペアリング: 切断 */
                    ESP_LOGI(TAG, "Button pressed, disconnecting...");
                    ble_disconnect();
                }
            } else {
                ESP_LOGW(TAG, "Button pressed but not in appropriate state (BLE=%d, DJI=%d)",
                         ble_state, dji_state);
            }
        }

        /* Check button B press */
        /* ボタンB押下チェック */
        if (M5_BtnB_wasPressed()) {
            dji_state_t dji_state = dji_get_state();
            if (dji_state == DJI_STATE_PAIRED || dji_state == DJI_STATE_RECORDING) {
                /* Toggle Rec Keep mode */
                /* Rec Keepモード切り替え */
                extern bool dji_is_rec_keep_mode_enabled(void);  /* Forward declaration */
                extern esp_err_t dji_set_rec_keep_mode(bool);    /* Forward declaration */
                bool current = dji_is_rec_keep_mode_enabled();
                esp_err_t ret = dji_set_rec_keep_mode(!current);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to set Rec Keep mode: %s", esp_err_to_name(ret));
                }
            }
        }

        /* Check PWR button */
        /* PWRボタン処理 */
        if (M5_BtnPWR_wasPressed()) {
            ESP_LOGI(TAG, "PWR button pressed, starting countdown...");

            /* Show power off/reset guide */
            /* 電源OFF/リセット案内表示 */
            M5_display_fillScreen(TFT_BLACK);

            int x = M5_display_width() / 2;
            int y_line1 = M5_display_height() / 2 - 20;
            int y_line2 = y_line1 + 45;

            /* Line 1: "Press 3s" + "to PWR OFF" (2-line, yellow, large) */
            /* 1行目: "Press 3s" + "to PWR OFF" (2行、黄色、大) */
            M5_display_setTextColor(TFT_YELLOW, TFT_BLACK);
            M5Display_setTextDatum(top_center);
            M5_display_setTextSize(2);
            M5_display_drawString("Press 3s", x, y_line1);
            M5_display_drawString("to PWR OFF", x, y_line1 + 16);

            /* Line 2: "Release to reset" (1-line, white, small) */
            /* 2行目: "Release to reset" (1行、白色、小) */
            M5_display_setTextColor(TFT_WHITE, TFT_BLACK);
            M5_display_setTextSize(1);
            M5_display_drawString("Release to reset", x, y_line2);

            /* Small delay to show guide */
            /* 案内表示のための小さな遅延 */
            vTaskDelay(pdMS_TO_TICKS(500));

            /* 3 second countdown */
            /* 3秒カウントダウン */
            bool button_released = false;
            for (int i = 3; i > 0; i--) {
                /* Update display for countdown */
                /* カウントダウン表示更新 */
                M5_display_fillScreen(TFT_BLACK);

                /* Show countdown number in red */
                /* カウント数字を赤色で表示 */
                M5_display_setTextColor(TFT_RED, TFT_BLACK);
                M5Display_setTextDatum(top_center);
                M5_display_setTextSize(3);

                char countdown[8];
                snprintf(countdown, sizeof(countdown), "%d", i);
                M5_display_drawString(countdown, x, y_line1);

                /* Show "Release to reset" message below countdown */
                /* カウントダウンの下に「Release to reset」を表示 */
                M5_display_setTextSize(1);
                M5_display_setTextColor(TFT_WHITE, TFT_BLACK);
                M5_display_drawString("Release to reset", x, y_line2);

                /* Check if button is still pressed during countdown */
                /* カウントダウン中にボタンが押されているかチェック */
                for (int j = 0; j < 20; j++) {  /* 50ms * 20 = 1000ms */
                    M5_update();
                    if (!M5_BtnPWR_isPressed()) {
                        button_released = true;
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(50));
                }

                if (button_released) {
                    /* Button released: Reset device */
                    /* ボタン離し: リセット */
                    ESP_LOGI(TAG, "PWR button released, restarting device...");
                    M5_Power_restart();
                    break;  /* Not reached, but for clarity */
                }
            }

            /* Button held for 3 seconds: Power off */
            /* 3秒間押し続けた: 電源OFF */
            ESP_LOGI(TAG, "PWR button held for 3 seconds, powering off...");

            /* Show bye message */
            /* さようならメッセージ表示 */
            M5_display_fillScreen(TFT_BLACK);
            M5_display_setTextColor(TFT_WHITE, TFT_BLACK);
            M5Display_setTextDatum(top_center);
            M5_display_setTextSize(2);
            M5_display_drawString("BYE", x, y_line1);

            /* Ensure minimum display time */
            /* 最小表示時間確保 */
            vTaskDelay(pdMS_TO_TICKS(500));
            M5_Power_off();
        }

        /* Small delay to prevent watchdog trigger */
        /* ウォッチドッグトリガー防止のための小さな遅延 */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
