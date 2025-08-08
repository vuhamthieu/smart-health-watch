#include "http_client.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "health_tracker.h"
#include "temperature_task.h"
#include "gps_tracker.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi.h"
#include <math.h>
#include "freertos/semphr.h"
#include "freertos/queue.h"

extern SemaphoreHandle_t http_semaphore;
extern QueueHandle_t http_queue;

static const char *TAG = "HTTP_CLIENT";

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s",
                 evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP data received: %.*s", evt->data_len, (char *)evt->data);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP client disconnected");
        break;
    default:
        break;
    }
    return ESP_OK;
}

void http_client_task(void *pv)
{
    http_message_t message;

    ESP_LOGI(TAG, "HTTP client task started");

    for (;;)
    {
        // Wait for HTTP requests in queue
        if (xQueueReceive(http_queue, &message, portMAX_DELAY) == pdTRUE)
        {

            // Take HTTP semaphore to prevent conflicts
            if (xSemaphoreTake(http_semaphore, pdMS_TO_TICKS(2000)) == pdTRUE)
            {

                if (!is_wifi_connected())
                {
                    ESP_LOGW(TAG, "WiFi not connected, skipping request");
                    xSemaphoreGive(http_semaphore);
                    continue;
                }

                switch (message.data_type)
                {
                case 0: 
                    http_client_send_temp(message.data.temperature);
                    break;
                case 1:
                    http_client_send_hr_spo2(
                        message.data.health.heart_rate,
                        message.data.health.spo2);
                    break;
                case 2: 
                    http_client_send_gps(
                        message.data.gps.lat,
                        message.data.gps.lon);
                    break;
                }

                xSemaphoreGive(http_semaphore);

                // Add delay between HTTP requests
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            else
            {
                ESP_LOGW(TAG, "HTTP semaphore timeout");
            }
        }
    }
}

void send_sensor_data_task(void *pvParameters)
{
    esp_http_client_config_t config = {
        .url = "http://192.168.43.76:5000/update",
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_POST,
    };

    while (1)
    {
        if (is_wifi_connected())
        {
            health_data_t health_data = {0};
            health_get_data(&health_data);
            float temperature = temperature_get_data();
            gps_data_t gps_data = {0};
            gps_get_data(&gps_data);

            cJSON *root = cJSON_CreateObject();
            cJSON_AddNumberToObject(root, "heart_rate", health_data.heart_rate);
            cJSON_AddNumberToObject(root, "spo2", health_data.spo2);
            cJSON_AddNumberToObject(root, "temperature", temperature);
            cJSON_AddNumberToObject(root, "latitude", gps_data.latitude);
            cJSON_AddNumberToObject(root, "longitude", gps_data.longitude);
            char *json_data = cJSON_PrintUnformatted(root);

            esp_http_client_handle_t client = esp_http_client_init(&config);
            esp_http_client_set_header(client, "Content-Type", "application/json");
            esp_http_client_set_post_field(client, json_data, strlen(json_data));

            esp_err_t err = esp_http_client_perform(client);
            if (err == ESP_OK)
            {
                ESP_LOGI(TAG, "HTTP POST Status = %d", esp_http_client_get_status_code(client));
            }
            else
            {
                ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
            }

            esp_http_client_cleanup(client);
            cJSON_Delete(root);
            free(json_data);
        }
        else
        {
            ESP_LOGW(TAG, "WiFi not connected, skipping data send");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void http_client_init(void)
{
    ESP_LOGI(TAG, "HTTP client initialized (manual mode)");
}

void http_client_send_temp(float temperature)
{
    ESP_LOGI(TAG, "Sending temperature: %.2f", temperature);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temperature", roundf(temperature * 100) / 100);
    char *json_data = cJSON_PrintUnformatted(root);

    ESP_LOGI(TAG, "JSON payload: %s", json_data);

    esp_http_client_config_t config = {
        .url = "http://192.168.43.76:5000/update",
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000, 
        .buffer_size = 1024,
        .buffer_size_tx = 1024,
        .keep_alive_enable = false, 
        .disable_auto_redirect = true};

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Connection", "close");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    ESP_LOGI(TAG, "Starting HTTP POST request...");
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d", status_code, content_length);

        // Read server response
        char response_buffer[256];
        int data_read = esp_http_client_read_response(client, response_buffer, sizeof(response_buffer));
        if (data_read >= 0)
        {
            response_buffer[data_read] = '\0';
            ESP_LOGI(TAG, "Server response: %s", response_buffer);
        }
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(json_data);
}

void http_client_send_hr_spo2(int heart_rate, int spo2)
{
    ESP_LOGI(TAG, "Sending HR/SpO2: %d/%d", heart_rate, spo2);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "heart_rate", heart_rate);
    cJSON_AddNumberToObject(root, "spo2", spo2);
    char *json_data = cJSON_PrintUnformatted(root);

    ESP_LOGI(TAG, "JSON payload: %s", json_data);

    esp_http_client_config_t config = {
        .url = "http://192.168.43.76:5000/update",
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
        .buffer_size = 1024,
        .buffer_size_tx = 1024,
        .keep_alive_enable = false,
        .disable_auto_redirect = true};

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Connection", "close");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HR/SpO2 POST success - Status: %d", status);
    }
    else
    {
        ESP_LOGE(TAG, "HR/SpO2 POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(json_data);
}

void http_client_send_gps(float latitude, float longitude)
{
    if (!is_wifi_connected())
    {
        ESP_LOGW(TAG, "WiFi not connected, skipping GPS data send");
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "latitude", latitude);
    cJSON_AddNumberToObject(root, "longitude", longitude);
    char *json_data = cJSON_PrintUnformatted(root);

    esp_http_client_config_t config = {
        .url = "http://192.168.43.76:5000/update",
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "GPS POST Status = %d", esp_http_client_get_status_code(client));
    }
    else
    {
        ESP_LOGE(TAG, "GPS POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(json_data);
}