#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "i2c_common.h"
#include "button.h"
#include "ui_manager.h"
#include "health_tracker.h"
#include "temperature_task.h"
#include "gps_tracker.h"
#include "esp_log.h"
#include "string.h"
#include "wifi.h"
#include "nvs_flash.h"
#include "http_client.h"
#include "mqtt.h"
#include "mqtt_task.h"
#include "bluetooth.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

SemaphoreHandle_t i2c_mutex = NULL;
SemaphoreHandle_t http_semaphore = NULL;
QueueHandle_t http_queue = NULL;

#include "lvgl.h"
#include "lvgl_helpers.h"

static uint32_t last_interaction_ms = 0;

static bool screen_off = false;

static ui_manager_t ui;

float raw_hr, raw_sp;

float temperature_get_data_protected()
{
    float temp = 0.0;
    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
    {
        temp = temperature_get_data();
        xSemaphoreGive(i2c_mutex);
        // ESP_LOGI("TEMP", "Protected read: %.2f°C", temp);
    }
    else
    {
        ESP_LOGW("TEMP", "Failed to acquire I2C mutex for temperature");
    }
    return temp;
}

bool health_get_data_protected(health_data_t *health_data)
{
    bool success = false;
    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
    {
        health_update();
        health_get_data(health_data);
        success = true;
        xSemaphoreGive(i2c_mutex);
        // ESP_LOGI("HEALTH", "Protected read: HR=%d, SpO2=%d",
        //          health_data->heart_rate, health_data->spo2);
    }
    else
    {
        ESP_LOGW("HEALTH", "Failed to acquire I2C mutex for health");
    }
    return success;
}


void temperature_update_protected()
{
    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(500)) == pdTRUE)
    {
        temperature_update();
        xSemaphoreGive(i2c_mutex);
    }
    else
    {
        ESP_LOGW("TEMP", "Failed to acquire I2C mutex for temperature update");
    }
}

void update_interaction_time()
{
    last_interaction_ms = esp_timer_get_time() / 1000;
}

void check_screen_timeout(void *pvParameters)
{
    for (;;)
    {
        uint32_t current_time_ms = esp_timer_get_time() / 1000;
        if (!screen_off && (current_time_ms - last_interaction_ms > SCREEN_TIMEOUT_MS))
        {
            gpio_set_level(TFT_BL_PIN, 0);
            screen_off = true;
            ESP_LOGI("SCREEN", "Screen turned off due to inactivity");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void wifi_status_task(void *pv)
{
    for (;;)
    {
        ui_update_wifi_status(&ui);
        ui_update_home_wifi_icon(&ui);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// LVGL tick function
static void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(1);
}

static void handle_button(button_id_t btn)
{
    if (screen_off)
    {
        gpio_set_level(TFT_BL_PIN, 1); // Screen on
        screen_off = false;
        ESP_LOGI("SCREEN", "Screen turned on due to button press");
    }
    update_interaction_time();
    ui_manager_handle_button(&ui, btn);
}

static void gui_task(void *pv)
{
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
void sensor_manager_task(void *pv)
{
    bool loggedResult = false;
    static uint32_t last_http_send = 0;
    const uint32_t HTTP_SEND_INTERVAL = 3000;

    ESP_LOGI("SENSOR_MANAGER", "Task started with queue-based HTTP system");

    for (;;)
    {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

        switch (ui.current_state)
        {
        case UI_STATE_TEMP_SCANNING:
        {
            loggedResult = false;
            temperature_update_protected();
            ui_update_temp(&ui, 0);

            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (!ui.scan_done && now - ui.scan_start_time_ms >= 10000)
            {
                ui.scan_done = true;
                ui.current_state = UI_STATE_TEMP_RESULT;
            }
            break;
        }

        case UI_STATE_TEMP_RESULT:
        {
            if (!loggedResult && (current_time - last_http_send >= HTTP_SEND_INTERVAL))
            {
                float t = temperature_get_data_protected();
                ui_update_temp(&ui, t);

                if (is_wifi_connected())
                {
                    http_message_t msg = {
                        .data_type = 0,
                        .data.temperature = t};

                    if (xQueueSend(http_queue, &msg, 0) == pdTRUE)
                    {
                        ESP_LOGI("SENSOR", "Temperature queued for HTTP send: %.2f", t);
                        last_http_send = current_time;
                    }
                    else
                    {
                        ESP_LOGW("SENSOR", "HTTP queue full, skipping temperature send");
                    }
                }

                // Send via Bluetooth
                if (bluetooth_is_connected())
                {
                    bluetooth_notify_temperature(t);
                    ESP_LOGI("SENSOR", "Temperature sent via BLE: %.2f", t);
                }

                loggedResult = true;
            }
            break;
        }

        case UI_STATE_HR:
        {
            health_data_t hd = {0};
            if (health_get_data_protected(&hd)) // Use protected version
            {
                if ((hd.heart_rate != raw_hr || hd.spo2 != raw_sp) &&
                    (current_time - last_http_send >= HTTP_SEND_INTERVAL))
                {
                    printf("%d,%d\n", hd.heart_rate, hd.spo2);
                    raw_hr = hd.heart_rate;
                    raw_sp = hd.spo2;

                    // Queue HTTP request
                    if (is_wifi_connected())
                    {
                        http_message_t msg = {
                            .data_type = 1,
                            .data.health = {hd.heart_rate, hd.spo2}};

                        if (xQueueSend(http_queue, &msg, 0) == pdTRUE)
                        {
                            ESP_LOGI("SENSOR", "Health data queued: HR=%d, SpO2=%d",
                                     hd.heart_rate, hd.spo2);
                            last_http_send = current_time;
                        }
                        else
                        {
                            ESP_LOGW("SENSOR", "HTTP queue full, skipping health send");
                        }
                    }

                    // Send via Bluetooth
                    if (bluetooth_is_connected())
                    {
                        bluetooth_notify_heart_rate(hd.heart_rate, hd.spo2);
                        ESP_LOGI("SENSOR", "Health data sent via BLE: HR=%d, SpO2=%d", hd.heart_rate, hd.spo2);
                    }
                }

                ui_update_hr(&ui, hd.heart_rate, hd.spo2);
            }
            break;
        }

        case UI_STATE_HOME:
        case UI_STATE_MENU:
        case UI_STATE_WIFI:
        case UI_STATE_NOTIFY:
        case UI_STATE_DATA:
        case UI_STATE_BLUETOOTH:
        case UI_STATE_TEMP_IDLE:
            loggedResult = false;
            break;

        default:
            ESP_LOGW("SENSOR_MANAGER", "Unknown UI state: %d", ui.current_state);
            loggedResult = false;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void app_main(void)
{
    esp_err_t err = i2c_master_init();
    if (err != ESP_OK)
    {
        ESP_LOGE("MAIN", "I2C init failed: %s", esp_err_to_name(err));
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    const char *ssid = "Cafe nui 2.4G";
    const char *pass = "nguyenchat";
    // const char *ssid = "Samsung Galaxy S8";
    // const char *pass = "88888888";

    esp_err_t wifi_ok = wifi_init(ssid, pass);
    if (wifi_ok != ESP_OK)
    {
        ESP_LOGE("MAIN", "WiFi init failed: %s", esp_err_to_name(wifi_ok));
    }
    else
    {
        ESP_LOGI("MAIN", "WiFi initialized, waiting for user to start.");
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
    temperature_init();
    health_init();
    http_client_init();

    // Initialize Bluetooth
    esp_err_t ble_ret = bluetooth_init();
    
    if (ble_ret != ESP_OK)
    {
        ESP_LOGE("MAIN", "Bluetooth init failed: %s", esp_err_to_name(ble_ret));
    }
    else
    {
        ESP_LOGI("MAIN", "Bluetooth initialized successfully");

        vTaskDelay(pdMS_TO_TICKS(2000));

        esp_err_t adv_ret = bluetooth_start_advertising();
        if (adv_ret != ESP_OK)
        {
            ESP_LOGE("MAIN", "Failed to start advertising: %s", esp_err_to_name(adv_ret));
        }
        else
        {
            ESP_LOGI("MAIN", "Bluetooth advertising started - Device discoverable as 'Health Monitor'");
        }
    }

    // Create synchronization objects
    ESP_LOGI("MAIN", "Creating synchronization objects...");
    i2c_mutex = xSemaphoreCreateMutex();
    http_semaphore = xSemaphoreCreateBinary();
    http_queue = xQueueCreate(10, sizeof(http_message_t));

    if (i2c_mutex == NULL || http_semaphore == NULL || http_queue == NULL)
    {
        ESP_LOGE("MAIN", "Failed to create synchronization objects");
        while (1)
            vTaskDelay(pdMS_TO_TICKS(1000));
    }

    xSemaphoreGive(http_semaphore); // Initialize as available
    ESP_LOGI("MAIN", "Synchronization objects created successfully");

    /* ====== LVGL Initialization ====== */
    lv_init();

    // Setup LVGL tick timer
    const esp_timer_create_args_t lv_tick_timer = {
        .callback = &lv_tick_task,
        .name = "lv_tick"};
    esp_timer_handle_t h;
    ESP_ERROR_CHECK(esp_timer_create(&lv_tick_timer, &h));
    ESP_ERROR_CHECK(esp_timer_start_periodic(h, 1000));

    lvgl_driver_init();

    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[128 * 10];
    static lv_color_t buf2[128 * 10];
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 128 * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.hor_res = 128;
    disp_drv.ver_res = 160;
    lv_disp_drv_register(&disp_drv);

    // Enable backlight
    gpio_set_direction(TFT_BL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TFT_BL_PIN, 1);
    update_interaction_time();

    /* ====== UI Initialization ====== */
    ui_manager_init(&ui);
    button_init(handle_button);

    /* ====== Create Tasks ====== */
    ESP_LOGI("MAIN", "Creating tasks with proper priorities...");

    BaseType_t ret1 = xTaskCreatePinnedToCore(gui_task, "gui", 8192, NULL, 3, NULL, 0);
    BaseType_t ret2 = xTaskCreate(sensor_manager_task, "sensor", 4096, NULL, 4, NULL);
    BaseType_t ret3 = xTaskCreate(mqtt_client_task, "mqtt_client", 4096, NULL, 2, NULL);
    BaseType_t ret4 = xTaskCreate(check_screen_timeout, "screen_timeout", 2048, NULL, 1, NULL);
    BaseType_t ret5 = xTaskCreate(wifi_status_task, "wifi_status", 2048, NULL, 1, NULL);

    if (ret1 != pdPASS || ret2 != pdPASS || ret3 != pdPASS || ret4 != pdPASS || ret5 != pdPASS)
    {
        ESP_LOGE("MAIN", "Failed to create one or more tasks");
        while (1)
            vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI("MAIN", "All tasks created successfully");

    ESP_LOGI("MAIN", "System initialized with LVGL");
}
