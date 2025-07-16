#include "temp.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_NUM I2C_NUM_0
#define MLX90614_ADDR 0x57
#define MLX90614_REG_OBJECT_TEMP 0x07

static const char *TAG = "TEMP";

void temp_init(void)
{
    ESP_LOGI(TAG, "MLX90614 ready");
}

float temp_read_celsius(void)
{
    uint8_t reg = MLX90614_REG_OBJECT_TEMP;
    uint8_t data[2];
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, MLX90614_ADDR,
                                                &reg, 1, data, 2, pdMS_TO_TICKS(1000));
    if (ret == ESP_OK) {
        uint16_t raw = (data[1] << 8) | data[0];
        float temp = (raw * 0.02) - 273.15;
        ESP_LOGI(TAG, "Temp = %.2f°C", temp);
        return temp;
    } else {
        ESP_LOGE(TAG, "Failed to read temp: %s", esp_err_to_name(ret));
        return -1000;
    }
}