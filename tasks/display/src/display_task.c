// #include "sdkconfig.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "u8g2.h"
// #include "u8g2_esp32_hal.h"
// #include "ui_manager.h"
// #include "esp_log.h"

// #define TAG "DISPLAY"

// void display_task(void *pvParameters) {
//     ui_manager_t *ui = (ui_manager_t *)pvParameters;

//     while (1) {
//         ui_manager_update_display(ui);
//         vTaskDelay(pdMS_TO_TICKS(100)); 
//     }
// }