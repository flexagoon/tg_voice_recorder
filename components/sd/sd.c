#include "sd.h"

#include "esp_vfs_fat.h"

#define SCLK CONFIG_SCLK
#define MOSI CONFIG_MOSI
#define MISO CONFIG_MISO
#define CS CONFIG_CS
#define MNT_PATH CONFIG_MNT_PATH

static const char *TAG = "tgvr_sd";

void mount_sd(void) {
  ESP_LOGI(TAG, "Mounting SD card at %s", MNT_PATH);

  const spi_bus_config_t spi_bus_cfg = {
      .sclk_io_num = SCLK,
      .miso_io_num = MISO,
      .mosi_io_num = MOSI,
  };
  ESP_ERROR_CHECK(
      spi_bus_initialize(SDSPI_DEFAULT_HOST, &spi_bus_cfg, SDSPI_DEFAULT_DMA));

  const sdmmc_host_t host_cfg = SDSPI_HOST_DEFAULT();

  sdspi_device_config_t device_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
  device_cfg.gpio_cs = CS;

  const esp_vfs_fat_mount_config_t mount_cfg = VFS_FAT_MOUNT_DEFAULT_CONFIG();
  ESP_ERROR_CHECK(esp_vfs_fat_sdspi_mount(MNT_PATH, &host_cfg, &device_cfg,
                                          &mount_cfg, NULL));

  ESP_LOGI(TAG, "SD mounted");
}
