/* SPDX-License-Identifier: MIT */
/*
 * UI Module Implementation
 * Provides display system initialization and cleanup
 * Rendering is handled by ui_renderer, state by ui_state
 */

#include "ui/ui.h"
#include "ui/ui_renderer.h"
#include "m5_wrapper.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char *TAG = "UI";

/* ========== Public Interface ========== */

/* Legacy double buffer initialization - deprecated
 * Now handled by ui_renderer_init() */
bool ui_init_double_buffer(void) {
    ESP_LOGW(TAG, "ui_init_double_buffer is deprecated, use ui_renderer_init instead");
    /* Forward to ui_renderer for compatibility */
    return ui_renderer_init();
}

/* Legacy double buffer cleanup - deprecated
 * Now handled by ui_renderer_cleanup() */
void ui_cleanup_double_buffer(void) {
    ESP_LOGW(TAG, "ui_cleanup_double_buffer is deprecated, use ui_renderer_cleanup instead");
    /* Forward to ui_renderer for compatibility */
    ui_renderer_cleanup();
}

#ifdef __cplusplus
}
#endif
