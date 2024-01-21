#include <Arduino.h>
#include <Wire.h>

#include <sys/param.h>

#include "driver/temp_sensor.h"
#include "driver/i2s.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#include "dns_server.h"
#include "wifi_handlers.h"
#include "server.h"

#include "DRV8847.h"
#include "shift_register.h"
#include "haptic.h"

#define DRIVER_SDA 21
#define DRIVER_SCL 45
#define nFAULT_1 13 // Labelled nFAULT5 in schematic, but physically connected to first driver
#define nFAULT_2 10
#define nFAULT_3  3 // Labelled nFAULT1 in sch
#define nFAULT_4  9 // Labelled nFAULT3 in sch
#define nFAULT_6 46 // Labelled nFAULT3 in sch

#define SHIFT_DATA 4
#define SHIFT_WS   5
#define SHIFT_BCLK 6

static const temp_sensor_config_t temp_config = TSENS_CONFIG_DEFAULT();

static const char *TAG = "main";

void setup() {
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
  // Enable non-blocking mode on stdin and stdout
  fcntl(fileno(stdout), F_SETFL, 0);
  fcntl(fileno(stdin), F_SETFL, 0);

  ESP_LOGI(TAG, "Begin!");

  pinMode(nFAULT_1, INPUT_PULLUP);
  pinMode(nFAULT_2, INPUT_PULLUP);
  pinMode(nFAULT_3, INPUT_PULLUP);
  pinMode(nFAULT_4, INPUT_PULLUP);
  pinMode(nFAULT_6, INPUT_PULLUP);

  pinMode(SHIFT_WS, OUTPUT);

  // Init shift registers
  shift_init(SHIFT_BCLK, SHIFT_WS, SHIFT_DATA);

  delay(2000);

  // Init I2C to drivers
  Wire.begin(DRIVER_SDA, DRIVER_SCL, 50000);
  // delay(1000);
  //Wire.setTimeOut(100);

  // Initialize and start internal temperature sensor
  ESP_ERROR_CHECK(temp_sensor_set_config(temp_config));
  ESP_ERROR_CHECK(temp_sensor_start());

  // Initialize networking stack
  ESP_ERROR_CHECK(esp_netif_init());

  // Create default event loop needed by the  main app
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

  start_haptic_task();

  uint8_t res;

  // pinMode(DRIVER_SCL, INPUT_PULLUP);
  delay(1);
  printf("i2c state: %d\n", digitalRead(DRIVER_SCL));

  ESP_LOGD(TAG, "States: fault=(%d, %d, %d, %d, %d), i2c=(%d, %d)", digitalRead(nFAULT_1), digitalRead(nFAULT_2), digitalRead(nFAULT_3), digitalRead(nFAULT_4), digitalRead(nFAULT_6), digitalRead(DRIVER_SDA), digitalRead(DRIVER_SCL));
  for (uint8_t addr = 0x60; addr <= 0x64; addr++) {
    // if (addr == 0x62) continue;
    if (res = drv_disable_nFAULT(addr)) {
      ESP_LOGE(TAG, "Driver %d disable nFAULT error: %d", addr, res);
    } else {
      ESP_LOGI(TAG, "Disabled nFAULT");
    }
    // delay(10);
  }
  ESP_LOGD(TAG, "States: fault=(%d, %d, %d, %d, %d)", digitalRead(nFAULT_1), digitalRead(nFAULT_2), digitalRead(nFAULT_3), digitalRead(nFAULT_4), digitalRead(nFAULT_6));
  pinMode(nFAULT_1, OUTPUT);
  pinMode(nFAULT_2, OUTPUT);
  pinMode(nFAULT_3, OUTPUT);
  pinMode(nFAULT_4, OUTPUT);
  pinMode(nFAULT_6, OUTPUT);
  digitalWrite(nFAULT_1, 0);
  digitalWrite(nFAULT_2, 1);
  digitalWrite(nFAULT_3, 0);
  digitalWrite(nFAULT_4, 0);
  digitalWrite(nFAULT_6, 0);
  // ESP_LOGD(TAG, "Test 0x60: %d", drv_test(0x60));
  drv_update_addr(0x60, 0x61);
  digitalWrite(nFAULT_2, 0);
  digitalWrite(nFAULT_3, 1);
  // ESP_LOGD(TAG, "Test 0x60: %d", drv_test(0x60));
  drv_update_addr(0x60, 0x62);
  digitalWrite(nFAULT_3, 0);
  digitalWrite(nFAULT_4, 1);
  // ESP_LOGD(TAG, "Test 0x60: %d", drv_test(0x60));
  drv_update_addr(0x60, 0x63);
  digitalWrite(nFAULT_4, 0);
  digitalWrite(nFAULT_6, 1);
  // ESP_LOGD(TAG, "Test 0x60: %d", drv_test(0x60));
  drv_update_addr(0x60, 0x64);
  /*digitalWrite(nFAULT_1, 1); // Enable first driver
  digitalWrite(nFAULT_2, 0); // Pull both low to release
  delay(1);
  printf("nFAULT1 up\n");
  printf("Test 0x60: %d\n", drv_test(0x60));
  printf("Test 0x61: %d\n", drv_test(0x61));
  digitalWrite(nFAULT_1, 0); // Enable second driver
  digitalWrite(nFAULT_2, 1); // Pull both low to release
  delay(1);
  printf("nFAULT2 up\n");
  printf("Test 0x60: %d\n", drv_test(0x60));
  printf("Test 0x61: %d\n", drv_test(0x61));*/
  pinMode(nFAULT_1, INPUT_PULLUP);
  pinMode(nFAULT_2, INPUT_PULLUP);
  pinMode(nFAULT_3, INPUT_PULLUP);
  pinMode(nFAULT_4, INPUT_PULLUP);
  pinMode(nFAULT_6, INPUT_PULLUP);
  for (uint8_t addr = 0x60; addr <= 0x64; addr++) {
    // if (addr == 0x62) continue;
    if ((res = drv_enable_nFAULT(addr))) {
      ESP_LOGE(TAG, "Driver %#04x enable nFAULT error: %d", addr, res);
    } else {
      ESP_LOGI(TAG, "Reenabled %#04x nFAULT", addr);
    }
  }
  ESP_LOGD(TAG, "States: fault=(%d, %d, %d, %d), i2c=(%d, %d)", digitalRead(nFAULT_1), digitalRead(nFAULT_2), digitalRead(nFAULT_3), digitalRead(nFAULT_4), digitalRead(DRIVER_SCL), digitalRead(DRIVER_SDA));
  ESP_LOGD(TAG, "Test 0x60: %d", drv_test(0x60));
  ESP_LOGD(TAG, "Test 0x61: %d", drv_test(0x61));
  ESP_LOGD(TAG, "Test 0x62: %d", drv_test(0x62));
  ESP_LOGD(TAG, "Test 0x63: %d", drv_test(0x63));
  ESP_LOGD(TAG, "Test 0x64: %d", drv_test(0x64));

  /*if (res = drv_update_addr(0x60, 0x61)) {
    ESP_LOGE(TAG, "Driver set addr error: %d", res);
  }
  printf("Test: %d\n", drv_test(0x61));
  digitalWrite(nFAULT_1, 1);
  digitalWrite(nFAULT_2, 0);
  printf("Test: %d\n", drv_test(0x60));
  // delay(1000);
  pinMode(nFAULT_1, INPUT_PULLUP);
  pinMode(nFAULT_2, INPUT_PULLUP);
  if (res = drv_enable_nFAULT(0x60)) {
    ESP_LOGE(TAG, "Driver enable nFAULT error: %d", res);
  }
  if (res = drv_enable_nFAULT(0x61)) {
    ESP_LOGE(TAG, "Driver enable nFAULT error: %d", res);
  }*/

  uint8_t ic2_data;
  if ((res = read_reg(0x60, 0x03, &ic2_data))) {
    ESP_LOGE(TAG, "Read res fail: %d", res);
  } else {
    ESP_LOGI(TAG, "Read IC2: %d", ic2_data);
  }

  /*static const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0, // default interrupt priority
    .dma_buf_count = 8,
    .dma_buf_len = 8,
    .use_apll = false
  };

  static const i2s_pin_config_t pin_config = {
    .bck_io_num = SHIFT_BCLK,
    .ws_io_num = SHIFT_WS,
    .data_out_num = SHIFT_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);   //install and start i2s driver
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_stop(I2S_NUM_0);

  // Use normal clock format, (WS is aligned with the last bit)
  I2S0.tx_conf1.tx_msb_shift = 0;
  I2S0.rx_conf1.rx_msb_shift = 0;

  // I2S0.conf.tx_right_first = 1;

  // auto stop when the fifo is empty
  I2S0.tx_conf.tx_stop_en = 1;

  // Disable TX interrupts
  I2S0.int_ena.tx_done = 0;
  I2S0.int_ena.tx_hung = 0;*/

  // Create the dma buffer
  /*volatile lldesc_t *dma = dmaDesc;
  dma->owner = 1; // Owner is the DMA controller
  dma->eof = 1;   // Eof of file, last item in the linked list
  dma->length = 4; // 4 bytes of data in the buffer, 32 bits
  dma->size = 4;
  dma->offset = 0;
  dma->sosf = 0;
  dma->buf = (uint8_t *)&data;
  dma->empty = 0;

  I2S0.out_link.addr = ((uint32_t)(&dmaDesc)) & I2S_OUTLINK_ADDR;*/
}

static uint8_t ret, cnt = 0;
static float temp = 0;
static char status_msg[12];

void loop() {
  if ((ret = temp_sensor_read_celsius(&temp)) != ESP_OK) {
    ESP_LOGE(TAG, "Temp read error: %d", ret);
  }
  int size = snprintf(status_msg, sizeof(status_msg), "temp=%.1f", temp);
  httpd_ws_frame_t pkt = {
    .type = HTTPD_WS_TYPE_TEXT,
    .payload = (uint8_t *) status_msg,
    .len = size+1
  };
  ws_frame_send_all(&pkt);
  ESP_LOGI(TAG, "tick=%d, temp=%.1f", xTaskGetTickCount(), temp);
  // ++cnt;
  delay(500);
}
