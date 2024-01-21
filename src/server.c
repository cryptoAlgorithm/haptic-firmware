#include "server.h"

#include "esp_log.h"

#include "ws_handler.h"

static const char *TAG = "SRV";

#define MAX_OPEN_SOCKETS 10

// HTTP GET Handler
static esp_err_t root_get_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "Serve root");
  httpd_resp_set_type(req, "text/html");
  httpd_resp_sendstr(req, "<html><head><title>HapticDriver</title></head><body><h1>Rarara</h1></body></html>");

  return ESP_OK;
}

static uint8_t recv_buf[12];

static esp_err_t ws_handler(httpd_req_t * req) {
  esp_err_t ret;
  if (req->method == HTTP_GET) {
    ESP_LOGI(TAG, "New WS connection handshake complete");
    // Send a hello
    if ((ret = ws_send_text(req, "hello")) != ESP_OK) {
      ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
    }
    return ret;
  }

  // Else, we should handle this websocket packet
  httpd_ws_frame_t pkt = { .payload = recv_buf };
  memset(recv_buf, 0, sizeof(recv_buf));
  if ((ret = httpd_ws_recv_frame(req, &pkt, sizeof(recv_buf))) != ESP_OK) {
    ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
    return ret;
  }
  ESP_LOGI(TAG, "frame len is %d", pkt.len);
  return ws_handle_frame(req, &pkt);
}

static const httpd_uri_t root = {
  .uri = "/",
  .method = HTTP_GET,
  .handler = root_get_handler
};
static const httpd_uri_t ws_server = {
  .uri = "/ws",
  .is_websocket = true,
  .method       = HTTP_GET,
  .handler = ws_handler
};

// HTTP Error (404) Handler - Redirects all requests to the root page
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
  // Set status
  httpd_resp_set_status(req, "302 Temporary Redirect");
  // Redirect to the "/" root directory
  httpd_resp_set_hdr(req, "Location", "/");
  // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
  httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

  ESP_LOGI(TAG, "Redirecting to root");
  return ESP_OK;
}

static httpd_handle_t server = NULL;

esp_err_t ws_frame_send_all(httpd_ws_frame_t *ws_pkt) {
  static size_t max_clients = MAX_OPEN_SOCKETS;
  size_t fds = max_clients;
  int client_fds[max_clients];

  esp_err_t ret = httpd_get_client_list(server, &fds, client_fds);

  if (ret != ESP_OK) {
    return ret;
  }

  for (int i = 0; i < fds; i++) {
    httpd_ws_client_info_t client_info = httpd_ws_get_fd_info(server, client_fds[i]);
    if (client_info == HTTPD_WS_CLIENT_WEBSOCKET) {
      httpd_ws_send_frame_async(server, client_fds[i], ws_pkt);
    }
  }

  return ESP_OK;
}

httpd_handle_t start_webserver(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_open_sockets = MAX_OPEN_SOCKETS;
  config.lru_purge_enable = true;

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &ws_server);
    httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
  }
  return server;
}