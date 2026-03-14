/* SPDX-License-Identifier: MIT */
/*
 * M5StickC Plus2 DJI Osmo360 BLE Remote
 * GPS Module Implementation
 *
 * Based on reference/OsmoDemo/logic/gps_logic.c
 * Copyright (C) 2025 SZ DJI Technology Co., Ltd.
 */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include "gps_module.h"
#include "config.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "gps_module";

/* GPS data structure */
/* GPSデータ構造体 */
static gps_data_t s_gps_data = {0};

/* Counter for consecutive invalid GPS readings */
/* GPS連続無効回数カウンター */
static uint8_t s_gps_invalid_count = 0;

/* Store previous altitude and time for velocity calculation */
/* 速度計算用の前回高度・時刻保存 */
static double s_previous_altitude = 0.0;
static double s_previous_time = 0.0;

/* UART read buffer */
/* UART読み取りバッファ */
static uint8_t s_rx_buffer[GPS_UART_RX_BUF_SIZE + 1] = {0};

/**
 * @brief Initialize GPS data structure
 *        GPSデータ構造体初期化
 */
static void init_gps_data(void) {
    s_gps_data.year = 0;
    s_gps_data.month = 0;
    s_gps_data.day = 0;
    s_gps_data.hour = 0;
    s_gps_data.minute = 0;
    s_gps_data.second = 0.0;

    s_gps_data.latitude = 0.0;
    s_gps_data.lat_indicator = 'N';
    s_gps_data.longitude = 0.0;
    s_gps_data.lon_indicator = 'E';

    s_gps_data.speed_knots = 0.0;
    s_gps_data.course = 0.0;
    s_gps_data.altitude = 0.0;
    s_gps_data.num_satellites = 0;

    s_gps_data.velocity_north = 0.0;
    s_gps_data.velocity_east = 0.0;
    s_gps_data.velocity_descend = 0.0;

    s_gps_data.rmc_valid = false;
    s_gps_data.gga_valid = false;
}

/**
 * @brief Convert NMEA format coordinates to decimal degrees
 *        NMEA形式座標を10進数度に変換
 *
 * @param nmea NMEA format coordinate string (DDMM.MMMM)
 * @param direction Direction character ('N', 'S', 'E', 'W')
 * @return Converted decimal degree value
 */
static double convert_nmea_to_degree(const char *nmea, char direction) {
    double deg = 0.0;
    double min = 0.0;

    /* Find decimal point position */
    /* 小数点の位置を検出 */
    const char *dot = nmea;
    while (*dot && *dot != '.') {
        dot++;
    }

    /* Calculate integer part */
    /* 整数部を計算 */
    int int_part = 0;
    const char *ptr = nmea;
    while (ptr < dot && isdigit((unsigned char)*ptr)) {
        int_part = int_part * 10 + (*ptr - '0');
        ptr++;
    }

    /* Calculate decimal part */
    /* 小数部を計算 */
    double frac_part = 0.0;
    double divisor = 10.0;
    ptr = dot + 1; /* Skip decimal point */
    while (*ptr && isdigit((unsigned char)*ptr)) {
        frac_part += (*ptr - '0') / divisor;
        divisor *= 10.0;
        ptr++;
    }

    /* Combine integer and decimal parts */
    /* 整数部と小数部を結合 */
    deg = int_part + frac_part;

    /* Separate degrees and minutes */
    /* 度と分を分離 */
    min = deg - ((int)(deg / 100)) * 100;
    deg = ((int)(deg / 100)) + min / 60.0;

    /* Adjust sign based on direction */
    /* 方向に基づいて符号を調整 */
    if (direction == 'S' || direction == 'W') {
        deg = -deg;
    }

    return deg;
}

/**
 * @brief Parse GNRMC sentence
 *        GNRMCセンテンスをパース
 *
 * Example: $GNRMC,074700.000,A,2234.732734,N,11356.317512,E,1.67,285.57,150125,,,A,V*03
 *
 * @param sentence Input GNRMC sentence string
 * @return true if valid RMC data found, false otherwise
 */
static bool parse_gnrmc(char *sentence) {
    /* Manual token parsing to handle empty fields correctly */
    /* 空フィールドを正しく処理するための手動トークンパース */
    char *start = sentence;
    int field = 0;

    double temp_latitude = 0.0;
    double temp_longitude = 0.0;

    /* Skip sentence type ($GNRMC) */
    /* 文タイプをスキップ ($GNRMC) */
    while (*start != ',' && *start != '\0') {
        start++;
    }
    if (*start == ',') {
        start++;
    }

    while (*start != '\0' && *start != '*') {
        char *end = strchr(start, ',');
        if (end == NULL) {
            end = strchr(start, '*');  /* Checksum delimiter / チェックサム区切り */
        }
        if (end == NULL) {
            break;  /* End of sentence / 文の終わり */
        }

        field++;
        size_t token_len = end - start;

        /* Process non-empty tokens */
        /* 空でないトークンを処理 */
        if (token_len > 0) {
            char token_buf[32] = {0};
            if (token_len < sizeof(token_buf) - 1) {
                strncpy(token_buf, start, token_len);
                token_buf[token_len] = '\0';

                switch (field) {
                    case 1: {
                        /* Time hhmmss.sss */
                        uint8_t hour = 0, minute = 0;
                        double second = 0.0;

                        if (token_len >= 6) {
                            hour = (token_buf[0] - '0') * 10 + (token_buf[1] - '0');
                            minute = (token_buf[2] - '0') * 10 + (token_buf[3] - '0');
                            second = atof(token_buf + 4);

                            s_gps_data.hour = hour;
                            s_gps_data.minute = minute;
                            s_gps_data.second = second;
                        }
                        break;
                    }
                    case 2:
                        /* Status A/V */
                        s_gps_data.rmc_valid = (token_buf[0] == 'A');
                        break;
                    case 3:
                        /* Latitude (temporary storage) */
                        temp_latitude = convert_nmea_to_degree(token_buf, 'N');
                        break;
                    case 4:
                        /* Update latitude direction */
                        s_gps_data.lat_indicator = token_buf[0];
                        s_gps_data.latitude = (s_gps_data.lat_indicator == 'S') ? -temp_latitude : temp_latitude;
                        break;
                    case 5:
                        /* Longitude (temporary storage) */
                        temp_longitude = convert_nmea_to_degree(token_buf, 'E');
                        break;
                    case 6:
                        /* Update longitude direction */
                        s_gps_data.lon_indicator = token_buf[0];
                        s_gps_data.longitude = (s_gps_data.lon_indicator == 'W') ? -temp_longitude : temp_longitude;
                        break;
                    case 7:
                        /* Ground speed (knots) */
                        s_gps_data.speed_knots = atof(token_buf);
                        break;
                    case 8:
                        /* Course (degrees) */
                        s_gps_data.course = atof(token_buf);
                        break;
                    case 9: {
                        /* Date ddmmyy */
                        uint8_t day = 0, month = 0, year = 0;

                        if (token_len >= 6) {
                            day = (token_buf[0] - '0') * 10 + (token_buf[1] - '0');
                            month = (token_buf[2] - '0') * 10 + (token_buf[3] - '0');
                            year = (token_buf[4] - '0') * 10 + (token_buf[5] - '0');

                            s_gps_data.day = day;
                            s_gps_data.month = month;
                            s_gps_data.year = year;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        start = end + 1;  /* Move to next field / 次のフィールドへ */
    }

    /* Calculate velocity components to north and east (m/s) */
    /* 北向き・東向き速度成分を計算 (m/s) */
    double speed_m_s = s_gps_data.speed_knots * 0.514444;
    s_gps_data.velocity_north = speed_m_s * cos(s_gps_data.course * M_PI / 180.0);
    s_gps_data.velocity_east = speed_m_s * sin(s_gps_data.course * M_PI / 180.0);

    return s_gps_data.rmc_valid;
}

/**
 * @brief Parse GNGGA sentence
 *        GNGGAセンテンスをパース
 *
 * Example: $GNGGA,074700.000,2234.732734,N,11356.317512,E,1,7,1.31,47.379,M,-2.657,M,,*65
 *
 * @param sentence Input GNGGA sentence string
 * @return true if valid GGA data found, false otherwise
 */
static bool parse_gngga(char *sentence) {
    /* Manual token parsing to handle empty fields correctly */
    /* 空フィールドを正しく処理するための手動トークンパース */
    char *start = sentence;
    int field = 0;

    double temp_latitude = 0.0;
    double temp_longitude = 0.0;

    /* Skip sentence type ($GNGGA) */
    /* 文タイプをスキップ ($GNGGA) */
    while (*start != ',' && *start != '\0') {
        start++;
    }
    if (*start == ',') {
        start++;
    }

    while (*start != '\0' && *start != '*') {
        char *end = strchr(start, ',');
        if (end == NULL) {
            end = strchr(start, '*');  /* Checksum delimiter / チェックサム区切り */
        }
        if (end == NULL) {
            break;  /* End of sentence / 文の終わり */
        }

        field++;
        size_t token_len = end - start;

        /* Process non-empty tokens */
        /* 空でないトークンを処理 */
        if (token_len > 0) {
            char token_buf[32] = {0};
            if (token_len < sizeof(token_buf) - 1) {
                strncpy(token_buf, start, token_len);
                token_buf[token_len] = '\0';

                switch (field) {
                    case 1:
                        /* Time hhmmss.sss - skip, already in RMC */
                        break;
                    case 2:
                        /* Latitude */
                        temp_latitude = convert_nmea_to_degree(token_buf, 'N');
                        break;
                    case 3:
                        /* N/S indicator */
                        s_gps_data.lat_indicator = token_buf[0];
                        s_gps_data.latitude = (s_gps_data.lat_indicator == 'S') ? -temp_latitude : temp_latitude;
                        break;
                    case 4:
                        /* Longitude */
                        temp_longitude = convert_nmea_to_degree(token_buf, 'E');
                        break;
                    case 5:
                        /* E/W indicator */
                        s_gps_data.lon_indicator = token_buf[0];
                        s_gps_data.longitude = (s_gps_data.lon_indicator == 'W') ? -temp_longitude : temp_longitude;
                        break;
                    case 6:
                        /* Position fix quality */
                        if (token_buf[0] != '\0') {
                            int quality = atoi(token_buf);
                            s_gps_data.gga_valid = (quality > 0);
                        }
                        break;
                    case 7:
                        /* Number of satellites in view */
                        s_gps_data.num_satellites = (uint8_t)atoi(token_buf);
                        break;
                    case 8:
                        /* HDOP */
                        break;
                    case 9:
                        /* Altitude (meters) */
                        s_gps_data.altitude = atof(token_buf);

                        /* Calculate descent velocity (needs previous altitude and time) */
                        /* 下降速度を計算（前回高度と時刻が必要） */
                        if (s_previous_time > 0.0) {
                            double current_time = s_gps_data.hour * 3600 + s_gps_data.minute * 60 + s_gps_data.second;
                            double delta_time = current_time - s_previous_time;

                            /* Handle day crossover */
                            /* 日付変更を処理 */
                            if (delta_time < -43200) {
                                delta_time += 86400;
                            } else if (delta_time > 43200) {
                                delta_time -= 86400;
                            }

                            /* Only process reasonable time differences */
                            /* 合理的な時間差のみ処理 */
                            if (delta_time > 0 && delta_time < 10) {
                                double delta_altitude = s_gps_data.altitude - s_previous_altitude;

                                /* Filter abnormal values */
                                /* 異常値をフィルタリング */
                                if (fabs(delta_altitude) < 100) {
                                    /* Note: negative for ascent, positive for descent */
                                    /* 注意：上昇が負、下降が正 */
                                    s_gps_data.velocity_descend = -delta_altitude / delta_time;
                                }
                            }
                        }
                        s_previous_altitude = s_gps_data.altitude;
                        s_previous_time = s_gps_data.hour * 3600 + s_gps_data.minute * 60 + s_gps_data.second;
                        break;
                    default:
                        break;
                }
            }
        }

        start = end + 1;  /* Move to next field / 次のフィールドへ */
    }

    return s_gps_data.gga_valid;
}

/**
 * @brief Parse all NMEA sentences in buffer
 *        バッファ内の全NMEAセンテンスをパース
 *
 * @param buffer Buffer containing NMEA sentences
 */
static void parse_nmea_buffer(char *buffer) {
    /* Note: Don't reset validity flags here. They should persist until new data arrives. */
    /* 有効フラグはここではリセットしない。新しいデータが届くまで保持する。 */

    char *start = buffer;
    char *end = NULL;

    /* Store RMC and GGA coordinates for averaging */
    /* RMCとGGA座標を保存して平均化 */
    double rmc_latitude = 0.0, rmc_longitude = 0.0;
    double gga_latitude = 0.0, gga_longitude = 0.0;

    /* Flags to track if RMC/GGA were found in this buffer */
    /* このバッファでRMC/GGAが見つかったかを追跡するフラグ */
    bool rmc_found = false;
    bool gga_found = false;

    while ((end = strchr(start, '\n')) != NULL) {
        size_t line_length = end - start;

        if (line_length > 0) {
            char line[GPS_UART_RX_BUF_SIZE] = {0};
            strncpy(line, start, line_length);
            line[line_length] = '\0';

            /* Parse the line */
            /* 該当行をパース */
            if (strncmp(line, "$GNRMC", 6) == 0 || strncmp(line, "$GPRMC", 6) == 0) {
                if (parse_gnrmc(line)) {
                    rmc_found = true;
                    rmc_latitude = s_gps_data.latitude;
                    rmc_longitude = s_gps_data.longitude;
                }
            } else if (strncmp(line, "$GNGGA", 6) == 0 || strncmp(line, "$GPGGA", 6) == 0) {
                if (parse_gngga(line)) {
                    gga_found = true;
                    gga_latitude = s_gps_data.latitude;
                    gga_longitude = s_gps_data.longitude;
                }
            }
        }

        start = end + 1;
    }

    /* Process last line (if not ending with newline) */
    /* 最終行を処理（改行で終わっていない場合） */
    if (*start != '\0') {
        if (strncmp(start, "$GNRMC", 6) == 0 || strncmp(start, "$GPRMC", 6) == 0) {
            if (parse_gnrmc(start)) {
                rmc_found = true;
                rmc_latitude = s_gps_data.latitude;
                rmc_longitude = s_gps_data.longitude;
            }
        } else if (strncmp(start, "$GNGGA", 6) == 0 || strncmp(start, "$GPGGA", 6) == 0) {
            if (parse_gngga(start)) {
                gga_found = true;
                gga_latitude = s_gps_data.latitude;
                gga_longitude = s_gps_data.longitude;
            }
        }
    }

    /* Update final status and position data after parsing all sentences */
    /* 全センテンスパース後に最終状態と位置データを更新 */
    if (rmc_found && gga_found) {
        /* Both RMC and GGA valid data found in this buffer */
        /* このバッファでRMCとGGAの両方の有効データを検出 */
        s_gps_invalid_count = 0; /* Reset counter */

        /* Calculate average */
        /* 平均値を計算 */
        s_gps_data.latitude = (rmc_latitude + gga_latitude) / 2.0;
        s_gps_data.longitude = (rmc_longitude + gga_longitude) / 2.0;
    } else if (rmc_found || gga_found) {
        /* Only one type of data found - don't change invalid count */
        /* 一方のデータのみ検出 - 無効カウンターは変更しない */
    } else {
        /* No valid data found */
        /* 有効データ未検出 */
        if (s_gps_invalid_count < UINT8_MAX) {
            s_gps_invalid_count++;
        }
    }
}

/* Public API implementation */

esp_err_t gps_init(void) {
    ESP_LOGI(TAG, "Initializing GPS module");

    /* Configure UART */
    /* UART設定 */
    const uart_config_t uart_config = {
        .baud_rate = GPS_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    /* Install UART driver */
    /* UARTドライバインストール */
    esp_err_t ret = uart_driver_install(
        GPS_UART_PORT,
        GPS_UART_RX_BUF_SIZE * 2,
        0,
        0,
        NULL,
        0
    );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Configure UART parameters */
    /* UARTパラメータ設定 */
    ret = uart_param_config(GPS_UART_PORT, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART param config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Set UART pins */
    /* UARTピン設定 */
    ret = uart_set_pin(
        GPS_UART_PORT,
        GPS_UART_TX_PIN,
        GPS_UART_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE
    );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART set pin failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Send GPS update rate command (2Hz) */
    /* GPS更新レートコマンド送信（2Hz） */
    uart_write_bytes(GPS_UART_PORT, GPS_UPDATE_RATE_CMD, strlen(GPS_UPDATE_RATE_CMD));

    /* Initialize GPS data structure */
    /* GPSデータ構造体初期化 */
    init_gps_data();

    ESP_LOGI(TAG, "GPS module initialized (UART%d, RX=%d, TX=%d, %d baud)",
             GPS_UART_PORT, GPS_UART_RX_PIN, GPS_UART_TX_PIN, GPS_UART_BAUD_RATE);

    return ESP_OK;
}

esp_err_t gps_poll(void) {
    /* Read available data from UART */
    /* UARTから利用可能なデータを読み取り */
    int rx_bytes = uart_read_bytes(GPS_UART_PORT, s_rx_buffer, GPS_UART_RX_BUF_SIZE, 10 / portTICK_PERIOD_MS);

    if (rx_bytes > 0) {
        s_rx_buffer[rx_bytes] = '\0';

        /* Parse NMEA data */
        /* NMEAデータをパース */
        parse_nmea_buffer((char *)s_rx_buffer);
    }

    /* Always return ESP_OK in polling mode */
    /* ポーリングモードでは常にESP_OK返却 */
    return ESP_OK;
}

esp_err_t gps_get_data(gps_data_t *out_data) {
    if (out_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *out_data = s_gps_data;
    return ESP_OK;
}

gps_status_t gps_get_status(void) {
    if (!gps_is_found()) {
        return GPS_STATUS_NULL;
    }

    /* Check minimum satellites - simplified criteria */
    /* 衛星数のみで判定 - シンプルな基準 */
    if (s_gps_data.num_satellites >= GPS_OK_MIN_SATELLITES) {
        return GPS_STATUS_OK;
    }

    /* Signal detected but not enough satellites yet */
    /* 信号検出済みだが衛星数がまだ不足 */
    return GPS_STATUS_SEARCH;
}

bool gps_is_found(void) {
    return (s_gps_invalid_count < GPS_INVALID_THRESHOLD);
}
