#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "cJSON.h"
#include <math.h>
#include <string.h>

#include "mqtt.h"
#include "wifi.h"
#include "http_client.h"

extern QueueHandle_t http_queue;

static const char *TAG = "MQTT_TASK";

#ifndef MQTT_BROKER_URI
#define MQTT_BROKER_URI "mqtt://192.168.1.60:1883"
#endif

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "health-device-01"
#endif

#ifndef MQTT_TOPIC_BASE
#define MQTT_TOPIC_BASE "health_monitor/device01"
#endif

static void build_and_publish_temperature(float t)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temperature", roundf(t * 100) / 100);
    char *json = cJSON_PrintUnformatted(root);

    char topic[96];
    snprintf(topic, sizeof(topic), "%s/temperature", MQTT_TOPIC_BASE);
    mqttc_publish(topic, json, 1, false);

    cJSON_Delete(root);
    free(json);
}

static void build_and_publish_health(int hr, int spo2)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "heart_rate", hr);
    cJSON_AddNumberToObject(root, "spo2", spo2);
    char *json = cJSON_PrintUnformatted(root);

    char topic[96];
    snprintf(topic, sizeof(topic), "%s/health", MQTT_TOPIC_BASE);
    mqttc_publish(topic, json, 1, false);

    cJSON_Delete(root);
    free(json);
}

static void build_and_publish_gps(float lat, float lon)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "latitude", lat);
    cJSON_AddNumberToObject(root, "longitude", lon);
    char *json = cJSON_PrintUnformatted(root);

    char topic[96];
    snprintf(topic, sizeof(topic), "%s/gps", MQTT_TOPIC_BASE);
    mqttc_publish(topic, json, 0, false);

    cJSON_Delete(root);
    free(json);
}

void mqtt_client_task(void *pv)
{
    ESP_LOGI(TAG, "MQTT client task starting");

    // Initialize MQTT client once
    if (mqttc_init(MQTT_BROKER_URI, MQTT_CLIENT_ID) != ESP_OK)
    {
        ESP_LOGE(TAG, "mqttc_init failed");
    }

    http_message_t msg;

    for (;;)
    {
        if (!is_wifi_connected())
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        static bool client_started = false;
        if (!client_started)
        {
            esp_err_t s = mqttc_start();
            if (s != ESP_OK)
            {
                ESP_LOGW(TAG, "mqttc_start retry later: %d", (int)s);
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
            client_started = true;
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        if (xQueueReceive(http_queue, &msg, pdMS_TO_TICKS(200)) == pdTRUE)
        {
            switch (msg.data_type)
            {
            case 0:
                ESP_LOGI(TAG, "Publish temperature: %.2f", msg.data.temperature);
                build_and_publish_temperature(msg.data.temperature);
                break;
            case 1:
                ESP_LOGI(TAG, "Publish health: HR=%d SpO2=%d", msg.data.health.heart_rate, msg.data.health.spo2);
                build_and_publish_health(msg.data.health.heart_rate, msg.data.health.spo2);
                break;
            case 2:
                ESP_LOGI(TAG, "Publish GPS: %.6f, %.6f", msg.data.gps.lat, msg.data.gps.lon);
                build_and_publish_gps(msg.data.gps.lat, msg.data.gps.lon);
                break;
            default:
                ESP_LOGW(TAG, "Unknown message type: %d", msg.data_type);
                break;
            }

            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}
