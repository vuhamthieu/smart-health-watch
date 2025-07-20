#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_common.h"
#include "button.h"
#include "ui_manager.h"
#include "esp_log.h"
#include "u8g2_esp32_hal.h"
#include "u8g2.h"
#include "gps_tracker.h"
#include "health_tracker.h"
#include "temperature_task.h"
#include "display_task.h"

static ui_manager_t ui;
static u8g2_t u8g2;

static void handle_button(button_id_t btn) {
    ui_manager_handle_button(&ui, btn);
}

extern ui_manager_t ui;

void sensor_manager_task(void *pv) {
    while (1) {
        switch (ui.current_state) {
            case UI_STATE_GPS:
                gps_update(); 
                break;
            case UI_STATE_TEMP:
                temperature_update();
                break;
            case UI_STATE_HR:
                health_update();
                break;
            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  
    }
}

void app_main(void) {
    esp_err_t err = i2c_master_init();
    if (err != ESP_OK) {
        ESP_LOGE("MAIN", "I2C init failed: %s", esp_err_to_name(err));
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    gps_init();
    temperature_init();   
    health_init();

    u8g2_esp32_hal_t hal = { .sda = 21, .scl = 22 };
    u8g2_esp32_hal_init(&hal);
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2, U8G2_R0,
        u8g2_esp32_i2c_byte_cb,
        u8g2_esp32_gpio_and_delay_cb
    );
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    ui_manager_init(&ui, &u8g2);
    button_init(handle_button);
    xTaskCreate(sensor_manager_task, "sensor", 4096, NULL, 5, NULL);
    xTaskCreate(display_task, "display", 4096, &ui, 5, NULL);
}
    