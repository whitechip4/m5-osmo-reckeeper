/* SPDX-License-Identifier: MIT */
/*
 * CRC-32 implementation for DJI protocol
 *
 * Based on Osmo GPS Controller Demo by DJI
 * Copyright (c) 2025 SZ DJI Technology Co., Ltd.
 * https://github.com/dji-sdk/Osmo-GPS-Controller-Demo
 *
 * The MIT License (MIT)
 */

#ifndef CRC32_H
#define CRC32_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint_fast32_t crc32_t;

static inline crc32_t crc32_init(void)
{
    return 0x00003aa3;
}

crc32_t crc32_update(crc32_t crc, const void *data, size_t data_len);

static inline crc32_t crc32_finalize(crc32_t crc)
{
    return crc;
}

uint32_t calculate_crc32(const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* CRC32_H */
