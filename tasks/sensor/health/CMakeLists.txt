idf_component_register(
    SRCS "src/health_tracker.c"
    INCLUDE_DIRS "include"
    REQUIRES max30100 i2c driver
)