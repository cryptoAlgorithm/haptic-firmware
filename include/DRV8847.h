#pragma once

#include "stdint.h"

uint8_t drv_test(uint8_t addr);

uint8_t read_reg(uint8_t addr, uint8_t reg, uint8_t * d);

uint8_t drv_disable_nFAULT(uint8_t addr);
uint8_t drv_enable_nFAULT(uint8_t addr);

uint8_t drv_update_addr(uint8_t addr, uint8_t new_addr);
