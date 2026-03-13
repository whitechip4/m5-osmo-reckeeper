/* SPDX-License-Identifier: MIT */
/*
 * DJI Protocol Parser
 * DJIプロトコルパーサー
 */

#ifndef DJI_PROTOCOL_PARSER_H
#define DJI_PROTOCOL_PARSER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Protocol frame parsing result structure
 *        プロトコルフレーム解析結果構造体
 */
typedef struct {
    uint8_t sof;                   /* Start of frame (0xAA) / フレームヘッダー */
    uint16_t version;              /* Protocol version / プロトコルバージョン */
    uint16_t frame_length;         /* Total frame length / フレーム長 */
    uint8_t cmd_type;              /* Command type / コマンドタイプ */
    uint8_t enc;                   /* Encryption flag / 暗号化フラグ */
    uint8_t res[3];                /* Reserved field / 予約領域 */
    uint16_t seq;                  /* Sequence number / シーケンス番号 */
    uint16_t crc16;                /* CRC-16 checksum / CRC-16 チェックサム */
    const uint8_t *data;           /* Pointer to data segment / データセグメントポインタ */
    size_t data_length;            /* Length of data segment / データセグメント長 */
    uint32_t crc32;                /* CRC-32 checksum / CRC-32 チェックサム */
} protocol_frame_t;

/**
 * @brief Parse notification frame from BLE
 *        BLE通知フレームの解析
 *
 * @param frame_data Raw frame data / 生フレームデータ
 * @param frame_length Frame length / フレーム長
 * @param frame_out Output structure / 出力構造体
 * @return 0 on success, negative on failure / 成功時0, 失敗時負数
 */
int protocol_parse_notification(const uint8_t *frame_data, size_t frame_length,
                                 protocol_frame_t *frame_out);

/**
 * @brief Create protocol frame for sending
 *        送信用プロトコルフレームの作成
 *
 * @param cmd_set Command set / コマンドセット
 * @param cmd_id Command ID / コマンドID
 * @param cmd_type Command type / コマンドタイプ
 * @param structure Data structure / データ構造体
 * @param seq Sequence number / シーケンス番号
 * @param frame_length_out Output frame length / フレーム長出力
 * @return Pointer to frame buffer (must be freed), NULL on failure
 *         フレームバッファへのポインタ (解放必須), 失敗時NULL
 */
uint8_t* protocol_create_frame(uint8_t cmd_set, uint8_t cmd_id,
                                uint8_t cmd_type, const void *structure,
                                uint16_t seq, size_t *frame_length_out);

#ifdef __cplusplus
}
#endif

#endif /* DJI_PROTOCOL_PARSER_H */
