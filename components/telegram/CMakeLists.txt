idf_component_register(SRCS "telegram.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES telegram_api.pem
                    PRIV_REQUIRES esp_http_client)
