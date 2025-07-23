#pragma once

#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t s_i2c_mutex;


// I2C Configuration
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA_IO   21
#define I2C_MASTER_SCL_IO   22
#define I2C_MASTER_FREQ_HZ  100000  
#define I2C_LOCK_TIMEOUT_MS 1000   

esp_err_t i2c_master_init(void);

esp_err_t i2c_common_write_read_device(i2c_port_t i2c_num, uint8_t device_addr, 
                                      const uint8_t* write_buffer, size_t write_size,
                                      uint8_t* read_buffer, size_t read_size, TickType_t ticks_to_wait);

esp_err_t i2c_common_write_to_device(i2c_port_t i2c_num, uint8_t device_addr, 
                                     const uint8_t* write_buffer, size_t write_size, TickType_t ticks_to_wait);