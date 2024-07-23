#include "haptic.h"

#include <string.h>
#include <FreeRTOS.h>
#include <task.h>

#include "esp_log.h"

// Increase log level
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL 5

#define IN1_1 12
#define IN1_2 13
#define IN1_3 15
#define IN1_4 14
#define IN2_1 20
#define IN2_2 21
#define IN2_3 23
#define IN2_4 22
#define IN3_1 0
#define IN3_2 3
#define IN3_3 4
#define IN3_4 5

static const char * TAG = "haptic";

static const haptic_output outputs[] = {{
  .inA = IN1_1,
  .inB = IN1_2
}, {
  .inA = IN1_3,
  .inB = IN1_4
}, {
  .inA = IN2_1,
  .inB = IN2_2
}, {
  .inA = IN2_3,
  .inB = IN2_4
}, {
  .inA = IN3_1,
  .inB = IN3_2
}, {
  .inA = IN3_3,
  .inB = IN3_4
}};

#define NUM_OUTPUTS (sizeof(outputs)/sizeof(haptic_output))
const uint8_t haptic_num_outputs = NUM_OUTPUTS;

/**
 * Feedback frequency per output.
 * 
 * Each increment corresponds to an additional ~20ms of delay between actuations.
 * Set to 0 to disable.
 */
static uint8_t output_delays[NUM_OUTPUTS] = { 100, 50 };

/**
 * Counters of each state's delay
 * 
 * Output state will be flipped when the counter reaches 0 (if the output isn't
 * disabled).
 */
static uint8_t output_counters[NUM_OUTPUTS] = { 0 };

void haptic_update_delays(uint8_t * delays) {
  memcpy(output_delays, delays, NUM_OUTPUTS);
  // ESP_LOGI(TAG, "Updated output delays");
  ESP_LOG_BUFFER_HEX(TAG, output_delays, NUM_OUTPUTS);
}

static void haptic_task(void * param) {
  uint8_t i, st, dirty = 0;
  for (;;) {
    for (i = 0; i < NUM_OUTPUTS; ++i) {
      if (output_counters[i] == 0) {
        if (output_delays[i] == 0) continue;
        output_counters[i] = output_delays[i];
      }
      // Decrement each counter; Flip output whenever it reaches 0
      if (--output_counters[i] == 0) {
        // st = shift_get_output(outputs[i].inA);
        if (st) {
          // shift_set_output(outputs[i].inA, 0);
          output_counters[i] = output_delays[i]; // Switch on again after delay
          // ESP_LOGD(TAG, "Output %d off", i);
        } else {
          // shift_set_output(outputs[i].inA, 1);
          output_counters[i] = 5; // Start counting down and disable output after 100ms
          // ESP_LOGD(TAG, "Output %d on", i);
        }
        dirty = 1;
      }
    }
    if (dirty) {
      // shift_update();
      dirty = 0;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
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
