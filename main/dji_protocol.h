/* SPDX-License-Identifier: MIT */
/*
 * DJI Protocol High-Level API
 * DJIプロトコル高レベルAPI
 */

#ifndef DJI_PROTOCOL_H
#define DJI_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DJI protocol state machine states
 *        DJIプロトコル状態マシン状態
 */
typedef enum {
    DJI_STATE_IDLE = 0,        /* Idle / アイドル */
    DJI_STATE_PAIRING,         /* Pairing in progress / ペアリング中 */
    DJI_STATE_PAIRED,          /* Paired successfully / ペアリング完了 */
    DJI_STATE_RECORDING,       /* Camera is recording / 録画中 */
    DJI_STATE_FAILED           /* Pairing/operation failed / 失敗 */
} dji_state_t;

/**
 * @brief Command send result structure
 *        コマンド送信結果構造体
 */
typedef struct {
    void *structure;           /* Parsed response structure / 解析済みレスポンス構造体 */
    size_t structure_length;   /* Structure length / 構造体長 */
    uint16_t seq;              /* Sequence number / シーケンス番号 */
} dji_command_result_t;

/**
 * @brief Initialize DJI protocol layer
 *        DJIプロトコルレイヤー初期化
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t dji_protocol_init(void);

/**
 * @brief Start pairing with Osmo360 (blocking call, timeout 35 seconds)
 *        Osmo360とのペアリング開始（ブロッキング呼び出し、タイムアウト35秒）
 *
 * @param is_first_pairing true for first pairing (verify_mode=1), false for reconnection (verify_mode=0)
 *                         初回ペアリング時はtrue (verify_mode=1)、再接続時はfalse (verify_mode=0)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t dji_start_pairing(bool is_first_pairing);

/**
 * @brief Subscribe to camera status updates
 *        カメラ状態更新の購読開始
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t dji_subscribe_status(void);

/**
 * @brief Handle BLE notification data
 *        BLE通知データ処理
 *
 * @param data Notification data / 通知データ
 * @param length Data length / データ長
 */
void dji_handle_notification(const uint8_t *data, uint16_t length);

/**
 * @brief Get current DJI state
 *        現在のDJI状態取得
 *
 * @return Current DJI state
 */
dji_state_t dji_get_state(void);

/**
 * @brief Register callback for DJI state changes
 *        DJI状態変化コールバック登録
 *
 * @param callback Function to call on state change / 状態変化時に呼ばれる関数
 */
void dji_set_state_callback(void (*callback)(dji_state_t new_state));

/**
 * @brief Reset DJI protocol state
 *        DJIプロトコル状態リセット
 *
 * Call this when BLE connection is lost to reset state machine.
 * BLE接続が失われたときに呼び出して状態マシンをリセットする。
 */
void dji_reset_state(void);

/**
 * @brief Toggle recording state
 *        録画状態を切り替え
 *
 * Starts recording if currently stopped, stops if currently recording.
 * 録画停止中なら開始、録画中なら停止。
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t dji_toggle_recording(void);

#ifdef __cplusplus
}
#endif

#endif /* DJI_PROTOCOL_H */
