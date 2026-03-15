/* SPDX-License-Identifier: MIT */
/*
 * System Initialization Interface
 * Handles all system initialization sequences
 */

#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* System initialization result
 * Returns true on success, false on failure */
bool system_initialize(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_INIT_H */
