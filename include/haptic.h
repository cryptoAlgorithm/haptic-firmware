#pragma once

#include "stdint.h"

typedef struct {
  uint8_t inA;
  uint8_t inB;
} haptic_output;

void start_haptic_task();

