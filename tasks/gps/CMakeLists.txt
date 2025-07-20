idf_component_register(
    SRCS "src/gps_tracker.c"
    INCLUDE_DIRS "include"
    REQUIRES driver freertos     # hoặc driver i2c, tùy GPS kết nối qua UART hay I2C
)
