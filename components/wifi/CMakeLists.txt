idf_component_register(SRCS "wifi.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_event esp_wifi nvs_flash)
