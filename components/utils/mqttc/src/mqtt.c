#include "mqtt_client.h"
#include "esp_log.h"
#include "mqtt.h"
#include "wifi.h"

static const char* TAG = "mqttc";
static esp_mqtt_client_handle_t s_client = NULL;
static bool s_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t eid, void *event_data) {
    esp_mqtt_event_handle_t e = event_data;
    switch (e->event_id) {
    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "Connected");
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "Disconnected");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "RX [%.*s]=[%.*s]", e->topic_len, e->topic, e->data_len, e->data);
        break;
    default:
        break;
    }
}

esp_err_t mqttc_init(const char* uri, const char* client_id) {
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = uri,
        .credentials.client_id = client_id,
        .session.keepalive = 60,
        .session.last_will.topic = NULL,
    };
    s_client = esp_mqtt_client_init(&cfg);
    if (!s_client) return ESP_FAIL;
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    return ESP_OK;
}

esp_err_t mqttc_start(void) {
    if (!s_client) return ESP_ERR_INVALID_STATE;
    if (!is_wifi_connected()) return ESP_ERR_INVALID_STATE;
    return esp_mqtt_client_start(s_client);
}

esp_err_t mqttc_stop(void) {
    if (!s_client) return ESP_ERR_INVALID_STATE;
    s_connected = false;
    return esp_mqtt_client_stop(s_client);
}

bool mqttc_is_connected(void) { return s_connected; }

int mqttc_publish(const char* topic, const char* payload, int qos, bool retain) {
    if (!s_client) return -1;
    return esp_mqtt_client_publish(s_client, topic, payload, 0, qos, retain);
}

int mqttc_subscribe(const char* topic, int qos) {
    if (!s_client) return -1;
    return esp_mqtt_client_subscribe(s_client, topic, qos);
}