/* SPDX-License-Identifier: MIT */
/*
 * DJI Protocol Parser (Simplified)
 * DJIプロトコルパーサー (簡略版)
 */

#include <string.h>
#include <stdlib.h>
#include <esp_log.h>
#include "dji_protocol_parser.h"
#include "dji_protocol_data_structures.h"
#include "crc16.h"
#include "crc32.h"

static const char *TAG = "DJI_PARSER";

/* Protocol frame field length definitions */
/* プロトコルフレーム各部の長さ定義 */
#define PROTOCOL_SOF_LENGTH 1
#define PROTOCOL_VER_LEN_LENGTH 2
#define PROTOCOL_CMD_TYPE_LENGTH 1
#define PROTOCOL_ENC_LENGTH 1
#define PROTOCOL_RES_LENGTH 3
#define PROTOCOL_SEQ_LENGTH 2
#define PROTOCOL_CRC16_LENGTH 2
#define PROTOCOL_CMD_SET_LENGTH 1
#define PROTOCOL_CMD_ID_LENGTH 1
#define PROTOCOL_CRC32_LENGTH 4

#define PROTOCOL_HEADER_LENGTH ( \
    PROTOCOL_SOF_LENGTH +        \
    PROTOCOL_VER_LEN_LENGTH +    \
    PROTOCOL_CMD_TYPE_LENGTH +   \
    PROTOCOL_ENC_LENGTH +        \
    PROTOCOL_RES_LENGTH +        \
    PROTOCOL_SEQ_LENGTH +        \
    PROTOCOL_CRC16_LENGTH +      \
    PROTOCOL_CMD_SET_LENGTH +    \
    PROTOCOL_CMD_ID_LENGTH)

#define PROTOCOL_TAIL_LENGTH PROTOCOL_CRC32_LENGTH

#define PROTOCOL_FULL_FRAME_LENGTH(data_length) ( \
    PROTOCOL_HEADER_LENGTH +                      \
    (data_length) +                               \
    PROTOCOL_TAIL_LENGTH)

int protocol_parse_notification(const uint8_t *frame_data, size_t frame_length,
                                 protocol_frame_t *frame_out)
{
    /* Check minimum frame length */
    /* 最小フレーム長チェック */
    if (frame_length < 16) {
        ESP_LOGE(TAG, "Frame too short: %zu bytes (min 16)", frame_length);
        return -1;
    }

    /* Check SOF */
    /* フレームヘッダーチェック */
    if (frame_data[0] != 0xAA) {
        ESP_LOGE(TAG, "Invalid SOF: 0x%02X", frame_data[0]);
        return -2;
    }

    /* Parse Ver/Length */
    /* Ver/Length 解析 */
    uint16_t ver_length = (frame_data[2] << 8) | frame_data[1];
    uint16_t expected_length = ver_length & 0x03FF;

    if (expected_length != frame_length) {
        ESP_LOGE(TAG, "Length mismatch: expected %u, got %zu", expected_length, frame_length);
        return -3;
    }

    /* Verify CRC-16 (SOF to SEQ) */
    /* CRC-16 検証 (SOFからSEQまで) */
    uint16_t crc16_received = (frame_data[11] << 8) | frame_data[10];
    uint16_t crc16_calculated = calculate_crc16(frame_data, 10);
    if (crc16_received != crc16_calculated) {
        ESP_LOGE(TAG, "CRC-16 mismatch: recv 0x%04X, calc 0x%04X",
                 crc16_received, crc16_calculated);
        return -4;
    }

    /* Verify CRC-32 (SOF to DATA) */
    /* CRC-32 検証 (SOFからDATAまで) */
    uint32_t crc32_received = (frame_data[frame_length - 1] << 24) |
                              (frame_data[frame_length - 2] << 16) |
                              (frame_data[frame_length - 3] << 8) |
                              frame_data[frame_length - 4];
    uint32_t crc32_calculated = calculate_crc32(frame_data, frame_length - 4);
    if (crc32_received != crc32_calculated) {
        ESP_LOGE(TAG, "CRC-32 mismatch: recv 0x%08X, calc 0x%08X",
                 (unsigned int)crc32_received, (unsigned int)crc32_calculated);
        return -5;
    }

    /* Fill output structure */
    /* 出力構造体への格納 */
    frame_out->sof = frame_data[0];
    frame_out->version = ver_length >> 10;
    frame_out->frame_length = expected_length;
    frame_out->cmd_type = frame_data[3];
    frame_out->enc = frame_data[4];
    memcpy(frame_out->res, &frame_data[5], 3);
    frame_out->seq = (frame_data[9] << 8) | frame_data[8];
    frame_out->crc16 = crc16_received;

    /* Process DATA segment */
    /* データセグメント処理 */
    if (frame_length > 16) {
        frame_out->data = &frame_data[12];
        frame_out->data_length = frame_length - 16;
    } else {
        frame_out->data = NULL;
        frame_out->data_length = 0;
    }

    frame_out->crc32 = crc32_received;

    ESP_LOGI(TAG, "Frame parsed: CmdSet=0x%02X, CmdID=0x%02X, SEQ=%u",
             frame_out->data[0], frame_out->data[1], frame_out->seq);

    return 0;
}

uint8_t* protocol_create_frame(uint8_t cmd_set, uint8_t cmd_id,
                                uint8_t cmd_type, const void *structure,
                                uint16_t seq, size_t *frame_length_out)
{
    if (structure == NULL || frame_length_out == NULL) {
        ESP_LOGE(TAG, "NULL parameter");
        return NULL;
    }

    size_t data_length = 0;
    uint8_t *payload_data = NULL;

    /* Determine data length based on command */
    /* コマンドに応じたデータ長決定 */
    switch (cmd_set) {
        case 0x00: /* Connection / Version / GPS */
            if (cmd_id == 0x19) { /* Connection request */
                data_length = sizeof(connection_request_command_frame_t);
            } else {
                ESP_LOGE(TAG, "Unsupported CmdID 0x%02X for CmdSet 0x00", cmd_id);
                return NULL;
            }
            break;

        case 0x1D: /* Camera control */
            if (cmd_id == 0x05) { /* Status subscription */
                data_length = sizeof(camera_status_subscription_command_frame_t);
            } else {
                ESP_LOGE(TAG, "Unsupported CmdID 0x%02X for CmdSet 0x1D", cmd_id);
                return NULL;
            }
            break;

        default:
            ESP_LOGE(TAG, "Unsupported CmdSet 0x%02X", cmd_set);
            return NULL;
    }

    /* Allocate and copy payload data */
    /* ペイロードデータの確保とコピー */
    payload_data = (uint8_t *)malloc(data_length);
    if (payload_data == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return NULL;
    }
    memcpy(payload_data, structure, data_length);

    /* Calculate total frame length */
    /* 全フレーム長計算 */
    *frame_length_out = PROTOCOL_HEADER_LENGTH + data_length + PROTOCOL_TAIL_LENGTH;

    /* Allocate frame buffer */
    /* フレームバッファ確保 */
    uint8_t *frame = (uint8_t *)malloc(*frame_length_out);
    if (frame == NULL) {
        ESP_LOGE(TAG, "Frame allocation failed");
        free(payload_data);
        return NULL;
    }

    memset(frame, 0, *frame_length_out);

    /* Build frame header */
    /* フレームヘッダー構築 */
    size_t offset = 0;
    frame[offset++] = 0xAA; /* SOF */

    uint16_t ver_length = (0 << 10) | (*frame_length_out & 0x03FF);
    frame[offset++] = ver_length & 0xFF;
    frame[offset++] = (ver_length >> 8) & 0xFF;

    frame[offset++] = cmd_type;    /* CmdType */
    frame[offset++] = 0x00;        /* ENC (no encryption) */
    frame[offset++] = 0x00;        /* RES */
    frame[offset++] = 0x00;
    frame[offset++] = 0x00;

    frame[offset++] = seq & 0xFF;         /* SEQ low */
    frame[offset++] = (seq >> 8) & 0xFF;  /* SEQ high */

    /* Calculate and fill CRC-16 */
    /* CRC-16 計算と格納 */
    uint16_t crc16 = calculate_crc16(frame, offset);
    frame[offset++] = crc16 & 0xFF;
    frame[offset++] = (crc16 >> 8) & 0xFF;

    /* Fill command set and ID */
    /* コマンドセットとID格納 */
    frame[offset++] = cmd_set;
    frame[offset++] = cmd_id;

    /* Copy payload data */
    /* ペイロードデータコピー */
    memcpy(&frame[offset], payload_data, data_length);
    offset += data_length;

    /* Calculate and fill CRC-32 */
    /* CRC-32 計算と格納 */
    uint32_t crc32 = calculate_crc32(frame, offset);
    frame[offset++] = crc32 & 0xFF;
    frame[offset++] = (crc32 >> 8) & 0xFF;
    frame[offset++] = (crc32 >> 16) & 0xFF;
    frame[offset++] = (crc32 >> 24) & 0xFF;

    free(payload_data);

    ESP_LOGI(TAG, "Frame created: CmdSet=0x%02X, CmdID=0x%02X, SEQ=%u, len=%zu",
             cmd_set, cmd_id, seq, *frame_length_out);

    /* Log first 16 bytes for debugging */
    /* デバッグ用に最初の16バイトをログ */
    ESP_LOGI(TAG, "Frame bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
             frame[0], frame[1], frame[2], frame[3], frame[4], frame[5], frame[6], frame[7],
             frame[8], frame[9], frame[10], frame[11], frame[12], frame[13], frame[14], frame[15]);

    return frame;
}
