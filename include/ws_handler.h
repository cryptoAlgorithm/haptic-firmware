#pragma "once"

#include "haptic.h"

#include "esp_http_server.h"

#define OUTPUT_UPDATE_PREAMBLE 0xaf

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
    // Check if first byte matches update preamble
    if (pkt->len == haptic_num_outputs+1 && pkt->payload[0] == 0xaf) {
      haptic_update_delays(pkt->payload+1);
    }
    break;
  case HTTPD_WS_TYPE_TEXT:
    /*switch (pkt->payload[0]) {
    case '1':
      ESP_LOGI("WS", "pong");
      return ws_send_text(req, "1");
    }*/
    break;
  // Control frame handling - we have to do this ourselves in order to 
  case HTTPD_WS_TYPE_PING:
    // echo it back as a pong packet
    pkt->type = HTTPD_WS_TYPE_PONG;
    return httpd_ws_send_frame(req, pkt);
  case HTTPD_WS_TYPE_CLOSE: {
    // TODO: See if we have any more clients, only stop haptic feedback when none remaining
    haptic_stop();
    // Respond with close frame accordingly
    httpd_ws_frame_t frame = {
      .len = 0,
      .payload = NULL,
      .type = HTTPD_WS_TYPE_CLOSE
    };
    return httpd_ws_send_frame(req, &frame);
  }
  }
  /*if (!strncmp((char *) cmp_buf, "1", pkt->len)) {
    return ws_send_text(req, "1");
  }*/
  return ESP_OK;
}