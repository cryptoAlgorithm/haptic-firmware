#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

extern const uint8_t haptic_num_outputs;

typedef struct {
  uint8_t inA;
  uint8_t inB;
} haptic_output;

void start_haptic_task();

void haptic_update_delays(uint8_t * delays);

#ifdef __cplusplus
}
#endif
