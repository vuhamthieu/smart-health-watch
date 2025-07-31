#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"

esp_err_t wifi_init(const char* ssid, const char* password);
esp_err_t wifi_disconnect(void);
esp_err_t wifi_stop(void);
esp_err_t wifi_start(void);
bool is_wifi_connected(void);
bool is_wifi_connecting(void);

#endif