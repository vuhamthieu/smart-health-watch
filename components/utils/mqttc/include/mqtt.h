#pragma once
#include "esp_err.h"
#include <stdbool.h>

esp_err_t mqttc_init(const char* uri, const char* client_id);
esp_err_t mqttc_start(void);
esp_err_t mqttc_stop(void);
bool      mqttc_is_connected(void);
int       mqttc_publish(const char* topic, const char* payload, int qos, bool retain);
int       mqttc_subscribe(const char* topic, int qos);