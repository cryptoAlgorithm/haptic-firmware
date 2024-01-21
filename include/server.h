#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_http_server.h"

httpd_handle_t start_webserver(void);

esp_err_t ws_frame_send_all(httpd_ws_frame_t *ws_pkt);

#ifdef __cplusplus
}
#endif