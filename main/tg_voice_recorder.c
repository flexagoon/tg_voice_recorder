#include "button.h"
#include "esp_log.h"
#include "microphone.h"
#include "sd.h"
#include "wifi.h"
#include <stdio.h>
#include <sys/stat.h>

static const char *TAG = "tgvr";

static TaskHandle_t mic_task = NULL;

void on_press(void) {
  ESP_LOGI(TAG, "Button pressed");
  vTaskNotifyGiveIndexedFromISR(mic_task, NOTIF_INDEX_START, NULL);
}

void on_release(void) {
  ESP_LOGI(TAG, "Button released");
  vTaskNotifyGiveIndexedFromISR(mic_task, NOTIF_INDEX_STOP, NULL);
}

void app_main(void) {
  mount_sd();
  // connect_wifi();
  // init_sntp();
  mic_task = init_microphone();
  init_button(on_press, on_release);
}
