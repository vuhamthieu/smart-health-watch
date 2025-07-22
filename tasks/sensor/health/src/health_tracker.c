#include "health_tracker.h"
#include "max30100.h"
#include "i2c_common.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "HEALTH_TRACKER";

static max30100_config_t s_config;
static health_data_t s_data = {0, 0, false};

void health_init(void) {
    esp_err_t err = max30100_init(
        &s_config, I2C_NUM_0, MAX30100_MODE_SPO2_HR, MAX30100_SAMPLING_RATE_100HZ,
        MAX30100_PULSE_WIDTH_1600US_ADC_16, MAX30100_LED_CURRENT_50MA,
        MAX30100_LED_CURRENT_27_1MA, 15, 10, true, false
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "max30100_init failed: %s", esp_err_to_name(err));
    }
}

void health_update(void) {
    max30100_data_t d;
    esp_err_t err = max30100_update(&s_config, &d);
    if (err == ESP_OK) {
        s_data.heart_rate = (int)d.heart_bpm;
        s_data.spo2 = (int)d.spO2;
        s_data.valid = true;
        ESP_LOGI(TAG, "Heart Rate: %d bpm, SpO2: %d%%", s_data.heart_rate, s_data.spo2);
    } else {
        s_data.valid = false;
        ESP_LOGW(TAG, "max30100_update failed: %s", esp_err_to_name(err));
    }
}

void health_get_data(health_data_t *out) {
    *out = s_data;
}