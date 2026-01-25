#include "wifi.h"

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define SSID CONFIG_SSID
#define PASSWORD CONFIG_WIFI_PASSWORD

static const int MAX_CONNECTION_ATTEMPTS = 5;

static const char *TAG = "tgvr_wifi";

static TaskHandle_t connect_notify_task = NULL;
static int connection_attempts = 0;

static void try_to_connect(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data) {
  if (connection_attempts < MAX_CONNECTION_ATTEMPTS) {
    ESP_LOGI(TAG, "Trying to connect to the AP, attempt %d",
             connection_attempts);
    esp_wifi_connect();
    connection_attempts++;
  } else {
    ESP_LOGE(TAG, "Failed to connect to the AP, aborting");
    abort();
  }
}

static void handle_connection(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data) {
  const ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
  connection_attempts = 0;
  if (connect_notify_task != NULL) {
    xTaskNotifyGive(connect_notify_task);
  }
}

esp_err_t connect_wifi(void) {
  connect_notify_task = xTaskGetCurrentTaskHandle();

  // CONFIG_ESP_WIFI_NVS_ENABLED is turned off, but ESP-IDF still complains if
  // NVS is not initialized before wifi.
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &try_to_connect,
                             NULL);
  esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                             &try_to_connect, NULL);

  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &handle_connection,
                             NULL);

  wifi_config_t wifi_config = {.sta = {
                                   .ssid = SSID,
                                   .password = PASSWORD,
                               }};
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG,
                      "Failed to set wifi config: %s",
                      esp_err_to_name(err_rc_));

  ESP_ERROR_CHECK(esp_wifi_start());

  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  ESP_LOGI(TAG, "connected to ap:%s", SSID);

  connect_notify_task = NULL;

  return ESP_OK;
}

esp_err_t init_sntp(void) {
  ESP_LOGI(TAG, "Initializing SNTP");
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  esp_netif_sntp_init(&config);
  esp_err_t result = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000));
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to synchronize time: %s", esp_err_to_name(result));
    return result;
  }
  ESP_LOGI(TAG, "SNTP time synchronized");
  return ESP_OK;
}
