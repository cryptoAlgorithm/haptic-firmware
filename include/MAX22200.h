#pragma once

#include <stdint.h>

#define MAX22200_MASK_STATUS_ACTIVE (1<<0)
#define MAX22200_MASK_STATUS_UVM    (1<<1)

/**
 * Configuration for a single channel
 */
typedef struct __attribute__((packed)) {
  /**
   * If HIT current reached diagnostic is enabled (1=en)
   */
  uint8_t hhf_en: 1;
  /**
   * If DPM fault diagnostic is enabled (1=en)
   */
  uint8_t dpm_en: 1;
  /**
   * If Open-load diagnostic is enabled (1=en)
   */
  uint8_t ol_en: 1;
  /**
   * Slew Rate Control (SRC)
   * 
   * 0x0: Fast OUT transitions
   * 0x1: OUT transition is slew-rate controlled in lowside mode
   */
  uint8_t src: 1;
  /**
   * Chopping frequency divisor
   * 
   * 0x0: FreqMain/4
   * 0x1: FreqMain/3
   * 0x2: FreqMain/2
   * 0x3: FreqMain
   */
  uint8_t freq: 2;
  /**
   * High or low side drive mode
   */
  uint8_t HSnLS: 1;
  /**
   * Current/voltage controlled drive selection
   * 
   * 0x0: CH_ is controlled in Current-Drive mode
   * 0x1: CH_ is controlled Voltage-Drive mode
   */
  uint8_t VDRnCDR: 1;
  /**
   * HIT time
   * 
   * 0: No HIT time
   * 1-254: THIT = HIT_T_[7:0] x 40 / fCHOP
   * 255: Continous IHIT
   */
  uint8_t hit_t: 8;
  /**
   * HIT current
   *
   * 0: HS off, LS on (in HS mode)
   * HS on, LS off (in LS mode)
   * 1-126: VDR duty cycle setting (in HS mode); VDR duty cycle or CDR current setting (in LS mode)
   * 127: HS on, LS off (in HS mode); HS off, LS on (in SL mode)
   */
  uint8_t hit: 7;
  /**
   * TRIG/SPI control selection
   *
   * 0x0: CH_ is controlled by ONCH_ SPI bit
   * 0x1: CH_ is controlled by TRIG_ pin
   */
  uint8_t TRGnSPI: 1;
  /**
   * Hold current
   *
   * 0: HS off, LS on (in HS mode); HS on, LS off (in LS mode)
   * 1-126: VDR duty cycle setting (in HS mode); VDR duty cycle or CDR current setting (in LS mode)
   * 127: HS on, LS off (in HS mode); HS off, LS on (in LS mode)
   */
  uint8_t hold: 7;
  /**
   * Full-Scale/Half-Full-Scale selection
   * 
   * 0x0: 1
   * 0x1: 0.5
   */
  uint8_t hfs: 1;
} max_ch_config_t;

typedef struct {
  uint8_t cmd;
  uint8_t cs;
  uint8_t fault;
  /**
   * Config that will be applied to all channels
   */
  max_ch_config_t * ch_cfg;
  /**
   * Bitfield representing state of all 8 output channels
   * 
   * Do not modify this directly - use `max_update_ch_state(...)` instead
   */
  uint8_t channel_state;
} max_config_t;

extern max_config_t max_driver_a;
extern max_config_t max_driver_b;

void max_init();
void max_set_ch_state(max_config_t *, uint8_t ch, uint8_t st);
