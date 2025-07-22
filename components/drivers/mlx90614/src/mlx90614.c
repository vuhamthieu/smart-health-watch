#include "mlx90614.h"
#include "i2c_common.h"
#include "esp_log.h"

#define MLX90614_ADDR     0x5A
#define MLX90614_CMD_TEMP 0x07 

static const char *TAG = "MLX90614";

float mlx90614_read_temp(void)
{
    uint8_t cmd = MLX90614_CMD_TEMP;
    uint8_t buf[3];
    
    esp_err_t ret = i2c_common_write_read_device(MLX90614_ADDR, &cmd, 1, buf, 3);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(ret));
        return -273.15f;
    }

    uint16_t raw = buf[0] | (buf[1] << 8);
    float temp = raw * 0.02f - 273.15f;
    ESP_LOGD(TAG, "Raw=0x%04X => %.2f C", raw, temp);
    return temp;
}