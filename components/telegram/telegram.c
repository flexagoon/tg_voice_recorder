#include "esp_http_client.h"
#include "esp_log.h"
#include <stdio.h>

#define BOT_TOKEN CONFIG_BOT_TOKEN
#define CHAT_ID CONFIG_CHAT_ID

extern const uint8_t telegram_api_cert[] asm("_binary_telegram_api_pem_start");

static const char *TAG = "tgvr_telegram";

#define MULTIPART_BOUNDARY "9cc57f2ca39c2d8"
static const char *CONTENT_TYPE =
    "Content-Type: multipart/form-data; boundary=" MULTIPART_BOUNDARY;
static const char *REQUEST_HEAD =
    "--" MULTIPART_BOUNDARY "\r\n"
    "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" CHAT_ID "\r\n"
    "--" MULTIPART_BOUNDARY "\r\n"
    "Content-Disposition: form-data; name=\"voice\"; "
    "filename=\"voice.ogg\"\r\n\r\n";
static const char *REQUEST_TAIL = "\r\n--" MULTIPART_BOUNDARY "--";

static long get_file_size(FILE *file) {
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  rewind(file);
  return size;
}

void send_voice(FILE *file) {
  esp_http_client_config_t http_client_cfg = {
      .url = "https://api.telegram.org/"
             "bot" BOT_TOKEN "/sendVoice",
      .cert_pem = (const char *)telegram_api_cert,
      .keep_alive_enable = true,
      .timeout_ms = 25000,
  };
  esp_http_client_handle_t client = esp_http_client_init(&http_client_cfg);

  esp_http_client_set_header(client, "Content-Type", CONTENT_TYPE);

  long file_size = get_file_size(file);

  long total_request_size =
      strlen(REQUEST_HEAD) + file_size + strlen(REQUEST_TAIL);
  esp_http_client_open(client, total_request_size);

  esp_http_client_write(client, REQUEST_HEAD, strlen(REQUEST_HEAD));

  size_t bytes_read;
  char buffer[1024];
  while ((bytes_read = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
    esp_http_client_write(client, buffer, bytes_read);
    ESP_LOGI(TAG, "Wrote %d bytes", bytes_read);
  }

  ESP_LOGI(TAG, "Writing done");
  esp_http_client_write(client, REQUEST_TAIL, strlen(REQUEST_TAIL));
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
}
