#pragma once
#include "esp_err.h"

/**
 * @brief  Đọc nhiệt độ từ MLX90614 (object temperature)
 * @return giá trị °C, hoặc -273.15 nếu lỗi
 */

float mlx90614_read_temp(void);
