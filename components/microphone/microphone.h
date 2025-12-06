#pragma once

#include "esp_err.h"
#include <stdio.h>

void init_microphone(void);
esp_err_t record_audio(FILE *file, int duration_sec);
