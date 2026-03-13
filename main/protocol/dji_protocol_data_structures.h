/* SPDX-License-Identifier: MIT */
/*
 * DJI Protocol Data Structures
 * DJIプロトコルデータ構造体
 */

#ifndef DJI_PROTOCOL_DATA_STRUCTURES_H
#define DJI_PROTOCOL_DATA_STRUCTURES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Connection Request/Response */
/* 接続リクエスト/レスポンス */
typedef struct __attribute__((packed)) {
    uint32_t device_id;            /* Device ID / デバイスID */
    uint8_t mac_addr_len;          /* MAC address length / MACアドレス長 */
    int8_t mac_addr[16];           /* MAC address / MACアドレス */
    uint32_t fw_version;           /* Firmware version / ファームウェアバージョン */
    uint8_t conidx;                /* Reserved field / 予約領域 */
    uint8_t verify_mode;           /* Verification mode / 認証モード */
    uint16_t verify_data;          /* Verification data or result / 認証データまたは結果 */
    uint8_t reserved[4];           /* Reserved field / 予約領域 */
} connection_request_command_frame_t;

typedef struct __attribute__((packed)) {
    uint32_t device_id;            /* Device ID / デバイスID */
    uint8_t ret_code;              /* Return code / 戻り値 (0=success) */
    uint8_t reserved[4];           /* Reserved field / 予約領域 */
} connection_request_response_frame_t;

/* Camera Status Subscription */
/* カメラ状態購読 */
typedef struct __attribute__((packed)) {
    uint8_t push_mode;             /* Push mode: 0-Off, 1-Single, 2-Periodic, 3-Periodic+Status change */
                                     /* プッシュモード: 0-Off, 1-単発, 2-定期, 3-定期+状態変化時 */
    uint8_t push_freq;             /* Push frequency in 0.1Hz, only 20 is allowed (2Hz) */
                                     /* プッシュ周波数 (0.1Hz単位), 20のみ有効 (2Hz) */
    uint8_t reserved[4];           /* Reserved field / 予約領域 */
} camera_status_subscription_command_frame_t;

/* Camera Status Push */
/* カメラ状態プッシュ */
typedef struct __attribute__((packed)) {
    uint8_t camera_mode;           /* Camera mode / カメラモード */
    uint8_t camera_status;         /* Camera status: 0x00-Screen off, 0x01-Live view, 0x02-Playback, 0x03-Recording, 0x05-Pre-recording */
                                     /* カメラ状態: 0x00-画面OFF, 0x01-ライブビュー, 0x02-再生, 0x03-録画中, 0x05-プリレコーディング */
    uint8_t video_resolution;      /* Video resolution / 動画解像度 */
    uint8_t fps_idx;               /* Frame rate / フレームレート */
    uint8_t eis_mode;              /* Electronic image stabilization mode / 電子手ぶれ補正モード */
    uint16_t record_time;          /* Current recording time in seconds / 録画時間 (秒) */
    uint8_t fov_type;              /* FOV type, reserved / FOVタイプ, 予約 */
    uint8_t photo_ratio;           /* Photo aspect ratio / 写真アスペクト比 */
    uint16_t real_time_countdown;  /* Real-time countdown in seconds / カウントダウン (秒) */
    uint16_t timelapse_interval;   /* Timelapse interval / タイムラプス間隔 */
    uint16_t timelapse_duration;   /* Time-lapse recording duration in seconds / タイムラプス録画時間 (秒) */
    uint32_t remain_capacity;      /* Remaining SD card capacity in MB / SDカード残容量 (MB) */
    uint32_t remain_photo_num;     /* Remaining number of photos / 撮影可能枚数 */
    uint32_t remain_time;          /* Remaining recording time in seconds / 残録画時間 (秒) */
    uint8_t user_mode;             /* User mode / ユーザーモード */
    uint8_t power_mode;            /* Power mode: 0-Normal, 3-Sleep / 電源モード: 0-通常, 3-スリープ */
    uint8_t camera_mode_next_flag; /* Pre-switch flag / プレ切り替えフラグ */
    uint8_t temp_over;             /* Temperature status: 0-Normal, 1-Warning, 2-High, 3-Overheat */
                                     /* 温度状態: 0-正常, 1-警告, 2-高温, 3-過熱 */
    uint32_t photo_countdown_ms;   /* Photo countdown in milliseconds / 写真カウントダウン (ミリ秒) */
    uint16_t loop_record_sends;    /* Loop recording duration in seconds / ループ録画時間 (秒) */
    uint8_t camera_bat_percentage; /* Camera battery percentage: 0-100% / カメラバッテリー残量 */
} camera_status_push_command_frame_t;

#endif /* DJI_PROTOCOL_DATA_STRUCTURES_H */
