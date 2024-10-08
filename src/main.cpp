#include <Arduino.h>
#include <Wire.h>

#include <sys/param.h>

#include "driver/temp_sensor.h"
#include "driver/i2s.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#include "pins.h"

#include "dns_server.h"
#include "wifi_handlers.h"
#include "server.h"

#include "battery.h"
#include "MAX22200.h"
#include "haptic.h"

static const temp_sensor_config_t temp_config = TSENS_CONFIG_DEFAULT();

static const char *TAG = "main";

void setup() {
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
  // Enable non-blocking mode on stdin and stdout
  fcntl(fileno(stdout), F_SETFL, 0);
  fcntl(fileno(stdin), F_SETFL, 0);

	esp_log_level_set("*", ESP_LOG_VERBOSE);

  battery_init();

  // For debugging - check if powered via usb only and add short delay
  if (battery_getmv() < 5100) { // USB shouldn't exceed this
    delay(2000); // Give some time for virtual serial to connect
    ESP_LOGD(TAG, "vBatt low, likely USB power only");
  }

  ESP_LOGI(TAG, "Begin!");

  // Initialize and start internal temperature sensor
  ESP_ERROR_CHECK(temp_sensor_set_config(temp_config));
  ESP_ERROR_CHECK(temp_sensor_start());

  // Init SPI to drivers
  max_init();

  // Initialize networking stack
  ESP_ERROR_CHECK(esp_netif_init());

  // Create default event loop needed by the main app
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Initialize NVS needed by Wi-Fi
  ESP_ERROR_CHECK(nvs_flash_init());

  // Initialize Wi-Fi including netif with default config
  esp_netif_create_default_wifi_ap();

  // Initialise ESP32 in SoftAP mode
  wifi_init_softap();

  // Start the server for the first time
  start_webserver();

  // Start the DNS server that will redirect all queries to the softAP IP
  // start_dns_server();

  // Test all outputs
  for (uint8_t ch = 0; ch < haptic_num_outputs; ++ch) {
    haptic_set_state(ch, 1, 1);
    if (ch > 0) haptic_set_state(ch-1, 0, 1);
    delay(50);
  }
  haptic_set_state(haptic_num_outputs-1, 0, 1);

  start_haptic_task();
}

void loop() {
  static uint8_t ret, cnt = 0, ch = 0;
  static float temp = 0;
  static char status_msg[64];
  static uint32_t vBatt;

  if ((ret = temp_sensor_read_celsius(&temp)) != ESP_OK) {
    ESP_LOGE(TAG, "Temp read error: %d", ret);
  }
  vBatt = battery_getmv();
  size_t size = snprintf(status_msg, sizeof(status_msg), "2,%.1f,%lu", temp, vBatt);
  httpd_ws_frame_t pkt = {
    .type = HTTPD_WS_TYPE_TEXT,
    .payload = (uint8_t *) status_msg,
    .len = size // snprintf does not include null terminator in length
  };
  ws_frame_send_all(&pkt);
  ESP_LOGI(TAG, "temp=%.1f", temp);
  delay(1000);
}
