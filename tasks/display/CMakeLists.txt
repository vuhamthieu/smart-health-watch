idf_component_register(
    SRCS "src/display_task.c"
    INCLUDE_DIRS "include" "../../main/include" "../../tasks/gps/include"
    REQUIRES u8g2 i2c button
)