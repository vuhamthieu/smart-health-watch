#pragma once
#include "esp_err.h"

/**
 * @brief  Khởi tạo I2C master (GPIO21=SDA, GPIO22=SCL)
 * @return ESP_OK nếu thành công
 */
esp_err_t i2c_master_init(void);
