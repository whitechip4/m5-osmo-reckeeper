/* SPDX-License-Identifier: MIT */
/*
 * NVS Storage Module for DJI Osmo360 Pairing Info
 * DJI Osmo360ペアリング情報用NVSストレージモジュール
 */

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize storage module
 *        ストレージモジュール初期化
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_init(void);

/**
 * @brief Save paired device MAC address
 *        ペアリング済みデバイスのMACアドレス保存
 *
 * @param mac_addr 6-byte MAC address
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_save_paired_device(const uint8_t *mac_addr);

/**
 * @brief Get paired device MAC address
 *        ペアリング済みデバイスのMACアドレス取得
 *
 * @param mac_addr Output buffer for 6-byte MAC address
 * @param is_found Output flag indicating if paired device was found
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_get_paired_device(uint8_t *mac_addr, bool *is_found);

/**
 * @brief Check if device is paired
 *        デバイスがペアリング済みかチェック
 *
 * @return true if paired, false otherwise
 */
bool storage_is_paired(void);

/**
 * @brief Clear paired device information
 *        ペアリング済みデバイス情報をクリア
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_clear_paired_device(void);

#ifdef __cplusplus
}
#endif

#endif /* __STORAGE_H__ */
