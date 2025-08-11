#include "temperature_task.h"
#include "mlx90614.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// static const char *TAG = "TEMP";
static float s_temp = -273.15f;
static float s_temp_backup = -273.15f;
static bool s_valid_temp = false;

void temperature_init(void)
{
    s_temp = -273.15f;
}

void temperature_update(void)
{
    int max_retry = 5;
    float last_valid = -273.15f;

    for (int i = 0; i < max_retry; ++i)
    {
        float t = mlx90614_read_temp();

        if (t > -273.15f)
        {
            last_valid = t;
            // ESP_LOGI(TAG, "Temperature: %.2f C", t);
            printf("%.2f\n", t);
            break;
        }
        else
        {
            // ESP_LOGW(TAG, "mlx90614_read_temp failed (retry %d)", i + 1);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (last_valid > -273.15f)
    {
        s_temp = last_valid;
        s_temp_backup = last_valid;
        s_valid_temp = true;
    }
    else
    {
        // ESP_LOGE(TAG, "Failed to get valid temperature in this round.");
        s_temp = s_temp_backup;
    }
}

float temperature_get_data(void)
{
    return s_temp;
}
