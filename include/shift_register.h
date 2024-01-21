#pragma once

#include <stdint.h>

#define REGISTER_BYTES 3 // Total bytes represented by shift registers

void shift_init(uint8_t, uint8_t, uint8_t);

void shift_set_output(uint8_t bit, uint8_t state);

void shift_update();
