#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_event.h"

#include "esp_http_server.h"

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void wifi_init_softap(void);

#ifdef __cplusplus
}
#endif