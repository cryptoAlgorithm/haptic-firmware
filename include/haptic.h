#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "MAX22200.h"
#include "stdint.h"

extern const uint8_t haptic_num_outputs;

typedef struct {
  max_config_t * driver;
  uint8_t ch;
} haptic_output;

void start_haptic_task();

void haptic_stop();
void haptic_set_state(uint8_t out_n, uint8_t st, uint8_t update);
void haptic_update_delays(uint8_t * delays);

#ifdef __cplusplus
}
#endif
