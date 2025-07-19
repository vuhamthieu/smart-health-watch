#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mlx90614.h"

static const char *TAG = "TEMP_TASK";
float current_temp = 0.0f;

void temperature_task(void *pvParameters)
{
    while (1) {
        current_temp = mlx90614_read_temp();
        ESP_LOGI(TAG, "Temperature: %.2f C", current_temp);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
