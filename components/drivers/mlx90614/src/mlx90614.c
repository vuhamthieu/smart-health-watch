#include "mlx90614.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define MLX90614_ADDR     0x5A
#define MLX90614_CMD_TEMP 0x07   // object temp register

static const char *TAG = "mlx90614";

/**
 * @brief  Đọc raw 16-bit, tính thành °C
 */
float mlx90614_read_temp(void)
{
    uint8_t buf[3];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Write pointer
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MLX90614_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, MLX90614_CMD_TEMP, true);
    // Read 3 bytes
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MLX90614_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buf, 2, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, buf + 2, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %d", ret);
        return -273.15f;
    }

    uint16_t raw = buf[0] | (buf[1] << 8);
    float temp = raw * 0.02f - 273.15f;
    ESP_LOGD(TAG, "Raw=0x%04X => %.2f C", raw, temp);
    return temp;
}
