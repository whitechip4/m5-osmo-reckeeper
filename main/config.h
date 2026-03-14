#ifndef CONFIG_H
#define CONFIG_H

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

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
