#include "i2c_common.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA_IO   21
#define I2C_MASTER_SCL_IO   22
#define I2C_MASTER_FREQ_HZ  100000

static const char *TAG = "i2c_common";
esp_err_t i2c_master_init(void)
{
    static bool s_i2c_initialized = false;

    if (s_i2c_initialized) {
        return ESP_OK;
    }

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
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

    s_i2c_initialized = true;

    ESP_LOGI(TAG, "I2C initialized @ GPIO%d(SDA), GPIO%d(SCL)",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    return ESP_OK;
}
