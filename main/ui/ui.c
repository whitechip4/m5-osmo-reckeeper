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

/**
 * @brief Initialize double buffer (deprecated)
 * @return true on success, false on failure
 * @deprecated Use ui_renderer_init() instead. This function forwards to ui_renderer_init().
 */
bool ui_init_double_buffer(void) {
    ESP_LOGW(TAG, "ui_init_double_buffer is deprecated, use ui_renderer_init instead");
    /* Forward to ui_renderer for compatibility */
    return ui_renderer_init();
}

/**
 * @brief Clean up double buffer (deprecated)
 * @deprecated Use ui_renderer_cleanup() instead. This function forwards to ui_renderer_cleanup().
 */
void ui_cleanup_double_buffer(void) {
    ESP_LOGW(TAG, "ui_cleanup_double_buffer is deprecated, use ui_renderer_cleanup instead");
    /* Forward to ui_renderer for compatibility */
    ui_renderer_cleanup();
}

#ifdef __cplusplus
}
#endif
