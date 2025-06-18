#include "wifi.h"

#include "esp_event.h"
#include "esp_log.h"
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
    abort();
  }
}

static void handle_connection(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data) {
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
  connection_attempts = 0;
  if (connect_notify_task != NULL) {
    xTaskNotifyGive(connect_notify_task);
  }
}

void connect_wifi(void) {
  connect_notify_task = xTaskGetCurrentTaskHandle();

  // CONFIG_ESP_WIFI_NVS_ENABLED is turned off, but ESP-IDF still complains if
  // NVS is not initialized before wifi.
  nvs_flash_init();
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

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
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_wifi_start();

  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  ESP_LOGI(TAG, "connected to ap:%s", SSID);

  connect_notify_task = NULL;
}
