idf_component_register(
    SRCS "src/wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_wifi esp_event esp_netif nvs_flash
)
