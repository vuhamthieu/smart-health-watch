#include "ui_manager.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "button.h"
#include "gps_tracker.h"
#include "health_tracker.h"
#include "temperature_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "UI_MANAGER";

// Animation callback cho heart blink
static void anim_heart_cb(void *obj, int32_t v) {
    lv_obj_set_style_opa(obj, v, LV_PART_MAIN);
}

void ui_manager_init(ui_manager_t *ui)
{
    memset(ui, 0, sizeof(*ui));
    ui->menu_item_count = 3;
    ui->selected_index = 0;  // Initialize selected index
    ui->menu_items[0] = (menu_item_t){"Foot Tracking",   UI_STATE_GPS};
    ui->menu_items[1] = (menu_item_t){"Body Temperature",UI_STATE_TEMP_IDLE};
    ui->menu_items[2] = (menu_item_t){"Heart Rate+SpO2", UI_STATE_HR};

    /* ---------- HOME ---------- */
    ui->scr_home = lv_obj_create(NULL);
    lv_obj_t *lbl_home = lv_label_create(ui->scr_home);
    lv_label_set_text(lbl_home, "Fitness Watch");
    lv_obj_center(lbl_home);

    /* Status bar - hiển thị trên mọi screen */
    ui->lbl_date = lv_label_create(ui->scr_home);
    lv_label_set_text(ui->lbl_date, "2026-01-01");
    lv_obj_align(ui->lbl_date, LV_ALIGN_TOP_LEFT, 4, 4);

    ui->lbl_battery = lv_label_create(ui->scr_home);
    lv_label_set_text_fmt(ui->lbl_battery, LV_SYMBOL_BATTERY_FULL " 100%%");
    lv_obj_align(ui->lbl_battery, LV_ALIGN_TOP_RIGHT, -4, 4);

    /* ---------- MENU ---------- */
    ui->scr_menu = lv_obj_create(NULL);
    ui->list_menu = lv_list_create(ui->scr_menu);
    for (int i = 0; i < ui->menu_item_count; ++i)
        lv_list_add_btn(ui->list_menu, LV_SYMBOL_RIGHT, ui->menu_items[i].name);
    lv_obj_set_size(ui->list_menu, 120, 130);
    lv_obj_center(ui->list_menu);

    /* ---------- TEMP ---------- */
    ui->scr_temp = lv_obj_create(NULL);
    lv_obj_t *lbl_temp_title = lv_label_create(ui->scr_temp);
    lv_label_set_text(lbl_temp_title, "BODY TEMPERATURE");
    lv_obj_align(lbl_temp_title, LV_ALIGN_TOP_MID, 0, 20);
    
    ui->lbl_temp = lv_label_create(ui->scr_temp);
    lv_label_set_text(ui->lbl_temp, "Press SELECT to scan");
    lv_obj_center(ui->lbl_temp);

    /* ---------- HR ---------- */
    ui->scr_hr = lv_obj_create(NULL);
    lv_obj_t *lbl_hr_title = lv_label_create(ui->scr_hr);
    lv_label_set_text(lbl_hr_title, "HEART RATE + SPO2");
    lv_obj_align(lbl_hr_title, LV_ALIGN_TOP_MID, 0, 20);

    ui->lbl_hr = lv_label_create(ui->scr_hr);
    lv_label_set_text(ui->lbl_hr, "HR: -- bpm");
    lv_obj_align(ui->lbl_hr, LV_ALIGN_CENTER, 0, -10);

    ui->lbl_spo2 = lv_label_create(ui->scr_hr);
    lv_label_set_text(ui->lbl_spo2, "SpO2: --%");
    lv_obj_align(ui->lbl_spo2, LV_ALIGN_CENTER, 0, +10);

    // Heart icon (dùng ký tự thay vì image cho đơn giản)
    ui->img_heart = lv_label_create(ui->scr_hr);
    lv_label_set_text(ui->img_heart, "♥");
    lv_obj_set_style_text_color(ui->img_heart, lv_color_hex(0xFF0000), LV_PART_MAIN);
    lv_obj_align(ui->img_heart, LV_ALIGN_CENTER, 50, -10);

    // Start heart animation
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui->img_heart);
    lv_anim_set_exec_cb(&a, anim_heart_cb);
    lv_anim_set_values(&a, LV_OPA_30, LV_OPA_100);
    lv_anim_set_time(&a, 500);
    lv_anim_set_playback_time(&a, 500);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);

    /* ---------- GPS ---------- */
    ui->scr_gps = lv_obj_create(NULL);
    lv_obj_t *lbl_gps_title = lv_label_create(ui->scr_gps);
    lv_label_set_text(lbl_gps_title, "FOOT TRACKING");
    lv_obj_align(lbl_gps_title, LV_ALIGN_TOP_MID, 0, 20);

    ui->lbl_gps = lv_label_create(ui->scr_gps);
    lv_label_set_text(ui->lbl_gps, "No signal");
    lv_obj_center(ui->lbl_gps);

    /* Load home screen */
    ui->current_state = UI_STATE_HOME;
    lv_scr_load(ui->scr_home);
    ESP_LOGI(TAG, "LVGL UI Manager initialized");
}

void ui_manager_handle_button(ui_manager_t *ui, button_id_t btn)
{
    // Add comprehensive safety check
    if (ui == NULL) {
        ESP_LOGE("UI_MANAGER", "UI manager is NULL in button handler");
        return;
    }
    
    ESP_LOGI("UI_MANAGER", "Button %d pressed in state %d", btn, ui->current_state);
    
    switch (ui->current_state)
    {
    case UI_STATE_HOME:
        if (btn == BUTTON_SELECT) {
            ui_switch(ui, UI_STATE_MENU);
        }
        break;
        
    case UI_STATE_MENU:
        if (btn == BUTTON_UP) {
            if (ui->selected_index > 0) {
                ui->selected_index--;
                ESP_LOGI("UI_MANAGER", "Menu up: index = %d", ui->selected_index);
            }
        }
        else if (btn == BUTTON_DOWN) {
            if (ui->selected_index < ui->menu_item_count - 1) {
                ui->selected_index++;
                ESP_LOGI("UI_MANAGER", "Menu down: index = %d", ui->selected_index);
            }
        }
        else if (btn == BUTTON_SELECT) {
            if (ui->selected_index >= 0 && ui->selected_index < ui->menu_item_count) {
                ESP_LOGI("UI_MANAGER", "Menu select: %s", ui->menu_items[ui->selected_index].name);
                ui_switch(ui, ui->menu_items[ui->selected_index].state);
            }
        }
        else if (btn == BUTTON_BACK) {
            ESP_LOGI("UI_MANAGER", "Back to home from menu");
            ui_switch(ui, UI_STATE_HOME);
        }
        break;

    case UI_STATE_TEMP_IDLE:
    case UI_STATE_TEMP_RESULT:
        if (btn == BUTTON_SELECT) {
            temperature_init();
            ui->scan_start_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            ui->scan_done = false;
            ui_switch(ui, UI_STATE_TEMP_SCANNING);
            ESP_LOGI(TAG, "Start temperature scanning");
        }
        else if (btn == BUTTON_BACK) {
            ESP_LOGI("UI_MANAGER", "Back to menu from temp");
            ui_switch(ui, UI_STATE_MENU);
        }
        break;

    case UI_STATE_TEMP_SCANNING:
        if (btn == BUTTON_BACK) {
            ESP_LOGI("UI_MANAGER", "Back to menu from temp scanning");
            ui_switch(ui, UI_STATE_MENU);
        }
        break;

    case UI_STATE_GPS:
    case UI_STATE_HR:
        if (btn == BUTTON_BACK) {
            ESP_LOGI("UI_MANAGER", "Back to menu from %s", 
                    (ui->current_state == UI_STATE_GPS) ? "GPS" : "HR");
            ui_switch(ui, UI_STATE_MENU);
        }
        break;
        
    default:
        ESP_LOGW("UI_MANAGER", "Unhandled button %d in state %d", btn, ui->current_state);
        break;
    }
}

void ui_switch(ui_manager_t *ui, ui_state_t new_state)
{
    // Safety check for ui pointer
    if (ui == NULL) {
        ESP_LOGE("UI_MANAGER", "UI manager is NULL");
        return;
    }
        
    // Stop any ongoing animations first
    lv_anim_del_all();
    
    ui->current_state = new_state;
    lv_obj_t *target = NULL;
    
    switch (new_state) {
        case UI_STATE_HOME:   
            target = ui->scr_home; 
            break;
        case UI_STATE_MENU:   
            target = ui->scr_menu; 
            ui->selected_index = 0; // Reset menu selection
            break;
        case UI_STATE_TEMP_IDLE:
        case UI_STATE_TEMP_SCANNING:
        case UI_STATE_TEMP_RESULT: 
            target = ui->scr_temp; 
            break;
        case UI_STATE_HR:     
            target = ui->scr_hr; 
            break;
        case UI_STATE_GPS:    
            target = ui->scr_gps; 
            break;
        default:
            ESP_LOGE("UI_MANAGER", "Unknown UI state: %d", new_state);
            target = ui->scr_home;
            ui->current_state = UI_STATE_HOME;
            break;
    }
    
    // Enhanced safety checks
    if (target == NULL) {
        ESP_LOGE("UI_MANAGER", "Target screen is NULL for state %d", new_state);
        // Force fallback to home
        target = ui->scr_home;
        ui->current_state = UI_STATE_HOME;
    }
    
    if (target != NULL && lv_obj_is_valid(target)) {
        // Use simple load instead of animation to avoid race conditions
        lv_scr_load(target);
        ESP_LOGI("UI_MANAGER", "Switched to state %d", new_state);
    } else {
        ESP_LOGE("UI_MANAGER", "Failed to switch to state %d - invalid target", new_state);
        // Last resort: try to load home screen without animation
        if (ui->scr_home != NULL) {
            lv_scr_load(ui->scr_home);
            ui->current_state = UI_STATE_HOME;
        }
    }
}


void ui_set_date(ui_manager_t *ui, const char *date)
{
    lv_label_set_text(ui->lbl_date, date);
}

void ui_set_battery(ui_manager_t *ui, int p)
{
    lv_label_set_text_fmt(ui->lbl_battery, LV_SYMBOL_BATTERY_FULL " %d%%", p);
}

void ui_update_temp(ui_manager_t *ui, float t)
{
    if (ui->current_state < UI_STATE_TEMP_IDLE || ui->current_state > UI_STATE_TEMP_RESULT) return;
    
    if (ui->current_state == UI_STATE_TEMP_SCANNING) {
        lv_label_set_text(ui->lbl_temp, "Measuring...\nPlease wait 10 seconds");
    } else if (t < -273.0f) {
        lv_label_set_text(ui->lbl_temp, "Sensor error\nPress SELECT to rescan");
    } else {
        lv_label_set_text_fmt(ui->lbl_temp, "Done! Temp: %.2f C\nPress SELECT to rescan", t);
    }
}

void ui_update_hr(ui_manager_t *ui, int hr, int spo2)
{
    if (hr > 0 && spo2 > 0) {
        lv_label_set_text_fmt(ui->lbl_hr,   "HR: %d bpm", hr);
        lv_label_set_text_fmt(ui->lbl_spo2, "SpO2: %d%%", spo2);
    } else {
        lv_label_set_text(ui->lbl_hr, "Scanning...");
        lv_label_set_text(ui->lbl_spo2, "");
    }
}

void ui_update_gps(ui_manager_t *ui, float lat, float lon, bool valid)
{
    if (valid) {
        lv_label_set_text_fmt(ui->lbl_gps, "Lat: %.6f\nLon: %.6f", lat, lon);
    } else {
        lv_label_set_text(ui->lbl_gps, "No signal");
    }
}


/*                                                                          SSD1306 UI Manager
#include "ui_manager.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "u8g2_esp32_hal.h"
#include "button.h"
#include "gps_tracker.h"
#include "health_tracker.h"
#include "temperature_task.h"
#include "display_task.h"

static const char *TAG = "UI_MANAGER";

static bool heart_visible = true;
static TickType_t last_heart_toggle = 0;
static const TickType_t toggle_interval = pdMS_TO_TICKS(500);

void ui_manager_init(ui_manager_t *ui, u8g2_t *u8g2)
{
    ui->u8g2 = u8g2;
    ui->current_state = UI_STATE_HOME;
    ui->selected_index = 0;
    ui->menu_item_count = 3;
    memset(ui->info.date, 0, sizeof(ui->info.date));
    ui->info.battery = 100;
    ui->menu_items[0] = (menu_item_t){"Foot Tracking", UI_STATE_GPS};
    ui->menu_items[1] = (menu_item_t){"Body Temperature", UI_STATE_TEMP_IDLE};
    ui->menu_items[2] = (menu_item_t){"Heart Rate + SpO2", UI_STATE_HR};
    ESP_LOGI(TAG, "UI Manager initialized");
}

void ui_manager_handle_button(ui_manager_t *ui, button_id_t btn)
{
    switch (ui->current_state)
    {
    case UI_STATE_HOME:
        if (btn == BUTTON_SELECT)
            ui->current_state = UI_STATE_MENU;
        break;
    case UI_STATE_MENU:
        if (btn == BUTTON_BACK)
        {
            ui->current_state = UI_STATE_HOME;
        }
        else if (btn == BUTTON_UP && ui->selected_index > 0)
        {
            ui->selected_index--;
        }
        else if (btn == BUTTON_DOWN && ui->selected_index < ui->menu_item_count - 1)
        {
            ui->selected_index++;
        }
        else if (btn == BUTTON_SELECT)
        {
            ui_state_t new_state = ui->menu_items[ui->selected_index].state;
            if (new_state == UI_STATE_TEMP_IDLE)
            {
                temperature_init();
                ui->scan_start_time_ms = 0;
                ui->scan_done = false;
            }
            ui->current_state = new_state;
        }
        break;

    case UI_STATE_TEMP_IDLE:
    case UI_STATE_TEMP_RESULT:
        if (btn == BUTTON_SELECT)
        {
            temperature_init();
            ui->scan_start_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            ui->scan_done = false;
            ui->current_state = UI_STATE_TEMP_SCANNING;
            ESP_LOGI(TAG, "Start temperature scanning");
        }
        if (btn == BUTTON_BACK)
        {
            ui->current_state = UI_STATE_MENU;
        }
        break;

    case UI_STATE_TEMP_SCANNING:
        if (btn == BUTTON_BACK)
        {
            ui->current_state = UI_STATE_MENU;
        }
        break;

    case UI_STATE_GPS:
    case UI_STATE_HR:
        if (btn == BUTTON_BACK)
        {
            ui->current_state = UI_STATE_MENU;
        }
        break;
    }
}

void ui_manager_update_display(ui_manager_t *ui)
{
    u8g2_ClearBuffer(ui->u8g2);
    u8g2_SetFont(ui->u8g2, u8g2_font_ncenB08_tr);
    char buf[50];

    if (strlen(ui->info.date) > 0)
    {
        u8g2_DrawStr(ui->u8g2, 0, 10, ui->info.date);
    }
    else
    {
        u8g2_DrawStr(ui->u8g2, 0, 10, "2026-01-01");
    }

    int battery = ui->info.battery;
    u8g2_DrawFrame(ui->u8g2, 100, 2, 16, 8);
    u8g2_DrawBox(ui->u8g2, 116, 4, 2, 4);
    int bars = (battery + 24) / 25;
    for (int i = 0; i < bars && i < 4; i++)
    {
        u8g2_DrawBox(ui->u8g2, 102 + i * 3, 4, 2, 4);
    }

    switch (ui->current_state)
    {
    case UI_STATE_HOME:
    {
        u8g2_SetFont(ui->u8g2, u8g2_font_ncenB10_tr);
        u8g2_DrawStr(ui->u8g2, 10, 40, "Fitness Watch");
        break;
    }

    case UI_STATE_MENU:
        for (int i = 0; i < ui->menu_item_count; i++)
        {
            int y_pos = (i + 1) * 15 + 15;
            if (i == ui->selected_index)
            {
                u8g2_SetDrawColor(ui->u8g2, 1);
                u8g2_DrawBox(ui->u8g2, 0, y_pos - 12, 128, 15);
                u8g2_SetDrawColor(ui->u8g2, 0);
                u8g2_DrawStr(ui->u8g2, 10, y_pos, ui->menu_items[i].name);
                u8g2_SetDrawColor(ui->u8g2, 1);
            }
            else
            {
                u8g2_SetDrawColor(ui->u8g2, 1);
                u8g2_DrawStr(ui->u8g2, 10, y_pos, ui->menu_items[i].name);
            }
        }
        break;

    case UI_STATE_GPS:
    {
        gps_data_t gps;
        gps_get_data(&gps);
        u8g2_DrawStr(ui->u8g2, 0, 30, "FOOT TRACKING");
        if (gps.valid)
        {
            snprintf(buf, sizeof(buf), "Lat: %.6f", gps.latitude);
            u8g2_DrawStr(ui->u8g2, 0, 45, buf);
            snprintf(buf, sizeof(buf), "Lon: %.6f", gps.longitude);
            u8g2_DrawStr(ui->u8g2, 0, 60, buf);
        }
        else
        {
            u8g2_DrawStr(ui->u8g2, 0, 50, "No signal");
        }
        break;
    }

    case UI_STATE_TEMP_IDLE:
        u8g2_DrawStr(ui->u8g2, 0, 30, "BODY TEMPERATURE");
        u8g2_DrawStr(ui->u8g2, 0, 50, "Press select to scan");
        break;

    case UI_STATE_TEMP_SCANNING:
        u8g2_DrawStr(ui->u8g2, 0, 30, "BODY TEMPERATURE");
        u8g2_DrawStr(ui->u8g2, 0, 45, "Measuring...");
        u8g2_DrawStr(ui->u8g2, 0, 60, "Please wait 10 seconds");
        break;

    case UI_STATE_TEMP_RESULT:
    {
        float t = temperature_get_data();
        u8g2_DrawStr(ui->u8g2, 0, 30, "BODY TEMPERATURE");
        if (t > -273.15f)
        {
            snprintf(buf, sizeof(buf), "Done! Temp: %.2f C", t);
            u8g2_DrawStr(ui->u8g2, 0, 45, buf);
            u8g2_DrawStr(ui->u8g2, 0, 60, "Press select to rescan");
        }
        else
        {
            u8g2_DrawStr(ui->u8g2, 0, 45, "Sensor error");
            u8g2_DrawStr(ui->u8g2, 0, 60, "Press select to rescan");
        }
        break;
    }

    case UI_STATE_HR:
    {
        health_data_t hd;
        health_get_data(&hd);
        u8g2_DrawStr(ui->u8g2, 0, 30, "HEART RATE + SPO2");
        if (hd.heart_rate > 0 && hd.spo2 > 0)
        {
            snprintf(buf, sizeof(buf), "HR: %d bpm", hd.heart_rate);
            u8g2_DrawStr(ui->u8g2, 0, 45, buf);
            snprintf(buf, sizeof(buf), "SpO2: %d%%", hd.spo2);
            u8g2_DrawStr(ui->u8g2, 0, 60, buf);
            if (heart_visible)
            {
                int x = 85;
                int y = 40;
                u8g2_DrawDisc(ui->u8g2, x + 5, y + 5, 5, U8G2_DRAW_ALL);
                u8g2_DrawDisc(ui->u8g2, x + 15, y + 5, 5, U8G2_DRAW_ALL);
                u8g2_DrawTriangle(ui->u8g2, x, y + 7, x + 20, y + 7, x + 10, y + 20);
            }
        }
        else
        {
            u8g2_DrawStr(ui->u8g2, 0, 50, "Scanning ...");
        }
        if (xTaskGetTickCount() - last_heart_toggle > toggle_interval)
        {
            heart_visible = !heart_visible;
            last_heart_toggle = xTaskGetTickCount();
        }

        break;
    }

    default:
        u8g2_DrawStr(ui->u8g2, 0, 30, "Unknown state");
        break;
    }

    u8g2_SendBuffer(ui->u8g2);
}
*/