idf_component_register(
    SRCS "main.c" "ui_manager.c"
    INCLUDE_DIRS "include" "." "icons"
    REQUIRES
        max30100
        mlx90614
        button
        i2c
        u8g2
        gps
        health
        temperature
        lvgl
        lvgl_esp32_drivers
        lvgl__lvgl
        wifi
        http
        mqttc
        bluetooth
    PRIV_REQUIRES freertos esp_common driver esp_lcd
    EMBED_FILES "partitions.csv"    
)

