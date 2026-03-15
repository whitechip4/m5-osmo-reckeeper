/* SPDX-License-Identifier: MIT */
/*
 * Button Handlers Interface
 * Handles all button press events for the application
 */

#ifndef BUTTON_HANDLERS_H
#define BUTTON_HANDLERS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Button action result codes */
typedef enum {
    BTN_ACTION_NONE,               /* No action performed */
    BTN_ACTION_CONNECT,            /* BLE connection initiated */
    BTN_ACTION_DISCONNECT,         /* Disconnection initiated */
    BTN_ACTION_TOGGLE_RECORDING,   /* Recording toggled */
    BTN_ACTION_TOGGLE_REC_KEEP,    /* Rec Keep mode toggled */
    BTN_ACTION_RESET,              /* Device reset initiated */
    BTN_ACTION_POWER_OFF           /* Power off initiated */
} button_action_t;

/* Button handler functions */
/* ボタンハンドラー関数 */

/* Handle button A press
 * Returns: Action code indicating what was performed */
button_action_t btn_handle_a_press(void);

/* Handle button B press
 * Returns: Action code indicating what was performed */
button_action_t btn_handle_b_press(void);

/* Handle PWR button press (blocking call with countdown)
 * This function will block until the button is released or 3 seconds elapse.
 * Returns: Action code indicating reset or power off */
button_action_t btn_handle_pwr_press(void);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_HANDLERS_H */
