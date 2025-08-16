#include "bluetooth.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "BLE_HEALTH";

// BLE connection state
static bool s_ble_connected = false;
static uint16_t s_conn_id = 0;
static uint16_t s_gatts_if = 0;

// Service and characteristic handles
static uint16_t s_service_handle = 0;
static uint16_t s_char_hr_handle = 0;
static uint16_t s_char_temp_handle = 0;
static uint16_t s_char_spo2_handle = 0;
static uint16_t s_char_gps_handle = 0;
static uint16_t s_char_cmd_handle = 0;
static uint16_t s_char_notify_handle = 0;

// Advertising data
static esp_ble_adv_data_t s_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(uint16_t),
    .p_service_uuid = (uint8_t[]){HEALTH_SERVICE_UUID & 0xFF, (HEALTH_SERVICE_UUID >> 8) & 0xFF},
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t s_adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// GATT attribute database
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_write_notify = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

// Service UUID
static const uint16_t health_service_uuid = HEALTH_SERVICE_UUID;

// Characteristic UUIDs
static const uint16_t hr_char_uuid = HEALTH_CHAR_HR_UUID;
static const uint16_t temp_char_uuid = HEALTH_CHAR_TEMP_UUID;
static const uint16_t spo2_char_uuid = HEALTH_CHAR_SPO2_UUID;
static const uint16_t gps_char_uuid = HEALTH_CHAR_GPS_UUID;
static const uint16_t cmd_char_uuid = HEALTH_CHAR_CMD_UUID;
static const uint16_t notify_char_uuid = HEALTH_CHAR_NOTIFY_UUID;

// Attribute table
enum {
    HEALTH_IDX_SVC,

    HEALTH_IDX_HR_CHAR,
    HEALTH_IDX_HR_VAL,
    HEALTH_IDX_HR_CFG,

    HEALTH_IDX_TEMP_CHAR,
    HEALTH_IDX_TEMP_VAL,
    HEALTH_IDX_TEMP_CFG,

    HEALTH_IDX_SPO2_CHAR,
    HEALTH_IDX_SPO2_VAL,
    HEALTH_IDX_SPO2_CFG,

    HEALTH_IDX_GPS_CHAR,
    HEALTH_IDX_GPS_VAL,
    HEALTH_IDX_GPS_CFG,

    HEALTH_IDX_CMD_CHAR,
    HEALTH_IDX_CMD_VAL,

    HEALTH_IDX_NOTIFY_CHAR,
    HEALTH_IDX_NOTIFY_VAL,
    HEALTH_IDX_NOTIFY_CFG,

    HEALTH_IDX_NB,
};

static uint16_t s_handle_table[HEALTH_IDX_NB];

static const esp_gatts_attr_db_t gatt_db[HEALTH_IDX_NB] = {
    // Service Declaration
    [HEALTH_IDX_SVC] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(health_service_uuid), (uint8_t *)&health_service_uuid}},

    // Heart Rate Characteristic Declaration
    [HEALTH_IDX_HR_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, sizeof(char_prop_read_notify), sizeof(char_prop_read_notify), (uint8_t *)&char_prop_read_notify}},
    // Heart Rate Characteristic Value
    [HEALTH_IDX_HR_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hr_char_uuid, ESP_GATT_PERM_READ, 4, 0, NULL}},
    // Heart Rate Client Characteristic Configuration Descriptor
    [HEALTH_IDX_HR_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), 0, NULL}},

    // Temperature Characteristic Declaration
    [HEALTH_IDX_TEMP_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, sizeof(char_prop_read_notify), sizeof(char_prop_read_notify), (uint8_t *)&char_prop_read_notify}},
    // Temperature Characteristic Value
    [HEALTH_IDX_TEMP_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&temp_char_uuid, ESP_GATT_PERM_READ, 4, 0, NULL}},
    // Temperature Client Characteristic Configuration Descriptor
    [HEALTH_IDX_TEMP_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), 0, NULL}},

    // SpO2 Characteristic Declaration
    [HEALTH_IDX_SPO2_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, sizeof(char_prop_read_notify), sizeof(char_prop_read_notify), (uint8_t *)&char_prop_read_notify}},
    // SpO2 Characteristic Value
    [HEALTH_IDX_SPO2_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&spo2_char_uuid, ESP_GATT_PERM_READ, 4, 0, NULL}},
    // SpO2 Client Characteristic Configuration Descriptor
    [HEALTH_IDX_SPO2_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), 0, NULL}},

    // GPS Characteristic Declaration
    [HEALTH_IDX_GPS_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, sizeof(char_prop_read_notify), sizeof(char_prop_read_notify), (uint8_t *)&char_prop_read_notify}},
    // GPS Characteristic Value
    [HEALTH_IDX_GPS_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&gps_char_uuid, ESP_GATT_PERM_READ, 16, 0, NULL}},
    // GPS Client Characteristic Configuration Descriptor
    [HEALTH_IDX_GPS_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), 0, NULL}},

    // Command Characteristic Declaration
    [HEALTH_IDX_CMD_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, sizeof(char_prop_read_write), sizeof(char_prop_read_write), (uint8_t *)&char_prop_read_write}},
    // Command Characteristic Value
    [HEALTH_IDX_CMD_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&cmd_char_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, 32, 0, NULL}},

    // Notification Characteristic Declaration (for phone notifications)
    [HEALTH_IDX_NOTIFY_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, sizeof(char_prop_write_notify), sizeof(char_prop_write_notify), (uint8_t *)&char_prop_write_notify}},
    // Notification Characteristic Value
    [HEALTH_IDX_NOTIFY_VAL] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&notify_char_uuid, ESP_GATT_PERM_WRITE, 256, 0, NULL}},
    // Notification Client Characteristic Configuration Descriptor
    [HEALTH_IDX_NOTIFY_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), 0, NULL}},
};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&s_adv_params);
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising start failed");
        } else {
            ESP_LOGI(TAG, "Advertising started - Device name: Health Monitor");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        ESP_LOGI(TAG, "Advertising stopped");
        break;
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d", param->reg.status, param->reg.app_id);
        s_gatts_if = gatts_if;
        esp_ble_gap_set_device_name("Health Monitor");
        esp_ble_gap_config_adv_data(&s_adv_data);
        esp_ble_gatts_create_attr_table(gatt_db, gatts_if, HEALTH_IDX_NB, 0);
        break;

    case ESP_GATTS_CREATE_EVT:
        if (param->add_attr_tab.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "Create attribute table failed, error code=0x%x", param->add_attr_tab.status);
        } else if (param->add_attr_tab.num_handle != HEALTH_IDX_NB) {
            ESP_LOGE(TAG, "Create attribute table abnormally, num_handle (%d) doesn't equal to HEALTH_IDX_NB(%d)", param->add_attr_tab.num_handle, HEALTH_IDX_NB);
        } else {
            ESP_LOGI(TAG, "Create attribute table successfully, the number handle = %d", param->add_attr_tab.num_handle);
            memcpy(s_handle_table, param->add_attr_tab.handles, sizeof(s_handle_table));
            esp_ble_gatts_start_service(s_handle_table[HEALTH_IDX_SVC]);
        }
        break;

    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
        s_conn_id = param->connect.conn_id;
        s_ble_connected = true;
        esp_ble_gap_stop_advertising();
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
        s_ble_connected = false;
        esp_ble_gap_start_advertising(&s_adv_params);
        break;

    case ESP_GATTS_WRITE_EVT:
        ESP_LOGI(TAG, "GATT_WRITE_EVT, handle = %d, value len = %d", param->write.handle, param->write.len);
        if (param->write.handle == s_handle_table[HEALTH_IDX_CMD_VAL]) {
            ESP_LOGI(TAG, "Command received: %.*s", param->write.len, param->write.value);
            // Handle commands here (e.g., change sampling rate, calibrate, etc.)
        } else if (param->write.handle == s_handle_table[HEALTH_IDX_NOTIFY_VAL]) {
            ESP_LOGI(TAG, "Notification received from phone: %.*s", param->write.len, param->write.value);
            // Handle phone notifications here (display on watch screen)
        }
        break;

    default:
        break;
    }
}

esp_err_t bluetooth_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing Bluetooth...");

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
        return ret;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "gap register error, error code = %x", ret);
        return ret;
    }

    ret = esp_ble_gatts_app_register(0);
    if (ret) {
        ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Bluetooth initialized successfully");
    return ESP_OK;
}

esp_err_t bluetooth_start_advertising(void)
{
    return esp_ble_gap_start_advertising(&s_adv_params);
}

esp_err_t bluetooth_stop_advertising(void)
{
    return esp_ble_gap_stop_advertising();
}

esp_err_t bluetooth_notify_heart_rate(uint16_t hr, uint8_t spo2)
{
    if (!s_ble_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t hr_data[4];
    hr_data[0] = hr & 0xFF;
    hr_data[1] = (hr >> 8) & 0xFF;
    hr_data[2] = spo2;
    hr_data[3] = 0; // Reserved

    return esp_ble_gatts_send_indicate(s_gatts_if, s_conn_id, s_handle_table[HEALTH_IDX_HR_VAL],
                                       sizeof(hr_data), hr_data, false);
}

esp_err_t bluetooth_notify_temperature(float temp)
{
    if (!s_ble_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    // Convert float to bytes (IEEE 754)
    union {
        float f;
        uint8_t bytes[4];
    } temp_union;
    temp_union.f = temp;

    return esp_ble_gatts_send_indicate(s_gatts_if, s_conn_id, s_handle_table[HEALTH_IDX_TEMP_VAL],
                                       4, temp_union.bytes, false);
}

esp_err_t bluetooth_notify_gps(float lat, float lon)
{
    if (!s_ble_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    // Pack lat/lon as 2 floats (8 bytes total)
    union {
        struct {
            float lat;
            float lon;
        } coords;
        uint8_t bytes[8];
    } gps_union;
    gps_union.coords.lat = lat;
    gps_union.coords.lon = lon;

    return esp_ble_gatts_send_indicate(s_gatts_if, s_conn_id, s_handle_table[HEALTH_IDX_GPS_VAL],
                                       8, gps_union.bytes, false);
}

esp_err_t bluetooth_send_notification(const char* title, const char* message)
{
    if (!s_ble_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    // Format: "TITLE|MESSAGE" (max 256 bytes)
    char notification_data[256];
    snprintf(notification_data, sizeof(notification_data), "%s|%s", title, message);

    return esp_ble_gatts_send_indicate(s_gatts_if, s_conn_id, s_handle_table[HEALTH_IDX_NOTIFY_VAL],
                                       strlen(notification_data), (uint8_t*)notification_data, false);
}

bool bluetooth_is_connected(void)
{
    return s_ble_connected;
}