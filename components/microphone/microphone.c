#include "driver/i2s_std.h"
#include "esp_log.h"
#include "format_wav.h"

#define SCK CONFIG_SCK
#define WS CONFIG_WS
#define SD CONFIG_SD

#define SAMPLE_RATE CONFIG_SAMPLE_RATE
#define BYTES_PER_SEC (SAMPLE_RATE * 32 / 8)
#define BUFFER_SIZE (BYTES_PER_SEC * CONFIG_WRITE_FREQ_MS / 1000)

static const char *TAG = "tgvr_microphone";

static i2s_chan_handle_t channel = NULL;

void init_microphone(void) {
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

  ESP_LOGI(TAG, "Microphone initialized");
}

void record_audio(FILE *file, int duration_sec) {
  ESP_LOGI(TAG, "Recording audio for %d seconds", duration_sec);

  const int bytes_per_sample = 16 / 8;
  const int chunk_size = SAMPLE_RATE * bytes_per_sample * duration_sec;
  const wav_header_t wav_header =
      WAV_HEADER_PCM_DEFAULT(chunk_size, 16, SAMPLE_RATE, 1);

  fwrite(&wav_header, sizeof(wav_header), 1, file);

  // For some reason, samples need to be recorded as 32-bit and then converted
  // to 16-bit. No idea why, but this works, and I don't feel like debugging
  // audio signal and bit alignment issues.
  // https://esp32.com/viewtopic.php?t=15185
  size_t bytes_read;
  size_t bytes_written = 0;
  static int32_t buffer[BUFFER_SIZE];
  static int16_t converted_samples[BUFFER_SIZE / 2];
  while (bytes_written < chunk_size) {
    if (i2s_channel_read(channel, &buffer, BUFFER_SIZE, &bytes_read, 1000) ==
        ESP_OK) {
      int samples_read = bytes_read / sizeof(int32_t);
      for (int i = 0; i < samples_read; i++) {
        buffer[i] >>= 11;
        int16_t sample_16bit = (buffer[i] > INT16_MAX)   ? INT16_MAX
                               : (buffer[i] < INT16_MIN) ? INT16_MIN
                                                         : (int16_t)buffer[i];
        converted_samples[i] = sample_16bit;
      }
      int converted_bytes = samples_read * sizeof(int16_t);
      fwrite(converted_samples, converted_bytes, 1, file);
      bytes_written += converted_bytes;
    } else {
      ESP_LOGW(TAG, "I2S read timeout");
    }
  }

  ESP_LOGI(TAG, "Recording finished, wrote %zu bytes", bytes_written);
}
