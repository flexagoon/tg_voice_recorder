#include "driver/i2s_std.h"
#include "esp_log.h"

#define SCK CONFIG_SCK
#define WS CONFIG_WS
#define SD CONFIG_SD

static const char *TAG = "tgvr_microphone";

static i2s_chan_handle_t channel = NULL;

void init_microphone(void) {
  ESP_LOGI(TAG, "Initializing microphone");

  const i2s_chan_config_t chan_cfg =
      I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &channel));
  ESP_LOGI(TAG, "I2S channel created");

  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
      .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_24BIT,
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
  std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(channel, &std_cfg));
  ESP_LOGI(TAG, "I2S channel initialized in standard mode");

  ESP_ERROR_CHECK(i2s_channel_enable(channel));
  ESP_LOGI(TAG, "I2S channel enabled");

  ESP_LOGI(TAG, "Microphone initialized");
}

void record_audio(FILE *file, int duration_sec) {}
