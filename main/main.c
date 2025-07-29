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

#include "lvgl.h"
#include "lvgl_helpers.h"


static uint32_t last_interaction_ms = 0;

static bool screen_off = false;

static ui_manager_t ui;

float raw_hr, raw_sp;

void update_interaction_time() {
    last_interaction_ms = esp_timer_get_time() / 1000;
}

void check_screen_timeout(void *pvParameters) {
    for (;;) {
        uint32_t current_time_ms = esp_timer_get_time() / 1000;
        if (!screen_off && (current_time_ms - last_interaction_ms > SCREEN_TIMEOUT_MS)) {
            gpio_set_level(TFT_BL_PIN, 0); 
            screen_off = true;
            ESP_LOGI("SCREEN", "Screen turned off due to inactivity");
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}


// LVGL tick function
static void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(1); // Increment LVGL tick by 1ms
}

static void handle_button(button_id_t btn) {
    if (screen_off) {
        gpio_set_level(TFT_BL_PIN, 1); // Screen on
        screen_off = false;
        ESP_LOGI("SCREEN", "Screen turned on due to button press");
    }
    update_interaction_time();
    ui_manager_handle_button(&ui, btn);
}

static void gui_task(void *pv) {
    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void sensor_manager_task(void *pv)
{
    //ESP_LOGI("SENSOR_MANAGER", "Task started");

    bool loggedResult = false;
    for (;;)
    {
        switch (ui.current_state)
        {
        case UI_STATE_TEMP_SCANNING:
        {
            loggedResult = false;
            temperature_update();

            ui_update_temp(&ui, 0);

            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (!ui.scan_done && now - ui.scan_start_time_ms >= 10000)
            {
                ui.scan_done = true;
                ui.current_state = UI_STATE_TEMP_RESULT;
                //ESP_LOGI("SENSOR", "Scan done → STATE_TEMP_RESULT");
            }
            break;
        }

        case UI_STATE_TEMP_RESULT:
        {
            if (!loggedResult)
            {
                float t = temperature_get_data();
                printf("%.2f\n", t);

                //ESP_LOGI("SENSOR", "-- TEMP_RESULT → %.2f°C", t);

                ui_update_temp(&ui, t);

                loggedResult = true;
            }
            break;
        }

        case UI_STATE_HR:
        {
            health_update();

            health_data_t hd;

            health_get_data(&hd);

            if (hd.heart_rate != raw_hr || hd.spo2 != raw_sp)
            {
                printf("%d,%d\n", hd.heart_rate, hd.spo2);
                raw_hr = hd.heart_rate;
                raw_sp = hd.spo2;
            }

            ui_update_hr(&ui, hd.heart_rate, hd.spo2);

            // printf("%d,%d\n", hd.heart_rate, hd.spo2);

            ui_update_hr(&ui, hd.heart_rate, hd.spo2);

            // ESP_LOGI("SENSOR", "HR=%d bpm, SpO2=%d%%", hd.heart_rate, hd.spo2);
            break;
        }

        case UI_STATE_GPS:
        {
            ESP_LOGI("SENSOR", "-- STATE_GPS --");
            gps_data_t gps;
            gps_get_data(&gps);

            ui_update_gps(&ui, gps.latitude, gps.longitude, gps.valid);

            if (gps.valid)
            {
                ESP_LOGI("SENSOR", "Lat=%.6f, Lon=%.6f", gps.latitude, gps.longitude);
            }
            else
            {
                ESP_LOGI("SENSOR", "GPS no signal");
            }
            break;
        }

        case UI_STATE_HOME:
        case UI_STATE_MENU:
        case UI_STATE_TEMP_IDLE:
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

    vTaskDelay(pdMS_TO_TICKS(1000));
    gps_init();
    temperature_init();
    health_init();

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
    xTaskCreatePinnedToCore(gui_task, "gui", 4096, NULL, 2, NULL, 0); // Pin to Core 0
    xTaskCreate(sensor_manager_task, "sensor", 4096, NULL, 5, NULL);
    xTaskCreate(check_screen_timeout, "screen_timeout", 2048, NULL, 5, NULL);

    ESP_LOGI("MAIN", "System initialized with LVGL");
}

/*
#include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "i2c_common.h"
 #include "button.h"
 #include "ui_manager.h"
 #include "esp_log.h"
 #include "u8g2_esp32_hal.h"
 #include "u8g2.h"
 #include "gps_tracker.h"
 #include "health_tracker.h"
 #include "temperature_task.h"
 #include "display_task.h"

 static ui_manager_t ui;
 static u8g2_t      u8g2;

 static void handle_button(button_id_t btn) {
     ui_manager_handle_button(&ui, btn);
 }

 void sensor_manager_task(void *pv)
 {
     for (;;) {
         switch (ui.current_state) {
             case UI_STATE_GPS:
                 gps_update();
                 break;
             case UI_STATE_TEMP_SCANNING: {
                 temperature_update();
                 uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
                 if (!ui.scan_done && now - ui.scan_start_time_ms >= 10000) {
                     ui.scan_done = true;
                     ui.current_state = UI_STATE_TEMP_RESULT;
                 }
                 break;
             }
             case UI_STATE_HR:
                 health_update();
                 break;
             default:
                 break;
         }

         vTaskDelay(pdMS_TO_TICKS(30));
     }
 }

 void app_main(void) {
     esp_err_t err = i2c_master_init();
     if (err != ESP_OK) {
         ESP_LOGE("MAIN", "I2C init failed: %s", esp_err_to_name(err));
         while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
     }

     vTaskDelay(pdMS_TO_TICKS(1000));
     gps_init();
     temperature_init();
     health_init();

     u8g2_esp32_hal_t hal = { .sda = 21, .scl = 22 };
     u8g2_esp32_hal_init(&hal);

     vTaskDelay(pdMS_TO_TICKS(1000));
     u8g2_Setup_ssd1306_i2c_128x64_noname_f(
         &u8g2, U8G2_R0,
         u8g2_esp32_i2c_byte_cb,
         u8g2_esp32_gpio_and_delay_cb
     );
     u8g2_InitDisplay(&u8g2);
     u8g2_SetPowerSave(&u8g2, 0);

     ui_manager_init(&ui, &u8g2);
     button_init(handle_button);

     xTaskCreate(sensor_manager_task, "sensor", 4096, NULL, 5, NULL);
     xTaskCreate(display_task,        "disp",   4096, &ui,  5, NULL);
 }
*/