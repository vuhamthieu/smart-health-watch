#include "display.h"
#include "u8g2.h"
#include "u8g2_esp32_hal.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA 21
#define I2C_MASTER_SCL 22
#define I2C_FREQ_HZ 100000
#define SSD1306_ADDRESS 0x3C

static const char *TAG = "DISPLAY";
static u8g2_t u8g2;

void display_init(void)
{
    ESP_LOGI(TAG, "Initializing OLED display");

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .scl_io_num = I2C_MASTER_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    esp_err_t err = i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0);
    if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "I2C driver already installed, skipping...");
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install error: %s", esp_err_to_name(err));
    }

    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
    u8g2_esp32_hal.sda = I2C_MASTER_SDA;
    u8g2_esp32_hal.scl = I2C_MASTER_SCL;
    u8g2_esp32_hal_init(&u8g2_esp32_hal);

    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2, U8G2_R0,
        u8g2_esp32_i2c_byte_cb,
        u8g2_esp32_gpio_and_delay_cb
    );

    u8g2_InitDisplay(&u8g2);
    vTaskDelay(pdMS_TO_TICKS(100));  
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(&u8g2, 0, 20, "OLED Ready!");
    u8g2_SendBuffer(&u8g2);
}

void display_show_temp(float temp) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Temp: %.2f C", temp);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(&u8g2, 0, 20, buf);
    u8g2_SendBuffer(&u8g2);
}