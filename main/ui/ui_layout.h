/* SPDX-License-Identifier: MIT */
/*
 * UI Layout Definitions
 * Defines coordinate positions and layout structure for LCD display
 */

#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>

/* UI Layout structure
 * Pre-calculated coordinates for consistent UI rendering */
typedef struct {
    int center_x;      /* Screen center X coordinate */
    int y_line1;       /* First line (main status) */
    int y_line2;       /* Second line (sub status) */
    int y_line3;       /* Third line (device ID) */
    int y_gps;         /* GPS status position */
    int y_gps_siv;     /* GPS satellite count position */
    int y_battery_dev; /* Device battery position */
    int y_battery_cam; /* Camera battery position */
    int y_battery_sd;  /* SD card position */
} ui_layout_t;

/* Get current layout structure
 * Returns pointer to const layout with pre-calculated coordinates */
const ui_layout_t* ui_get_layout(void);

#endif /* UI_LAYOUT_H */
