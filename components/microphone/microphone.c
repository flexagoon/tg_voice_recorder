#include "microphone.h"

#include "driver/i2s_std.h"
#include "esp_log.h"
#include "format_wav.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"

#include <errno.h> // IWYU pragma: keep
#include <string.h>
#include <time.h>

#define SCK CONFIG_SCK
#define WS CONFIG_WS
#define SD CONFIG_SD
#define MNT CONFIG_MNT_PATH

#define SAMPLE_RATE CONFIG_SAMPLE_RATE
#define BYTES_PER_SEC (SAMPLE_RATE * 32 / 8)
#define BUFFER_SIZE (BYTES_PER_SEC * CONFIG_WRITE_FREQ_MS / 1000)
#define FILTER_BITS CONFIG_FILTER_BITS

static const char *TAG = "tgvr_microphone";

static i2s_chan_handle_t channel = NULL;

void audio_record_task() {
  while (1) {
    ulTaskNotifyTakeIndexed(NOTIF_INDEX_START, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Audio recording started");

    const time_t now = time(NULL);
    char filename[sizeof(MNT "/YYYY-MM-DD_HH-MM-SS.wav")];
    strftime(filename, sizeof(filename), MNT "/%F_%H-%M-%S.wav", gmtime(&now));
    ESP_LOGI(TAG, "Recording audio to %s", filename);

    FILE *file = fopen(filename, "wb+");
    if (file == NULL) {
      ESP_LOGE(TAG, "Failed to open temporary raw audio file: %d", errno);
      continue;
    }

    const wav_header_t placeholder_wav_header =
        WAV_HEADER_PCM_DEFAULT(0, 16, SAMPLE_RATE, 1);
    fwrite(&placeholder_wav_header, sizeof(placeholder_wav_header), 1, file);

    size_t bytes_written = 0;
    while (ulTaskNotifyTakeIndexed(NOTIF_INDEX_STOP, pdFALSE, 0) == 0) {
      size_t bytes_read;
      static int32_t buffer[BUFFER_SIZE];
      static int16_t converted_samples[BUFFER_SIZE / 2];
      if (i2s_channel_read(channel, &buffer, BUFFER_SIZE, &bytes_read,
                           portMAX_DELAY) == ESP_OK) {
        // For some reason, samples need to be recorded as 32-bit and then
        // converted to 16-bit. No idea why, but this works, and I don't feel
        // like debugging audio signal and bit alignment issues.
        // https://esp32.com/viewtopic.php?t=15185
        int samples_read = bytes_read / sizeof(int32_t);
        for (int i = 0; i < samples_read; i++) {
          // Make the sound less noisy by removing the lowest bits.
          converted_samples[i] = (int16_t)(buffer[i] >> FILTER_BITS);
        }
        int converted_bytes = samples_read * sizeof(int16_t);
        fwrite(converted_samples, converted_bytes, 1, file);
        bytes_written += converted_bytes;
      } else {
        ESP_LOGW(TAG, "I2S read timeout");
      }
    }

    ESP_LOGI(TAG, "Finalizing audio recording");

    // Update the WAV header with correct file size.
    fseek(file, 0, SEEK_SET);
    wav_header_t final_wav_header =
        WAV_HEADER_PCM_DEFAULT(bytes_written, 16, SAMPLE_RATE, 1);
    fwrite(&final_wav_header, sizeof(final_wav_header), 1, file);

    fclose(file);

    ESP_LOGI(TAG, "Audio recording saved to %s", filename);
  }
}

TaskHandle_t init_microphone(void) {
  ESP_LOGI(TAG, "Initializing microphone");

  const i2s_chan_config_t chan_cfg =
      I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &channel));
  ESP_LOGI(TAG, "I2S channel created");

  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
      .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,
                                                  I2S_SLOT_MODE_MONO),
      .gpio_cfg = {.bclk = SCK,
                   .ws = WS,
                   .din = SD,
                   .mclk = I2S_GPIO_UNUSED,
                   .dout = I2S_GPIO_UNUSED,
                   .invert_flags =
                       {
                           .mclk_inv = false,
                           .bclk_inv = false,
                           .ws_inv = false,
                       }},
  };
  std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
  std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(channel, &std_cfg));
  ESP_LOGI(TAG, "I2S channel initialized in standard mode");

  ESP_ERROR_CHECK(i2s_channel_enable(channel));
  ESP_LOGI(TAG, "I2S channel enabled");

  TaskHandle_t task_handle = NULL;
  xTaskCreate(audio_record_task, "audio_record_task", 4096, NULL,
              tskIDLE_PRIORITY + 1, &task_handle);

  ESP_LOGI(TAG, "Microphone initialized");
  return task_handle;
}
