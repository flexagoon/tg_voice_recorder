#include "telegram.h"

#include "esp_check.h"
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

esp_err_t send_voice(FILE *file) {
  esp_http_client_config_t http_client_cfg = {
      .url = "https://api.telegram.org/"
             "bot" BOT_TOKEN "/sendVoice",
      .cert_pem = (const char *)telegram_api_cert,
      .keep_alive_enable = true,
  };
  esp_http_client_handle_t client = esp_http_client_init(&http_client_cfg);
  ESP_RETURN_ON_FALSE(client != NULL, ESP_FAIL, TAG,
                      "Failed to initialize HTTP client");

  ESP_ERROR_CHECK(
      esp_http_client_set_header(client, "Content-Type", CONTENT_TYPE));

  const long file_size = get_file_size(file);

  const long total_request_size =
      strlen(REQUEST_HEAD) + file_size + strlen(REQUEST_TAIL);
  ESP_RETURN_ON_ERROR(esp_http_client_open(client, total_request_size), TAG,
                      "Failed to open HTTP stream: %s",
                      esp_err_to_name(err_rc_));

  int bytes_written;

  bytes_written =
      esp_http_client_write(client, REQUEST_HEAD, strlen(REQUEST_HEAD));
  ESP_RETURN_ON_FALSE(bytes_written != -1, ESP_ERR_HTTP_WRITE_DATA, TAG,
                      "Failed to write request head");

  size_t bytes_read;
  char buffer[1024];
  while ((bytes_read = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
    bytes_written = esp_http_client_write(client, buffer, bytes_read);
    ESP_RETURN_ON_FALSE(bytes_written != -1, ESP_ERR_HTTP_WRITE_DATA, TAG,
                        "Failed to write file chunk");
    ESP_LOGI(TAG, "Wrote %d bytes", bytes_read);
  }
  ESP_LOGI(TAG, "Writing complete");

  bytes_written =
      esp_http_client_write(client, REQUEST_TAIL, strlen(REQUEST_TAIL));
  ESP_RETURN_ON_FALSE(bytes_written != -1, ESP_ERR_HTTP_WRITE_DATA, TAG,
                      "Failed to write request tail");

  ESP_ERROR_CHECK(esp_http_client_close(client));
  ESP_ERROR_CHECK(esp_http_client_cleanup(client));

  return ESP_OK;
}
