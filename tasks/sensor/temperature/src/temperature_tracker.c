#include "temperature_task.h"
#include "mlx90614.h"
#include "esp_log.h"

static const char *TAG = "TEMP";
static float       s_temp = -273.15f;

void temperature_init(void) {
    // any one‑time setup goes here, if needed
    s_temp = -273.15f;
}

void temperature_update(void) {
    s_temp = -273.15f;             // reset trước khi đọc
    float t = mlx90614_read_temp();
    if (t > -273.15f) {
        s_temp = t;
        ESP_LOGI(TAG, "Temperature: %.2f C", s_temp);
    } else {
        ESP_LOGW(TAG, "mlx90614_read_temp failed");
    }
}

float temperature_get_data(void) {
    return s_temp;
}
