#pragma "once"

#include "esp_http_server.h"

static esp_err_t ws_send_text(httpd_req_t * req, const char * str) {
  return httpd_ws_send_frame(req, &(httpd_ws_frame_t) {
    .payload = (uint8_t *) str,
    .len = strlen(str),
    .type = HTTPD_WS_TYPE_TEXT
  });
}

static esp_err_t ws_handle_frame(httpd_req_t * req, httpd_ws_frame_t * pkt) {
  if (pkt->len == 0) return ESP_OK;

  switch (pkt->type) {
  case HTTPD_WS_TYPE_BINARY:
    // 
    break;
  case HTTPD_WS_TYPE_TEXT:
    switch (pkt->payload[0]) {
    case '1':
      return ws_send_text(req, "1");
    }
  }
  /*if (!strncmp((char *) cmp_buf, "1", pkt->len)) {
    return ws_send_text(req, "1");
  }*/
  return ESP_OK;
}