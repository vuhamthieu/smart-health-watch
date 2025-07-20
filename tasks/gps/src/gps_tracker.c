// In tasks/gps/src/gps_tracker.c:

#include "gps_tracker.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "driver/gpio.h"

static const char *TAG = "GPS";
static gps_data_t  s_current     = { 0.0f, 0.0f, false };
static const uart_port_t GPS_UART_PORT = UART_NUM_1;

static bool s_uart_installed = false;

void gps_init(void) {
    if (!s_uart_installed) {
        uart_config_t uart_cfg = {
            .baud_rate  = 9600,
            .data_bits  = UART_DATA_8_BITS,
            .parity     = UART_PARITY_DISABLE,
            .stop_bits  = UART_STOP_BITS_1,
            .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE
        };
        ESP_ERROR_CHECK(uart_param_config(GPS_UART_PORT, &uart_cfg));
        ESP_ERROR_CHECK(uart_set_pin(
            GPS_UART_PORT,
            GPIO_NUM_19,  // TX
            GPIO_NUM_18,  // RX
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE
        ));
        ESP_ERROR_CHECK(uart_driver_install(GPS_UART_PORT, 1024, 0, 0, NULL, 0));
        ESP_LOGI(TAG, "GPS UART driver installed");
        s_uart_installed = true; 
    }
}
void gps_update(void) {
    if (!s_uart_installed) {
        ESP_LOGE(TAG, "GPS UART not initialized");
        return;
    }
// <<< mark it done
    uint8_t buf[128];
    int len = uart_read_bytes(GPS_UART_PORT, buf, sizeof(buf) - 1, pdMS_TO_TICKS(1000));
    if (len > 0) {
        buf[len] = '\0'; 
        ESP_LOGD(TAG, "Received: %s", buf);

        char *token = strtok((char *)buf, ",");
        if (token && strcmp(token, "$GPGGA") == 0) {
            token = strtok(NULL, ",");  // Time
            token = strtok(NULL, ",");  // Latitude
            if (token) {
                s_current.latitude = atof(token);
                token = strtok(NULL, ",");  // N/S indicator
                if (token && strcmp(token, "S") == 0) {
                    s_current.latitude = -s_current.latitude;  // Convert to negative if South
                }
            }
            token = strtok(NULL, ",");  // Longitude
            if (token) {
                s_current.longitude = atof(token);
                token = strtok(NULL, ",");  // E/W indicator
                if (token && strcmp(token, "W") == 0) {
                    s_current.longitude = -s_current.longitude;  // Convert to negative if West
                }
            }
            s_current.valid = true;
            ESP_LOGI(TAG, "GPS fix: %.6f, %.6f", s_current.latitude, s_current.longitude);
        } else {
            s_current.valid = false;
            ESP_LOGW(TAG, "Invalid GPS data");
        }
    } else {
        ESP_LOGW(TAG, "No data received from GPS");
    }
}

void gps_get_data(gps_data_t *out) {
    if (out) {
        *out = s_current; 
    } else {
        ESP_LOGE(TAG, "Output pointer is NULL");
    }
}