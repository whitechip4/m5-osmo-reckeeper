/* SPDX-License-Identifier: MIT */
/*
 * UI Module Interface
 * Provides display system initialization and cleanup
 * Rendering is handled by ui_renderer, state by ui_state
 */

#ifndef UI_H
#define UI_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Legacy double buffer functions - deprecated, use ui_renderer instead
 * These are kept for backward compatibility but should not be used */
/* 互換性のために残されていますが、使用しないでください */
bool ui_init_double_buffer(void);
void ui_cleanup_double_buffer(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */
