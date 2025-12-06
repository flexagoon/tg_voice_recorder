#include "esp_check.h"
#include "microphone.h"
#include "sd.h"
#include "telegram.h"
#include "wifi.h"
#include <stdio.h>

static const char *TAG = "tgvr";

void app_main(void) {
  init_microphone();
  mount_sd("/data");
  connect_wifi();

  FILE *file = fopen("/data/voice.ogg", "rb");
  ESP_RETURN_VOID_ON_FALSE(file != NULL, TAG, "Failed to open file");

  send_voice(file);
  fclose(file);
}
