#include "button.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#define PIN CONFIG_BUTTON_PIN
#define DEBOUNCE_TIME_US CONFIG_DEBOUNCE_TIME_MS * 1000

static const char *TAG = "tgvr_button";

static void (*on_press_callback)(void);
static void (*on_release_callback)(void);
static int64_t last_interrupt_time = 0;

void button_isr_handler(void *arg) {
  int64_t current_time = esp_timer_get_time();
  if (current_time - last_interrupt_time < DEBOUNCE_TIME_US) {
    return;
  }
  last_interrupt_time = current_time;

  int pin_level = gpio_get_level(PIN);
  if (pin_level == 0) {
    on_press_callback();
  } else {
    on_release_callback();
  }
}

void init_button(void (*on_press)(void), void (*on_release)(void)) {
  ESP_LOGI(TAG, "Initializing button on pin %d", PIN);

  on_press_callback = on_press;
  on_release_callback = on_release;

  gpio_config_t pin_cfg = {
      .pin_bit_mask = 1ULL << PIN,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_ANYEDGE,
  };
  ESP_ERROR_CHECK(gpio_config(&pin_cfg));
  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  ESP_ERROR_CHECK(gpio_isr_handler_add(PIN, button_isr_handler, NULL));

  ESP_LOGI(TAG, "Button initialized");
}
