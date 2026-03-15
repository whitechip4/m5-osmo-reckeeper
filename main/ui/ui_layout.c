/* SPDX-License-Identifier: MIT */
/*
 * UI Layout Implementation
 * Provides pre-calculated coordinates for UI rendering
 */

#include "ui/ui_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration for M5Unified C interface */
struct M5UnifiedImpl;
extern struct M5UnifiedImpl M5;

int M5_display_width(void);
int M5_display_height(void);

#ifdef __cplusplus
}
#endif

/* Static layout structure */
/* 静的レイアウト構造体 */
static ui_layout_t s_layout = {0};

/* Initialize layout with current screen dimensions
 * Call this before using the layout structure */
static void init_layout(void) {
    if (s_layout.center_x != 0) {
        return;  /* Already initialized */
    }

    s_layout.center_x = M5_display_width() / 2;
    s_layout.y_line1 = M5_display_height() / 2 - 20;
    s_layout.y_line2 = s_layout.y_line1 + 25;
    s_layout.y_line3 = s_layout.y_line2 + 20;
    s_layout.y_gps = 2;
    s_layout.y_gps_siv = 12;
    s_layout.y_battery_dev = 2;
    s_layout.y_battery_cam = 12;
    s_layout.y_battery_sd = 22;
}

/* Get current layout structure
 * Returns pointer to const layout with pre-calculated coordinates */
const ui_layout_t* ui_get_layout(void) {
    init_layout();
    return &s_layout;
}
