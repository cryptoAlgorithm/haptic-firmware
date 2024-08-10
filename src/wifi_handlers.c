#include "wifi_handlers.h"

#include "esp_log.h"
#include "esp_system.h"

#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"

// Increase log level
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL 5

#define EXAMPLE_ESP_WIFI_SSID "HapticDriver"
#define EXAMPLE_ESP_WIFI_PASS "Student01"
#define EXAMPLE_MAX_STA_CONN 2

// Not using ESP_LOG in event handlers because will lead to crash
#define TAG "[AP] "

// Increase wifi task stack size to support advanced logging.
static int32_t task_create_pinned_to_core_wrapper_mgos(
    void *task_func, const char *name, uint32_t stack_depth, void *param,
    uint32_t prio, void *task_handle, uint32_t core_id) {
  if (stack_depth < 4608) stack_depth = 4096;
  return xTaskCreatePinnedToCore(
      task_func, name, stack_depth, param, prio, task_handle,
      (core_id < portNUM_PROCESSORS ? core_id : tskNO_AFFINITY));
}

static int32_t task_create_wrapper_mgos(void *task_func, const char *name,
                                        uint32_t stack_depth, void *param,
                                        uint32_t prio, void *task_handle) {
  if (stack_depth < 4608) stack_depth = 4096;
  return xTaskCreate(task_func, name, stack_depth, param, prio, task_handle);
}

void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        //ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
        //         MAC2STR(event->mac), event->aid);
        //printf(TAG "station " MACSTR " join, AID=%d\n",
        //            MAC2STR(event->mac), event->aid);
       // ESP_LOGI(TAG, "station join, AID=%d", event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        //ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
        //         MAC2STR(event->mac), event->aid);
        //printf(TAG "station " MACSTR " leave, AID=%d\n",
        //           MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(void) {
    g_wifi_osi_funcs._task_create = task_create_wrapper_mgos;
    g_wifi_osi_funcs._task_create_pinned_to_core = task_create_pinned_to_core_wrapper_mgos;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = EXAMPLE_MAX_STA_CONN
        }
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:'%s' password:'%s'",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}