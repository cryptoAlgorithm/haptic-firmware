#include "haptic.h"

#include <string.h>
#include <FreeRTOS.h>
#include <task.h>

#include "esp_log.h"

// Increase log level
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL 5

static const char * TAG = "haptic";

#define ON_CYCLES 6 // approx 6*10 = 60ms

static const haptic_output outputs[] = { /* row 0 */ {
  .driver = &max_driver_b,
  .ch = 5
}, {
  .driver = &max_driver_b,
  .ch = 7
}, {
  .driver = &max_driver_a,
  .ch = 0
}, /* row 1 */ {
  .driver = &max_driver_b,
  .ch = 2
}, {
  .driver = &max_driver_a,
  .ch = 3
}, {
  .driver = &max_driver_a,
  .ch = 6
}, /* row 2 */ {
  .driver = &max_driver_b,
  .ch = 1
}, {
  .driver = &max_driver_b,
  .ch = 6
}, {
  .driver = &max_driver_a,
  .ch = 5
}, /* row 3 */ {
  .driver = &max_driver_b,
  .ch = 4
}, {
  .driver = &max_driver_a,
  .ch = 1
}, {
  .driver = &max_driver_a,
  .ch = 4
}};

#define NUM_OUTPUTS (sizeof(outputs)/sizeof(haptic_output))
const uint8_t haptic_num_outputs = NUM_OUTPUTS;

/**
 * Feedback frequency per output.
 * 
 * Each increment corresponds to an additional ~10ms of delay between actuations.
 * Set to 0 to disable.
 */
static uint8_t output_delays[NUM_OUTPUTS] = { 0 };

/**
 * Counters of each state's delay
 * 
 * Output state will be flipped when the counter reaches 0 (if the output isn't
 * disabled).
 */
static uint8_t output_counters[NUM_OUTPUTS] = { 0 };

void haptic_update_delays(uint8_t * delays) {
  for (uint8_t i = 0; i < NUM_OUTPUTS; ++i) {
    if (delays[i] <= ON_CYCLES && delays[i] != 0) delays[i] = ON_CYCLES+1; // Enforce minimum delay between cycles if input malformed
  }
  memcpy(output_delays, delays, NUM_OUTPUTS);
  // ESP_LOGI(TAG, "Updated output delays");
  ESP_LOG_BUFFER_HEX(TAG, output_delays, NUM_OUTPUTS);
}

void haptic_stop() {
  ESP_LOGI(TAG, "stopping all output chs");
  memset(output_delays, 0, NUM_OUTPUTS);
}

static uint8_t haptic_get_state(uint8_t out_n) {
  if (out_n >= haptic_num_outputs) { // out of range
    ESP_LOGE(TAG, "supplied out of range haptic output: %u", out_n);
    return 0;
  }
  const haptic_output * out = &outputs[out_n];
  return max_get_ch_state(out->driver, out->ch);
}

static void haptic_push_state() {
  // attempt push for both drivers because we don't know which ones are actually dirty - outputs span both
  max_push_ch_state(&max_driver_a);
  max_push_ch_state(&max_driver_b);
}

void haptic_set_state(uint8_t out_n, uint8_t st, uint8_t update) {
  if (out_n >= haptic_num_outputs) { // out of range
    ESP_LOGE(TAG, "supplied out of range haptic output: %u", out_n);
    return;
  }
  const haptic_output * out = &outputs[out_n];
  max_set_ch_state(out->driver, out->ch, st, 0);
  if (update) {
    haptic_push_state();
  }
}

static void haptic_task(void * param) {
  uint8_t i, st;
  for (;;) {
    for (i = 0; i < NUM_OUTPUTS; ++i) {
      if (output_counters[i] == 0) {
        if (output_delays[i] == 0) continue;
        output_counters[i] = output_delays[i];
      }
      // Decrement each counter; Flip output whenever it reaches 0
      if (--output_counters[i] == 0) {
        st = haptic_get_state(i);
        if (st) {
          haptic_set_state(i, 0, 0);
          // shift_set_output(outputs[i].inA, 0);
          output_counters[i] = output_delays[i] == 0 ? 0 : (output_delays[i]-ON_CYCLES); // Switch on again after delay
          // ESP_LOGD(TAG, "Output %d off", i);
        } else {
          haptic_set_state(i, 1, 0);
          // shift_set_output(outputs[i].inA, 1);
          output_counters[i] = ON_CYCLES; // Start counting down and disable output after 100ms
          // ESP_LOGD(TAG, "Output %d on", i);
        }
      }
    }
    haptic_push_state();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  vTaskDelete(NULL);
  // if (cnt == 10) {
  /*  cnt = 0;
  } else {
    // Turn off
    shift_set_output(IN1, 0);
    shift_set_output(IN3, 0);
    shift_update();
    ESP_LOGD(TAG, "Bridge off");
  }*/
  
}

void start_haptic_task() {
  xTaskCreate(haptic_task, "haptic", 4096, NULL, 5, NULL);
}
