#include "health_tracker.h"
#include "max30100.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#define TAG "HEALTH_TRACKER"

static max30100_config_t max30100_config;
static health_data_t current_health_data = {0, 0, false};

void health_get_data(health_data_t *data) {
    *data = current_health_data;
}

void health_task(void *pv) {
    esp_err_t err = max30100_init(
        &max30100_config,          
        I2C_NUM_0,                
        MAX30100_MODE_SPO2_HR,     
        MAX30100_SAMPLING_RATE_100HZ, 
        MAX30100_PULSE_WIDTH_1600US_ADC_16, 
        MAX30100_LED_CURRENT_50MA, 
        MAX30100_LED_CURRENT_27_1MA, 
        15,                       
        10,                       
        true,                     
        false                     
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init MAX30100: %s", esp_err_to_name(err));
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    max30100_data_t sensor_data;

    while (1) {
        if (max30100_update(&max30100_config, &sensor_data) == ESP_OK) {
            current_health_data.heart_rate = (int)sensor_data.heart_bpm;
            current_health_data.spo2 = (int)sensor_data.spO2;
            current_health_data.valid = true;
            
            ESP_LOGI(TAG, "HR: %d bpm, SpO2: %d%%", 
                    current_health_data.heart_rate, 
                    current_health_data.spo2);
        } else {
            current_health_data.valid = false;
            ESP_LOGW(TAG, "Failed to read MAX30100 data");
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}