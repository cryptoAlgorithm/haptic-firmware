#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void battery_init();

/**
 * Get actual vBatt in millivolts
 */
uint32_t battery_getmv();

#ifdef __cplusplus
}
#endif