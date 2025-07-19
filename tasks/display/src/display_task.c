#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "u8g2.h"
#include "u8g2_esp32_hal.h"
#include <stdio.h>

// extern biến từ temperature task
extern float current_temp;

static u8g2_t u8g2;

void display_task(void *pvParameters)
{

    // 1) Init HAL
    u8g2_esp32_hal_t hal = {
        .sda = 21,
        .scl = 22,
    };
    u8g2_esp32_hal_init(&hal);

    // 2) Init u8g2
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2, U8G2_R0,
        u8g2_esp32_i2c_byte_cb,
        u8g2_esp32_gpio_and_delay_cb);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    char buf[32];
    while (1)
    {
        u8g2_ClearBuffer(&u8g2);
        if (current_temp == -273.15f)
        {
            u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
            u8g2_DrawStr(&u8g2, 0, 15, "BODY TEMPERATURE");
            u8g2_DrawStr(&u8g2, 0, 35, "Scanning ...");
        }
        else
        {
            u8g2_DrawStr(&u8g2, 0, 15, "BODY TEMPERATURE");
            snprintf(buf, sizeof(buf), "Temp: %.2f C", current_temp);
            u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
            u8g2_DrawStr(&u8g2, 0, 35, buf);
        }
        
        u8g2_SendBuffer(&u8g2);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}
