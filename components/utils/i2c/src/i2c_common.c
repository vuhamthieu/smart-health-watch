#include "i2c_common.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "i2c_common";
static SemaphoreHandle_t s_i2c_mutex = NULL;

esp_err_t i2c_master_init(void)
{
    static bool inited = false;
    if (inited) {
        return ESP_OK;
    }

    // 1) Tạo mutex
    s_i2c_mutex = xSemaphoreCreateMutex();
    if (!s_i2c_mutex) {
        ESP_LOGE(TAG, "Failed to create I2C mutex");
        return ESP_ERR_NO_MEM;
    }

    // 2) Cấu hình bus
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %d", err);
        return err;
    }
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %d", err);
        return err;
    }

    inited = true;
    ESP_LOGI(TAG, "I2C initialized @ SDA=%d, SCL=%d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    return ESP_OK;
}

esp_err_t i2c_common_write_read_device(uint8_t device_addr,
                                       const uint8_t* write_buffer, size_t write_size,
                                       uint8_t* read_buffer,    size_t read_size)
{
    if (!s_i2c_mutex) return ESP_ERR_INVALID_STATE;
    if (xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(I2C_LOCK_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    // START + WRITE ADDR
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    if (write_size > 0) {
        i2c_master_write(cmd, write_buffer, write_size, true);
    }
    // REPEATED START + READ ADDR
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_READ, true);
    if (read_size > 1) {
        i2c_master_read(cmd, read_buffer, read_size - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, read_buffer + read_size - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_LOCK_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(s_i2c_mutex);
    return ret;
}

esp_err_t i2c_common_write_to_device(uint8_t device_addr,
                                     const uint8_t* write_buffer, size_t write_size)
{
    if (!s_i2c_mutex) return ESP_ERR_INVALID_STATE;
    if (xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(I2C_LOCK_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, write_buffer, write_size, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_LOCK_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(s_i2c_mutex);
    return ret;
}
