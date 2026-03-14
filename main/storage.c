/* SPDX-License-Identifier: MIT */
/*
 * NVS Storage Module for DJI Osmo360 Pairing Info
 * DJI Osmo360ペアリング情報用NVSストレージモジュール
 */

#include "storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "STORAGE";

/* NVS Namespace and keys */
/* NVS名前空間とキー */
#define STORAGE_NAMESPACE "osmo_remote"
#define KEY_MAC_ADDR "mac_addr"
#define MAC_ADDR_SIZE 6

esp_err_t storage_init(void) {
    ESP_LOGI(TAG, "Initializing storage...");

    /* NVS is already initialized in ble_init(), just verify namespace exists */
    /* NVSはble_init()で既に初期化済み、名前空間の存在確認のみ */

    return ESP_OK;
}

esp_err_t storage_save_paired_device(const uint8_t *mac_addr) {
    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "MAC address is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(nvs_handle, KEY_MAC_ADDR, mac_addr, MAC_ADDR_SIZE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save MAC address: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Saved paired device MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5]);
    return ESP_OK;
}

esp_err_t storage_get_paired_device(uint8_t *mac_addr, bool *is_found) {
    if (mac_addr == NULL || is_found == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }

    *is_found = false;
    memset(mac_addr, 0, MAC_ADDR_SIZE);

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        /* Namespace doesn't exist yet, no paired device */
        /* 名前空間が存在しない、ペアリング済みデバイスなし */
        return ESP_OK;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size = MAC_ADDR_SIZE;
    err = nvs_get_blob(nvs_handle, KEY_MAC_ADDR, mac_addr, &required_size);

    if (err == ESP_OK) {
        *is_found = true;
        ESP_LOGI(TAG, "Found paired device MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac_addr[0], mac_addr[1], mac_addr[2],
                 mac_addr[3], mac_addr[4], mac_addr[5]);
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        /* No paired device saved */
        /* ペアリング済みデバイスなし */
        ESP_LOGI(TAG, "No paired device found");
        err = ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to read MAC address: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

bool storage_is_paired(void) {
    uint8_t mac_addr[MAC_ADDR_SIZE];
    bool is_found = false;

    if (storage_get_paired_device(mac_addr, &is_found) == ESP_OK) {
        return is_found;
    }
    return false;
}

esp_err_t storage_clear_paired_device(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_erase_key(nvs_handle, KEY_MAC_ADDR);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        /* Key doesn't exist, consider as success */
        /* キーが存在しない、成功とみなす */
        err = ESP_OK;
    }

    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Cleared paired device information");
    } else {
        ESP_LOGE(TAG, "Failed to clear paired device: %s", esp_err_to_name(err));
    }

    return err;
}
