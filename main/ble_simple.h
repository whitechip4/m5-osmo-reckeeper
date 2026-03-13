/* SPDX-License-Identifier: MIT */
/*
 * M5StickC Plus2 DJI Osmo360 BLE Remote
 * Development Step 2: BLE Connection (Simplified)
 */

#ifndef __BLE_SIMPLE_H__
#define __BLE_SIMPLE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "esp_err.h"
#include "esp_gatt_defs.h"
#include "esp_gattc_api.h"

/* BLE State Machine */
/* BLE 状態マシン */
typedef enum {
    BLE_STATE_IDLE = 0,       /* Ready to scan / スキャン準備完了 */
    BLE_STATE_SCANNING,       /* Scanning for DJI device / DJIデバイススキャン中 */
    BLE_STATE_CONNECTING,     /* Establishing connection / 接続中 */
    BLE_STATE_CONNECTED,      /* Connected / 接続完了 */
    BLE_STATE_FAILED          /* Connection failed / 接続失敗 */
} ble_state_t;

/* Connection status structure */
/* 接続状態構造体 */
typedef struct {
    bool is_connected;         /* Connection status / 接続状態 */
    uint16_t conn_id;          /* Connection ID / 接続ID */
    esp_gatt_if_t gattc_if;    /* GATT client interface / GATTクライアントIF */
    uint16_t notify_handle;    /* Notify characteristic handle / 通知キャラクタリスティックハンドル */
    uint16_t write_handle;     /* Write characteristic handle / 書き込みキャラクタリスティックハンドル */
    esp_bd_addr_t remote_bda;  /* Remote device address / 接続先MACアドレス */
} ble_connection_t;

/**
 * @brief Initialize BLE stack
 * BLEスタックの初期化
 *
 * @return esp_err_t
 *         - ESP_OK on success
 *         - Others on failure
 */
esp_err_t ble_init(void);

/**
 * @brief Start scanning and connect to DJI Osmo360
 * DJI Osmo360のスキャンと接続を開始
 *
 * @return esp_err_t
 *         - ESP_OK on success
 *         - Others on failure
 */
esp_err_t ble_start_scanning_and_connect(void);

/**
 * @brief Disconnect if connected
 * 接続中の場合、切断する
 *
 * @return esp_err_t
 */
esp_err_t ble_disconnect(void);

/**
 * @brief Get current BLE state
 * 現在のBLE状態を取得
 *
 * @return Current BLE state
 */
ble_state_t ble_get_state(void);

/**
 * @brief Register callback for BLE state changes
 * BLE状態変化時のコールバックを登録
 *
 * @param callback Function to call when state changes
 */
void ble_set_state_callback(void (*callback)(ble_state_t new_state));

/**
 * @brief Get connection information
 * 接続情報を取得
 *
 * @return Pointer to connection structure, NULL if not connected
 */
const ble_connection_t* ble_get_connection(void);

/**
 * @brief Notification callback type
 * 通知コールバック型
 */
typedef void (*ble_notify_callback_t)(const uint8_t *data, uint16_t length);

/**
 * @brief Register callback for BLE notifications
 * BLE通知コールバック登録
 *
 * @param callback Function to call when notification received
 */
void ble_set_notify_callback(ble_notify_callback_t callback);

/**
 * @brief Send data via BLE write characteristic
 * BLE書き込みキャラクタリスティック経由でデータ送信
 *
 * @param data Data to send
 * @param length Data length
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_write(const uint8_t *data, uint16_t length);

#endif /* __BLE_SIMPLE_H__ */
