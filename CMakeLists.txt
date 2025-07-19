# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS
    components/drivers/mlx90614
    components/drivers/neo6m
    components/utils/button
    components/utils/i2c
    components/libs/u8g2
    components/libs/max30100
    tasks/display
    tasks/gps
    tasks/sensor/health
    tasks/sensor/temperature
)

project(health_monitor)
