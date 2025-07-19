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
    *out = s_current;
}

void gps_task(void *pvParameters) {
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
        int len = uart_read_bytes(GPS_UART_PORT, buf, sizeof(buf)-1, pdMS_TO_TICKS(2000));
        if (len > 0) {
            buf[len] = '\0';
            if (strncmp((char*)buf, "$GPRMC", 6) == 0) {
                float lat = 0.0f;
                float lon = 0.0f;
                s_current.latitude  = lat;
                s_current.longitude = lon;
                s_current.valid     = true;
                ESP_LOGI(TAG, "Got fix: %f, %f", lat, lon);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
