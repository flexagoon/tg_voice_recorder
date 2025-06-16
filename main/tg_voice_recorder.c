#include "sd.h"
#include "wifi.h"

void app_main(void) {
  connect_wifi();
  mount_sd("/data");
}
