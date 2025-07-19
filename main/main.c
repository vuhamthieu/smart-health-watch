#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_common.h"

void temperature_task(void *pv);
void display_task(void *pv);

void app_main(void)
{
    esp_err_t err = i2c_master_init();
    if (err != ESP_OK) {
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    xTaskCreate(temperature_task, "temp_task", 2048, NULL, 5, NULL);
    xTaskCreate(display_task,     "disp_task",  4096, NULL, 5, NULL);
}
