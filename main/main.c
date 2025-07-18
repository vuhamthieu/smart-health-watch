#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include <esp_heap_caps.h>

#include "leaf_config.h"

#include "temperature_tracker.h" 

static const char* MAIN_TAG = "MAIN ***";

esp_err_t setup(void) {
    esp_err_t ret = ESP_OK;
    ESP_LOGI(MAIN_TAG, "Starting setup");
    
    size_t free_heap_size = esp_get_free_heap_size();
    ESP_LOGI(MAIN_TAG, "Free heap size: %zu bytes", free_heap_size);
    
    ESP_LOGI(MAIN_TAG, "Setting up NVS from flash");
    esp_err_t nvs_err = nvs_flash_init();
    
    if (nvs_err != ESP_OK) {
        ESP_LOGE(MAIN_TAG, "NVS flash initialization failed");
        ret = nvs_err;
    }

    ÉSP_LOGI(MAIN_TAG, "Initializing leaf config");
    initialize_leaf_config();
    
    if (ret != ESP_OK) {
        ESP_LOGE(MAIN_TAG, "Setup did not succeed");
    }
    
    return ret;
}

void app_main(void) {
    ESP_LOGI(MAIN_TAG, "Initializing main function");
    if (setup() != ESP_OK) {
        ESP_LOGE(MAIN_TAG, "Setup failed, exiting app_main");
        return;
    }

    ESP_LOGI(MAIN_TAG, "Setup completed successfully");
    xTaskCreate(
        temperature_tracker_task,
        "temperature_tracker",  // Task name
        2048,                   // Stack size in bytes
        NULL,                  // Task parameters
        5,                     // Task priority
        NULL                   // Task handle 
    );


    // xTaskCreate() // Here we will be initializing the temperature tracker task
    /* We have indeed other options or functions to create a task:
       xTaskCreatePinnedToCore - allows us to choose which core for each task
       xTaskCreateStatic - similar to xTaskCreate with the difference that it avoids dynamic allocation
    */
}