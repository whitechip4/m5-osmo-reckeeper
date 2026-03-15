/* SPDX-License-Identifier: MIT */
/*
 * Status Updater Interface
 * Handles periodic status updates in the event loop
 */

#ifndef STATUS_UPDATER_H
#define STATUS_UPDATER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Update all status displays
 * Should be called once per event loop iteration
 * Handles:
 *   - Recording time (when recording)
 *   - Camera battery (when connected)
 *   - SD card status (when connected)
 *   - GPS polling and display
 *   - GPS data transmission to camera */
void status_update_all(void);

#ifdef __cplusplus
}
#endif

#endif /* STATUS_UPDATER_H */
