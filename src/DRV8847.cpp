#include "DRV8847.h"

#include <Wire.h>

#define DEFAULT_ADDR    0x60

#define REG_SLAVE_ADDR  0x00
#define REG_IC1_CON     0x01
#define REG_IC2_CON     0x02
#define REG_SLR_STATUS1 0x03
#define REG_STATUS2     0x04

static const char * TAG = "DRV";

static uint8_t write_reg(uint8_t addr, uint8_t reg, uint8_t d) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(d);
  return Wire.endTransmission();
}
uint8_t read_reg(uint8_t addr, uint8_t reg, uint8_t * d) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  uint8_t res;
  if ((res = Wire.endTransmission(false))) return res;
  if (Wire.requestFrom(addr, (size_t) 1) != 1) {
    ESP_LOGE(TAG, "Request read fail");
    return -1;
  }
  *d = Wire.read();
  return 0;
}

uint8_t drv_test(uint8_t addr) {
  uint8_t readAddr;
  if (read_reg(addr, REG_SLAVE_ADDR, &readAddr)) {
    ESP_LOGE(TAG, "Failed to read address reg!");
    return 0;
  }
  ESP_LOGD(TAG, "Read addr: %d", readAddr);
  return readAddr == addr;
}

uint8_t drv_disable_nFAULT(uint8_t addr) {
  return write_reg(addr, REG_IC2_CON, 1 << 6);
}
uint8_t drv_enable_nFAULT(uint8_t addr) {
  return write_reg(addr, REG_IC2_CON, 0);
}

uint8_t drv_update_addr(uint8_t addr, uint8_t new_addr) {
  return write_reg(addr, REG_SLAVE_ADDR, new_addr);
}
