// i2c_common.c - Version fixed
#include "i2c_common.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "i2c_common";
 SemaphoreHandle_t s_i2c_mutex = NULL;
static bool s_i2c_initialized = false;

esp_err_t i2c_master_init(void)
{
    if (s_i2c_initialized) {
        ESP_LOGI(TAG, "I2C already initialized, skipping");
        return ESP_OK;
    }

    s_i2c_mutex = xSemaphoreCreateBinary();
    if (!s_i2c_mutex) {
        ESP_LOGE(TAG, "Failed to create I2C semaphore");
        return ESP_ERR_NO_MEM;
    }
    xSemaphoreGive(s_i2c_mutex);

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,  
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,  
        .clk_flags = 0
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    s_i2c_initialized = true;
    ESP_LOGI(TAG, "I2C initialized successfully");
    return ESP_OK;

cleanup:
    if (s_i2c_mutex) {
        vSemaphoreDelete(s_i2c_mutex);
        s_i2c_mutex = NULL;
    }
    return err;
}

esp_err_t i2c_common_write_to_device(i2c_port_t i2c_num, uint8_t device_addr, 
                                     const uint8_t* write_buffer, size_t write_size, 
                                     TickType_t ticks_to_wait)
{
    if (!s_i2c_mutex || !s_i2c_initialized) {
        ESP_LOGE(TAG, "I2C not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(2000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take I2C semaphore");
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        xSemaphoreGive(s_i2c_mutex);
        return ESP_ERR_NO_MEM;
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    if (write_size > 0) {
        i2c_master_write(cmd, write_buffer, write_size, true);
    }
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(i2c_num, cmd, ticks_to_wait);
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(s_i2c_mutex);
    
    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "I2C write failed: %s (addr=0x%02X)", 
                 esp_err_to_name(ret), device_addr);
    }
    
    return ret;
}

esp_err_t i2c_common_write_read_device(i2c_port_t i2c_num, uint8_t device_addr, 
                                      const uint8_t* write_buffer, size_t write_size,
                                      uint8_t* read_buffer, size_t read_size, 
                                      TickType_t ticks_to_wait)
{
    if (!s_i2c_mutex || !s_i2c_initialized) {
        ESP_LOGE(TAG, "I2C not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(2000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take I2C semaphore");
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        xSemaphoreGive(s_i2c_mutex);
        return ESP_ERR_NO_MEM;
    }

    // Write với repeated start (critical cho MLX90614)
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    if (write_size > 0) {
        i2c_master_write(cmd, write_buffer, write_size, true);
    }
    
    // REPEATED START
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_READ, true);
    
    if (read_size > 0) {
        if (read_size > 1) {
            i2c_master_read(cmd, read_buffer, read_size - 1, I2C_MASTER_ACK);
        }
        i2c_master_read_byte(cmd, read_buffer + read_size - 1, I2C_MASTER_NACK);
    }
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(i2c_num, cmd, ticks_to_wait);
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(s_i2c_mutex);
    
    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "I2C write-read failed: %s (addr=0x%02X)", 
                 esp_err_to_name(ret), device_addr);
    }
    
    return ret;
}
