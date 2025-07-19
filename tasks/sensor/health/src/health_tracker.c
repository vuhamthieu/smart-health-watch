#include "health_tracker.h"
#include "max30100.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#define TAG "HEALTH_TRACKER"

// Khai báo biến cấu hình và dữ liệu toàn cục
static max30100_config_t max30100_config;
static health_data_t current_health_data = {0, 0, false};

void health_get_data(health_data_t *data) {
    *data = current_health_data;
}

void health_task(void *pv) {
    // 1. Khởi tạo cấu hình MAX30100
    esp_err_t err = max30100_init(
        &max30100_config,          // Cấu trúc config
        I2C_NUM_0,                // Port I2C (thay đổi nếu cần)
        MAX30100_MODE_SPO2_HR,     // Chế độ đo cả SpO2 và nhịp tim
        MAX30100_SAMPLING_RATE_100HZ, // Tần số lấy mẫu 100Hz
        MAX30100_PULSE_WIDTH_1600US_ADC_16, // Độ rộng xung
        MAX30100_LED_CURRENT_50MA, // Dòng LED IR
        MAX30100_LED_CURRENT_27_1MA, // Dòng LED Red ban đầu
        15,                       // Kích thước bộ lọc trung bình
        10,                       // Kích thước mẫu BPM
        true,                     // Chế độ độ phân giải cao
        false                     // Không debug
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init MAX30100: %s", esp_err_to_name(err));
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    // 2. Cấu trúc lưu dữ liệu đọc từ cảm biến
    max30100_data_t sensor_data;

    while (1) {
        // 3. Đọc dữ liệu từ cảm biến
        if (max30100_update(&max30100_config, &sensor_data) == ESP_OK) {
            // 4. Cập nhật dữ liệu sức khỏe
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
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Cập nhật mỗi 1s
    }
}