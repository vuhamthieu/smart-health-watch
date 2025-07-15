// components/display/display.c
#include "display.h"
#include "esp_log.h"

static const char *TAG = "DISPLAY";

void display_init(void)
{
    ESP_LOGI(TAG, "Stub display_init called");
    // TODO: Khởi tạo OLED (I2C, SSD1306 driver, v.v.)
}

void display_show_temp(float temp)
{
    ESP_LOGI(TAG, "Stub display_show_temp: %.2f°C", temp);
    // TODO: Vẽ nhiệt độ temp lên OLED
}
