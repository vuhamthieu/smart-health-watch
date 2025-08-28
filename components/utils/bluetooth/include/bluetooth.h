#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// BLE Health Monitor Service UUID (custom)
#define HEALTH_SERVICE_UUID        0x180D  // Heart Rate Service (standard)
#define HEALTH_CHAR_HR_UUID        0x2A37  // Heart Rate Measurement
#define HEALTH_CHAR_TEMP_UUID      0x2A6E  // Temperature
#define HEALTH_CHAR_SPO2_UUID      0x2A5F  // Pulse Oximetry SpO2
#define HEALTH_CHAR_GPS_UUID       0x2A67  // Location and Navigation
#define HEALTH_CHAR_CMD_UUID       0x2A56  // Digital (for commands)
#define HEALTH_CHAR_NOTIFY_UUID    0x2A18  // Glucose Measurement (for notifications)

esp_err_t bluetooth_init(void);

esp_err_t bluetooth_start_advertising(void);

esp_err_t bluetooth_stop_advertising(void);

esp_err_t bluetooth_notify_heart_rate(uint16_t hr, uint8_t spo2);
esp_err_t bluetooth_notify_temperature(float temp);
esp_err_t bluetooth_notify_gps(float lat, float lon);

esp_err_t bluetooth_send_notification(const char* title, const char* message);

bool bluetooth_is_connected(void);
bool bluetooth_is_advertising(void);
esp_err_t bluetooth_disconnect(void);

#ifdef __cplusplus
}
#endif