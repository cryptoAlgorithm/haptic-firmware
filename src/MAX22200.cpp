#include "MAX22200.h"

#include <endian.h>

#include <Arduino.h>
#include <SPI.h>

#include "pins.h"

#define TAG "MAX"

// Default output config which keeps most things at their default
max_ch_config_t max_ch_cfg_default = {
  // Full scale current approx 937mA
  .hit_t = 25, // 250*40 / 25000 (f_chop)
  .hit = 100, // ~50% full scale
  .hold = 12 // Approx 12% full scale
};

max_config_t max_driver_a = {
  .cmd = DRIVER_CMD_1,
  .cs = DRIVER_CS_1,
  .fault = nFAULT_1,
  .ch_cfg = &max_ch_cfg_default
};
max_config_t max_driver_b = {
  .cmd = DRIVER_CMD_2,
  .cs = DRIVER_CS_2,
  .fault = nFAULT_1,
  .ch_cfg = &max_ch_cfg_default
};

// Register addresses (private to source)
#define MAX22200_REG_STATUS 0x00 // status read and config
#define MAX22200_REG_CF_CH0 0x01 // ch_x config
#define MAX22200_REG_CF_CH1 0x02
#define MAX22200_REG_CF_CH2 0x03
#define MAX22200_REG_CF_CH3 0x04
#define MAX22200_REG_CF_CH4 0x05
#define MAX22200_REG_CF_CH5 0x06
#define MAX22200_REG_CF_CH6 0x07
#define MAX22200_REG_CF_CH7 0x08
#define MAX22200_REG_FAULT  0x09 // detailed fault info
#define MAX22200_REG_CF_DPM 0x0A // Detection of Plunger Motion (DPM) config

#define MAX22200_NUM_CHS    (MAX22200_REG_CF_CH7-MAX22200_REG_CF_CH0+1)

// If defined, ignore UVM bit in status returned with CMD register write
// Only impacts logging for now
#define MAX22200_IGNORE_UVM

/**
 * Read or write to/from a register
 * 
 * addr - address of 32-bit register to read from
 * msb8b - 1 to only read the MSB 8 bits of the 32-bit register
 * d - data to write (used only for write, else ignored)
 * rw - 1 to write; 0 to read from register
 *
 * @returns Register value if read; status[7:0] otherwise
 */
static uint32_t max_reg(max_config_t * dev, uint8_t rw, uint8_t addr, uint8_t msb8b, uint32_t d) {
  // write command register (cmd = 1) to prepare for following operation
  digitalWrite(dev->cmd, 1);
  digitalWrite(dev->cs, 0);
  uint8_t cmd_w = ((rw & 1) << 7) | ((addr & 0b1111) << 1) | (msb8b & 1);
  // ESP_LOGD(TAG, "writing %u", cmd_w);
  uint8_t status = SPI.transfer(cmd_w);
  digitalWrite(dev->cs, 1);
  digitalWrite(dev->cmd, 0);
  if (status & (
#ifdef MAX22200_IGNORE_UVM
    ~(MAX22200_MASK_STATUS_ACTIVE | MAX22200_MASK_STATUS_UVM)
#else
    ~MAX22200_MASK_STATUS_ACTIVE
#endif
  )) { // check to see if any error
    ESP_LOGW(TAG, "cmd returned error status: %u", status);
  }

  // rw from target register (cmd = 0)
  digitalWrite(dev->cs, 0);
  uint32_t read;
  if (msb8b) read = SPI.transfer(rw ? d : 0);
  else read = SPI.transfer32(rw ? htobe32(d) : 0); // for tx must convert host to large endian because SPI is BE
  digitalWrite(dev->cs, 1);
  return rw ? status : read;
}

static uint32_t max_ch_config(max_config_t * dev, uint8_t ch_n, max_ch_config_t * cfg) {
  _Static_assert(sizeof(max_ch_config_t) == 4, "Incorrectly sized config, ensure packed and correct number of bits");

  uint32_t cfg32 = *((uint32_t *) cfg);
  ESP_LOGV(TAG, "set config n: %u, raw: %lu", ch_n, cfg32);

  return max_reg(dev, 1, MAX22200_REG_CF_CH0 + ch_n, 0, cfg32);
}

/**
 * Initialise MAX22200 following recommended device configuration flow
 * 
 * Refer to Figure 6. Programming Flow Chart of datasheet for more info.
 */
static esp_err_t max_dev_init(max_config_t * dev) {
  uint32_t status;

  ESP_LOGI(TAG, "begin dev init");

  do {
    // Status register read operation to check UVM flag
    status = max_reg(dev, 0, MAX22200_REG_STATUS, 0, 0);
    // Write status register for HW configuration and device activation
    max_reg(dev, 1, MAX22200_REG_STATUS, 0, MAX22200_MASK_STATUS_ACTIVE);

    // Configure all channels with fixed config
    for (uint8_t i = 0; i < MAX22200_NUM_CHS; ++i) {
      status = max_ch_config(dev, i, dev->ch_cfg);
      ESP_LOGD(TAG, "config ch: %u, status[7:0]: %lu", i, status);
    }

    // Status register read operation to check UVM flag
    do {
      status = max_reg(dev, 0, MAX22200_REG_STATUS, 0, 0);
    } while ((status & (MAX22200_MASK_STATUS_UVM)) && (status && MAX22200_MASK_STATUS_ACTIVE));

    if (status & MAX22200_MASK_STATUS_UVM) {
      ESP_LOGW(TAG, "communication error: status: %lu, trying init again...", status);
    }
  } while (status & 0xff & MAX22200_MASK_STATUS_UVM);

  if ((status & 0xff) != MAX22200_MASK_STATUS_ACTIVE) {
    ESP_LOGE(TAG, "status[7:0] unexpected value: %u", status & 0xff);
    return ESP_FAIL;
  }

  // Verify fault is deasserted
  if (!digitalRead(dev->fault)) {
    // TODO: Check status register for faults
    ESP_LOGE(TAG, "fault still asserted");
    return ESP_FAIL;
  }

  dev->dirty = 0;
  ESP_LOGI(TAG, "dev init success, final status: %lu", status);
  return ESP_OK;
}

void max_init() {
  // GPIO init
  pinMode(DRIVER_EN_1, OUTPUT);
  pinMode(DRIVER_EN_2, OUTPUT);
  pinMode(nFAULT_1, INPUT_PULLUP);
  pinMode(nFAULT_2, INPUT_PULLUP);
  pinMode(DRIVER_CMD_1, OUTPUT);
  pinMode(DRIVER_CMD_2, OUTPUT);
  pinMode(DRIVER_CS_1, OUTPUT);
  pinMode(DRIVER_CS_2, OUTPUT);
  digitalWrite(DRIVER_CS_1, 1); // CS should be 1 during idle
  digitalWrite(DRIVER_CS_2, 1);
  digitalWrite(DRIVER_EN_1, 1); // logic 1 to enable
  digitalWrite(DRIVER_EN_2, 1);
  // SPI init
  SPI.begin(DRIVER_CLK, DRIVER_MISO, DRIVER_MOSI);
  SPI.setFrequency(5000000); // 5MHz
  SPI.setBitOrder(SPI_MSBFIRST);
  SPI.setDataMode(SPI_MODE0); // SPI mode 0, 0

  delay(1); // According to spec must wait at least 0.5ms after EN = 1

  ESP_LOGI(TAG, "init drivers...");
  ESP_ERROR_CHECK(max_dev_init(&max_driver_a)); // Pull down ESP error check here instead of raising status code to outer scope
  ESP_ERROR_CHECK(max_dev_init(&max_driver_b)); // to hopefully provide more relevant abort messages if any
}

void max_set_ch_state(max_config_t * dev, uint8_t ch, uint8_t st, uint8_t update) {
  // First check if channel already has desired state, don't waste writes
  uint8_t msk = 1 << ch;
  if ((dev->channel_state & msk) == (st ? msk : 0)) {
    ESP_LOGD(TAG, "ignoring update to cur state: ch: %u, st: %u", ch, st);
    return;
  }
  dev->channel_state ^= msk;
  dev->dirty = 1;
  if (update) {
    max_push_ch_state(dev);
  }
}

uint8_t max_get_ch_state(max_config_t * dev, uint8_t ch) {
  return (dev->channel_state & (1 << ch)) != 0;
}

void max_push_ch_state(max_config_t * dev) {
  if (!dev->dirty) {
    // ESP_LOGD(TAG, "not updating, state not dirty");
    return;
  }
  // Write to MSB of status reg
  uint32_t status = max_reg(dev, 1, MAX22200_REG_STATUS, 1, /*st ? 0xff : 0);*/ dev->channel_state);
  // ESP_LOGI(TAG, "update ch state: %u, status[7:0]: %lu", dev->channel_state, status);
  dev->dirty = 0;
}
