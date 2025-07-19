#include "gps_tracker.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "driver/gpio.h"

static const char *TAG = "GPS";
static gps_data_t s_current = {0, 0, false};
static const uart_port_t GPS_UART_PORT = UART_NUM_1;

void gps_get_data(gps_data_t *out) {
    // Trả về bản sao dữ liệu mới nhất
    *out = s_current;
}

void gps_task(void *pvParameters) {
    // 1) Cấu hình UART cho Neo6M (TX/RX pins tuỳ board)
    uart_config_t uart_cfg = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(GPS_UART_PORT, &uart_cfg);
    uart_set_pin(GPS_UART_PORT, /*TX=*/GPIO_NUM_19, /*RX=*/GPIO_NUM_18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(GPS_UART_PORT, 1024, 0, 0, NULL, 0);

    uint8_t buf[128];
    while (1) {
        // 2) Đọc 1 dòng NMEA
        int len = uart_read_bytes(GPS_UART_PORT, buf, sizeof(buf)-1, pdMS_TO_TICKS(2000));
        if (len > 0) {
            buf[len] = '\0';
            // 3) Tìm chuỗi bắt đầu bằng "$GPRMC" hoặc "$GPGGA"
            if (strncmp((char*)buf, "$GPRMC", 6) == 0) {
                // TODO: parse GPS RMC sentence: tách các trường, convert lat/lon sang float
                // Ví dụ đơn giản giả định dữ liệu ok:
                float lat = /* parse từ buf */ 0.0f;
                float lon = /* parse từ buf */ 0.0f;
                // Cập nhật biến toàn cục
                s_current.latitude  = lat;
                s_current.longitude = lon;
                s_current.valid     = true;
                ESP_LOGI(TAG, "Got fix: %f, %f", lat, lon);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
