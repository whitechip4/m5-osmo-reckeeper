/* SPDX-License-Identifier: MIT */
/*
 * UI State Management
 * Single source of truth for all UI display data
 * Separates state management from rendering
 */

#ifndef UI_STATE_H
#define UI_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "dji_protocol.h"
#include "gps_module.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Display state structure
 * Contains all data needed for UI rendering
 * Updated by various parts of the application */
typedef struct {
    /* Main display state */
    dji_state_t main_state;           /* IDLE, PAIRING, PAIRED, RECORDING, etc. */
    uint32_t device_id;               /* Osmo device ID for pairing display */
    uint16_t recording_time;          /* Recording time in seconds */

    /* Status indicators */
    struct {
        gps_status_t gps_status;      /* GPS state */
        uint8_t gps_satellites;       /* Number of satellites */
        int device_battery;           /* Device battery (-1 for error) */
        int camera_battery;           /* Camera battery (0 for unavailable) */
        uint32_t sd_remaining_time;   /* SD card remaining recording time (seconds) */
        bool rec_keep_enabled;        /* Rec Keep mode state */
    } status;

    /* Power off screen (overrides normal display) */
    struct {
        bool active;                  /* true during power off flow */
        int countdown;                /* Countdown value (3,2,1) or 0 for guide */
        bool goodbye;                 /* true when showing "BYE" message */
    } poweroff;

} display_state_t;

/* ========== State Management Functions ========== */

/* Initialize UI state system
 * Must be called before any other ui_state functions */
void ui_state_init(void);

/* Set main display state
 * Args:
 *   state: Current DJI state
 *   device_id: Osmo device ID (0 if unknown)
 *   rec_time: Recording time in seconds */
void ui_state_set_main(dji_state_t state, uint32_t device_id, uint16_t rec_time);

/* Set GPS status
 * Args:
 *   status: GPS status
 *   satellites: Number of satellites */
void ui_state_set_gps(gps_status_t status, uint8_t satellites);

/* Set battery levels
 * Args:
 *   device_batt: Device battery percentage (-1 for error)
 *   camera_batt: Camera battery percentage (0 for unavailable) */
void ui_state_set_battery(int device_batt, int camera_batt);

/* Set SD card remaining recording time
 * Args:
 *   remaining_seconds: Remaining recording time in seconds */
void ui_state_set_sd_time(uint32_t remaining_seconds);

/* Set Rec Keep mode state
 * Args:
 *   enabled: true if Rec Keep mode is enabled */
void ui_state_set_rec_keep(bool enabled);

/* Set power off screen state
 * Args:
 *   active: true during power off flow
 *   countdown: Countdown value (3,2,1) or 0 for guide
 *   goodbye: true when showing "BYE" message */
void ui_state_set_poweroff(bool active, int countdown, bool goodbye);

/* Get current display state (read-only)
 * Returns: Pointer to current display state */
const display_state_t* ui_state_get(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_STATE_H */
