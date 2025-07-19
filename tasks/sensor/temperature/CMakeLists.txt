idf_component_register(
    SRCS "src/temperature_tracker.c"   # hoặc temperature_task.c
    INCLUDE_DIRS "include"
    REQUIRES mlx90614 i2c
)
