#include "esp_log.h"
#include "sd.h"
#include "telegram.h"
#include "wifi.h"
#include <stdio.h>

static const char *TAG = "tgvr";

void app_main(void) {
  connect_wifi();
  mount_sd("/data");

  FILE *file = fopen("/data/voice.ogg", "rb");
  if (file == NULL) {
    ESP_LOGE(TAG, "Failed to open file");
    return;
  }

  send_voice(file);
  fclose(file);
}
