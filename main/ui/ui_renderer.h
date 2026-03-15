/* SPDX-License-Identifier: MIT */
/*
 * UI Renderer Interface
 * Single render pass that draws all UI elements to buffer and pushes to display
 * Called periodically from main loop (1Hz)
 */

#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize renderer
 * Creates full-screen sprite buffer for double buffering
 * Returns: true on success, false on failure */
bool ui_renderer_init(void);

/* Render current display state to screen
 * 1. Clears buffer to black
 * 2. Draws all UI elements based on current state
 * 3. Pushes buffer to display in single operation
 * This should be called once per main loop iteration (1Hz) */
void ui_renderer_render(void);

/* Clean up renderer resources
 * Destroys sprite buffer */
void ui_renderer_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_RENDERER_H */
