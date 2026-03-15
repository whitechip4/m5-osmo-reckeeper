/* SPDX-License-Identifier: MIT */
/*
 * M5StickC Plus2 DJI Osmo360 BLE Remote
 * GPS Module Header
 */

#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/* GPS status definitions */
/* GPS状態定義 */
typedef enum {
    GPS_STATUS_NOGPS = 0,  /* No GPS module connected / GPSモジュール未接続 */
    GPS_STATUS_STANDBY = 1, /* NMEA data not received yet / NMEAデータ未受信 */
    GPS_STATUS_SEARCHING = 2, /* Acquiring satellites / 衛星探索中 */
    GPS_STATUS_OK = 3      /* Signal stable / 受信安定 */
} gps_status_t;

/* GPS data structure */
/* GPSデータ構造体 */
typedef struct {
    uint8_t year, month, day, hour, minute;
    double second;
    double latitude;
    char lat_indicator;  /* N/S */
    double longitude;
    char lon_indicator;  /* E/W */
    double speed_knots;
    double course;
    double altitude;
    uint8_t num_satellites;
    double velocity_north, velocity_east, velocity_descend;
    bool rmc_valid, gga_valid;
    /* status field removed - using satellite count only */
    /* statusフィールド削除 - 衛星数のみを使用 */
} gps_data_t;

/**
 * @brief Initialize GPS module
 *        GPSモジュール初期化
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t gps_init(void);

/**
 * @brief Poll GPS data (non-blocking)
 *        GPSデータポーリング（非ブロッキング）
 *
 * @return ESP_OK on success (always returns ESP_OK in polling mode)
 */
esp_err_t gps_poll(void);

/**
 * @brief Get current GPS data
 *        GPSデータ取得
 *
 * @param out_data Pointer to store GPS data
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out_data is NULL
 */
esp_err_t gps_get_data(gps_data_t *out_data);

/**
 * @brief Get GPS connection status
 *        GPS接続状態取得
 *
 * @return Current GPS status
 */
gps_status_t gps_get_status(void);

/**
 * @brief Check if GPS signal is found
 *        GPS信号検出確認
 *
 * @return true if GPS signal is found, false otherwise
 */
bool gps_is_found(void);

#ifdef __cplusplus
}
#endif

#endif /* GPS_MODULE_H */
