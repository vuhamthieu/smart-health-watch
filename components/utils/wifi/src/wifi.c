#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "wifi.h"
#include "esp_wifi.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define MAXIMUM_RETRY 5

bool isWifiConnected = false;
bool isWifiConnecting = false;

static const char *TAG = "wifi_station";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static char saved_ssid[33] = {0};
static char saved_password[65] = {0};
static bool wifi_connect_failed = false;


bool is_wifi_connect_failed(void) {
    return wifi_connect_failed;
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        s_retry_num = 0;               
        wifi_connect_failed = false; 
        esp_wifi_connect();
        isWifiConnecting = true;
        wifi_connect_failed = false; 
        ESP_LOGI(TAG, "WiFi STA started, connecting... isWifiConnected=%d, isWifiConnecting=%d", isWifiConnected, isWifiConnecting);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        isWifiConnected = false;
        isWifiConnecting = false;
        ESP_LOGI(TAG, "WiFi disconnected, isWifiConnected=%d, isWifiConnecting=%d", isWifiConnected, isWifiConnecting);
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            isWifiConnecting = true;
            ESP_LOGI(TAG, "Retrying to connect to the AP, attempt %d/%d", s_retry_num, MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            wifi_connect_failed = true; 
            ESP_LOGI(TAG, "Failed to connect to AP after %d attempts", MAXIMUM_RETRY);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        isWifiConnected = true;
        isWifiConnecting = false;
        wifi_connect_failed = false;
        ESP_LOGI(TAG, "WiFi connected, isWifiConnected=%d, isWifiConnecting=%d", isWifiConnected, isWifiConnecting);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP) {
        isWifiConnected = false;
        isWifiConnecting = false;
        wifi_connect_failed = false; 
        ESP_LOGI(TAG, "WiFi STA stopped, isWifiConnected=%d, isWifiConnecting=%d", isWifiConnected, isWifiConnecting);
    }
}

esp_err_t wifi_init(const char* ssid, const char* password) {
    strncpy(saved_ssid, ssid, sizeof(saved_ssid));
    strncpy(saved_password, password, sizeof(saved_password));

    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    // Set up wifi but not turn on
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_LOGI(TAG, "WiFi initialized with SSID: %s", ssid);
    return ESP_OK;
}

esp_err_t wifi_disconnect(void) {
    isWifiConnected = false;
    isWifiConnecting = false;
    ESP_LOGI(TAG, "Disconnecting WiFi...");
    esp_err_t ret = esp_wifi_disconnect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disconnect WiFi: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t wifi_stop(void) {
    isWifiConnected = false;
    isWifiConnecting = false;
    
    s_retry_num = 0;                   
    wifi_connect_failed = false;        
    
    ESP_LOGI(TAG, "Stopping WiFi...");
    esp_err_t ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop WiFi: %s", esp_err_to_name(ret));
    }
    return ret;
}


esp_err_t wifi_start(void) {
    ESP_LOGI(TAG, "Starting WiFi...");
    
    s_retry_num = 0;                   
    wifi_connect_failed = false;      
    isWifiConnecting = true;
    
    esp_err_t ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        isWifiConnecting = false;
        return ret;
    }
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect WiFi: %s", esp_err_to_name(ret));
        isWifiConnecting = false;
    }
    return ret;
}

bool is_wifi_connected(void) {
    return isWifiConnected;
}

bool is_wifi_connecting(void) {
    return isWifiConnecting;
}