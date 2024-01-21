#include "shift_register.h"

#include <Arduino.h>
#include <SPI.h>

// Maintain internal state of shift registers for byte-level updates
static uint8_t state[REGISTER_BYTES] = { 0 };
static uint8_t latch;

static const char * TAG = "shift_reg";

void shift_update() {
  // Write out state to registers
  digitalWrite(latch, LOW);
  for (uint8_t i = REGISTER_BYTES-1; i --> 0; ) SPI.write(state[i]);
  digitalWrite(latch, HIGH);
  ESP_LOGV(TAG, "Update state: %d, %d, %d", state[0], state[1], state[2]);
}

void shift_init(uint8_t clk, uint8_t rclk, uint8_t dta) {
  // I2S not used as benefits for this application don't justify its complexity
  SPI.begin(clk, -1, dta);
  SPI.setFrequency(10000000); // 10MHz
  latch = rclk;
  shift_update(); // Clear registerrs
}

void shift_set_output(uint8_t bit, uint8_t st) {
  state[bit>>3] = state[bit>>3] & ~(1 << bit%8) | (st << bit%8);
}
