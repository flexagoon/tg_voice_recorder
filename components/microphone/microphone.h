#pragma once

#include "freertos/idf_additions.h"

TaskHandle_t init_microphone(void);

#define NOTIF_INDEX_START 0
#define NOTIF_INDEX_STOP 1
