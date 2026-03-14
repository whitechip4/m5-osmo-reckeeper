#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Rec Keep mode auto-restart delay (milliseconds)
 *        Rec Keepモード自動再開遅延（ミリ秒）
 *
 * When Rec Keep mode is enabled and an external recording stop is detected,
 * recording will automatically restart after this delay.
 * Rec Keepモード有効時、外部停止検知後にこの遅延時間で録画自動再開。
 */
#define REC_KEEP_DELAY_MS 2000

/* GPS Configuration - Updated pins for GROVE port */
/* GPS設定 - GROVEポート用ピン変更 */
#define GPS_UART_PORT          UART_NUM_2
#define GPS_UART_TX_PIN        GPIO_NUM_32  /* GROVE port yellow (TX) / GROVEポート黄 */
#define GPS_UART_RX_PIN        GPIO_NUM_33  /* GROVE port white (RX) / GROVEポート白 */
#define GPS_UART_BAUD_RATE     115200
#define GPS_UART_RX_BUF_SIZE   800
#define GPS_UPDATE_RATE_CMD    "$PAIR050,500*26\r\n"  /* 2Hz update rate / 2Hz更新レート */
#define GPS_POLL_INTERVAL_MS   500                     /* 2Hz polling / 2Hzポーリング */
#define GPS_INVALID_THRESHOLD  10
#define GPS_OK_MIN_SATELLITES  4                       /* Minimum satellites for OK status / OK状態の最小衛星数 */

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
