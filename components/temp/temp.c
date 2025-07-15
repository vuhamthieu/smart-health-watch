    #include "temp.h"
    #include "driver/i2c.h"
    #include "esp_log.h"
    #include "sdkconfig.h"

    #define I2C_MASTER_NUM I2C_NUM_0
    #define I2C_MASTER_SDA 21
    #define I2C_MASTER_SCL 22
    #define I2C_FREQ_HZ 100000

    #define MLX90614_ADDR 0x5A
    #define MLX90614_REG_OBJECT_TEMP 0x07

    static const char *TAG = "TEMP";

    void temp_init(void)
    {
        ESP_LOGI(TAG, "Initializing GY-906 (MLX90614)");

        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
        
        "${workspaceFolder}/build/config",        .sda_io_num = I2C_MASTER_SDA,
            .scl_io_num = I2C_MASTER_SCL,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = I2C_FREQ_HZ,
        };
        i2c_param_config(I2C_MASTER_NUM, &conf);
        i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0);
    }

    float temp_read_celsius(void)
    {
        uint8_t reg = MLX90614_REG_OBJECT_TEMP;
        uint8_t data[3];

        esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, MLX90614_ADDR,
                                                    &reg, 1, data, 3, pdMS_TO_TICKS(1000));

        if (ret == ESP_OK) {
            uint16_t raw = data[1] << 8 | data[0];
            float temp = (raw * 0.02) - 273.15;
            ESP_LOGI(TAG, "Temp = %.2f°C", temp);
            return temp;
        } else {
            ESP_LOGE(TAG, "Failed to read temp: %s", esp_err_to_name(ret));
            return -1000;
        }
    }
