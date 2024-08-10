#include "battery.h"

#include "esp_log.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"

// Increase log level
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL 5
#define TAG "BATT"

#define VBATT_FACTOR 17 // voltage divider factor

#define ADC_ATTEN ADC_ATTEN_DB_0 // ADC attenuation
#define ADC_CH ADC1_CHANNEL_6

static esp_adc_cal_characteristics_t adc1_chars;

static esp_err_t adc_calibration_init(void) {
  esp_err_t ret;
  // bool cali_enable = false;

  ret = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP_FIT);
  if (ret == ESP_ERR_NOT_SUPPORTED) {
    ESP_LOGW(TAG, "Calibration scheme not supported, skip software calibration");
  } else if (ret == ESP_ERR_INVALID_VERSION) {
    ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
  } else if (ret == ESP_OK) {
    // cali_enable = true;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
  } else {
    ESP_LOGE(TAG, "Invalid arg");
  }

  return ret;
}

void battery_init() {
  ESP_ERROR_CHECK(adc_calibration_init());
  ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
  ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_CH, ADC_ATTEN));
}

uint32_t battery_getmv() {
  int raw = adc1_get_raw(ADC_CH);
  uint32_t volt = esp_adc_cal_raw_to_voltage(raw, &adc1_chars) * VBATT_FACTOR;
  ESP_LOGI(TAG, "raw: %d, volt: %lu", raw, volt);
  return volt;
}