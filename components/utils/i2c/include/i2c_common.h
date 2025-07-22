#pragma once

#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/semphr.h"

// I2C Configuration
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA_IO   21
#define I2C_MASTER_SCL_IO   22
#define I2C_MASTER_FREQ_HZ  100000
#define I2C_LOCK_TIMEOUT_MS 1000  // ms

/**
 * @brief  Khởi tạo I2C master (và tạo mutex)
 * @return ESP_OK nếu thành công
 */
esp_err_t i2c_master_init(void);

/**
 * @brief  Viết rồi đọc từ 1 thiết bị I2C, có mutex bảo vệ
 * @param  device_addr: địa chỉ 7‑bit của thiết bị
 * @param  write_buffer: con trỏ đến dữ liệu cần ghi (có thể NULL nếu không ghi)
 * @param  write_size: số byte cần ghi
 * @param  read_buffer: con trỏ đến buffer nhận dữ liệu
 * @param  read_size: số byte cần đọc
 * @return ESP_OK nếu thành công
 */
esp_err_t i2c_common_write_read_device(uint8_t device_addr,
                                       const uint8_t* write_buffer, size_t write_size,
                                       uint8_t* read_buffer,    size_t read_size);

/**
 * @brief  Viết đến 1 thiết bị I2C, có mutex bảo vệ
 * @param  device_addr: địa chỉ 7‑bit của thiết bị
 * @param  write_buffer: con trỏ đến dữ liệu cần ghi
 * @param  write_size: số byte cần ghi
 * @return ESP_OK nếu thành công
 */
esp_err_t i2c_common_write_to_device(uint8_t device_addr,
                                     const uint8_t* write_buffer, size_t write_size);
