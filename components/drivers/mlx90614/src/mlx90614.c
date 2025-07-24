#include "mlx90614.h"
#include "i2c_common.h"
#include "esp_log.h"

#define MLX90614_ADDR     0x5A
#define MLX90614_BODY_TEMP 0x07
#define MLX90614_ERROR_FLAG 0x8000  

static const char *TAG = "MLX90614";

static uint8_t mlx90614_calc_pec(uint8_t pec, uint8_t data)
{
    pec ^= data;
    for (uint8_t i = 8; i > 0; --i) {
        if (pec & 0x80) {
            pec = (pec << 1) ^ 0x07;
        } else {
            pec <<= 1;
        }
    }
    return pec;
}

float mlx90614_read_temp(void)
{
    uint8_t cmd = MLX90614_BODY_TEMP;
    uint8_t buf[3] = {0};
    
    esp_err_t ret = i2c_common_write_read_device(I2C_MASTER_NUM, MLX90614_ADDR, &cmd, 1, buf, 3, pdMS_TO_TICKS(2000));
    
    //ESP_LOGI(TAG, "Read ret=%d, raw data=%02X %02X %02X", ret, buf[0], buf[1], buf[2]);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(ret));
        return -273.15f;
    }

    uint8_t pec = 0;
    pec = mlx90614_calc_pec(pec, (MLX90614_ADDR << 1));
    pec = mlx90614_calc_pec(pec, cmd);
    pec = mlx90614_calc_pec(pec, (MLX90614_ADDR << 1) | 1);
    pec = mlx90614_calc_pec(pec, buf[0]);
    pec = mlx90614_calc_pec(pec, buf[1]);
    
    //ESP_LOGI(TAG, "PEC calculated: 0x%02X, received: 0x%02X", pec, buf[2]);
    
    if (pec != buf[2]) {
        ESP_LOGE(TAG, "PEC check failed! Data corrupted.");
        return -273.15f;
    }

    uint16_t raw = buf[0] | (buf[1] << 8);
    
    if (raw & MLX90614_ERROR_FLAG) {
        ESP_LOGW(TAG, "Error flag set in temperature data (raw: 0x%04X)", raw);
        return -273.15f;
    }
    
    raw &= 0x7FFF; 
    
    if (raw == 0x0000) {
        //ESP_LOGW(TAG, "Invalid raw data after masking: 0x%04X", raw);
        return -273.15f;
    }

    float temp = raw * 0.02f - 273.15f;
    
    if (temp < -70.0f || temp > 382.0f) {
        //ESP_LOGW(TAG, "Temperature out of valid range: %.2f C (raw: 0x%04X)", temp, raw);
        return -273.15f;
    }
    
    //ESP_LOGI(TAG, "Valid temperature: %.2f C (raw: 0x%04X)", temp, raw);
    return temp;
}
