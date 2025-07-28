idf_component_register(
    SRCS "main.c" "ui_manager.c"
    INCLUDE_DIRS "include" "."
    REQUIRES
        max30100
        mlx90614
        neo6m
        button
        i2c
        u8g2
        gps
        health
        temperature
        lvgl
        lvgl_esp32_drivers
        lvgl__lvgl
    PRIV_REQUIRES freertos esp_common driver esp_lcd
)

