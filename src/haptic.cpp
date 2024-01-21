#include "haptic.h"

#include <FreeRTOS.h>
#include <task.h>

#include "esp_log.h"

#include "shift_register.h"

#define IN1 12
#define IN2 13
#define IN3 15
#define IN4 14

static const char * TAG = "haptic";

static const haptic_output outputs[] = {{
  .inA = IN1,
  .inB = IN2
}, {
  .inA = IN3,
  .inB = IN4
}};

static void haptic_task(void *) {
  for (;;) {
    // Turn on
    shift_set_output(IN1, 1);
    shift_set_output(IN3, 1);
    shift_update();
    ESP_LOGD(TAG, "Bridge on");
    vTaskDelay(pdMS_TO_TICKS(100));
    shift_set_output(IN1, 0);
    shift_set_output(IN3, 0);
    shift_update();
    ESP_LOGD(TAG, "Bridge off");
    vTaskDelay(pdMS_TO_TICKS(2000));
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
