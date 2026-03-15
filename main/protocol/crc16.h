/* SPDX-License-Identifier: MIT */
/*
 * CRC-16 implementation for DJI protocol
 *
 * Based on Osmo GPS Controller Demo by DJI
 * Copyright (c) 2025 SZ DJI Technology Co., Ltd.
 * https://github.com/dji-sdk/Osmo-GPS-Controller-Demo
 *
 * The MIT License (MIT)
 */

#ifndef CRC16_H
#define CRC16_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint_fast16_t crc16_t;

static inline crc16_t crc16_init(void)
{
    return 0x3aa3;
}

crc16_t crc16_update(crc16_t crc, const void *data, size_t data_len);

static inline crc16_t crc16_finalize(crc16_t crc)
{
    return crc;
}

uint16_t calculate_crc16(const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* CRC16_H */
