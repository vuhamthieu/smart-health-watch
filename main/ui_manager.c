#include "ui_manager.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "button.h"
#include "gps_tracker.h"
#include "health_tracker.h"
#include <math.h>
#include "temperature_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "The_Artists_Garden_at_Eragny.c"
#include "gps_icon.c"
#include "temp_icon.c"
#include "heart_icon.c"
#include "data_icon.c"
#include "notify_icon.c"
#include "wifi.h"

static const char *TAG = "UI_MANAGER";

static TickType_t last_button_time = 0;

#define BUTTON_COOLDOWN_MS 200

static const void *icons[] = {NOTIFY_ICON, GPS_ICON, TEMP_ICON, HEART_ICON, DATA_ICON, LV_SYMBOL_WIFI, LV_SYMBOL_BLUETOOTH};

static const char *labels[] = {"Notifications", "GPS", "Body Temp", "Heart rate", "Dashboard", "Wifi", "Bluetooth"};

void ui_menu_update_selection(ui_manager_t *ui)
{
    if (!ui->list_menu)
        return;

    uint16_t cnt = lv_obj_get_child_cnt(ui->list_menu);
    for (uint16_t i = 0; i < cnt; i++)
    {
        lv_obj_t *btn = lv_obj_get_child(ui->list_menu, i);
        lv_obj_set_style_bg_color(btn, COLOR_BG_NORMAL, LV_PART_MAIN);
        lv_obj_clear_state(btn, LV_STATE_CHECKED);
    }

    lv_obj_t *sel = lv_obj_get_child(ui->list_menu, ui->selected_index);
    if (sel)
    {
        lv_obj_set_style_bg_color(sel, COLOR_BG_SELECTED, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(sel, 12, LV_PART_MAIN);
        lv_obj_add_state(sel, LV_STATE_CHECKED);
    }

    lv_obj_scroll_to_view(sel, LV_ANIM_ON);
}

void ui_create_menu(ui_manager_t *ui)
{
    ui->scr_menu = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_menu, lv_color_hex(0x000000), LV_PART_MAIN);

    ui->list_menu = lv_list_create(ui->scr_menu);
    lv_obj_set_size(ui->list_menu, 160, 160);
    lv_obj_align(ui->list_menu, LV_ALIGN_CENTER, 0, 0);
    lv_obj_get_style_pad_row(ui->list_menu, 20);

    for (int i = 0; i < ui->menu_item_count; i++)
    {
        lv_list_add_btn(ui->list_menu, icons[i], labels[i]);
    }

    ui_menu_update_selection(ui);
}

void ui_update_wifi_status(ui_manager_t *ui)
{
    if (ui->lbl_wifi_status)
    {
        if (is_wifi_connecting())
        {
            lv_label_set_text(ui->lbl_wifi_status, "WIFI: Turning on");
            lv_obj_set_style_text_color(ui->lbl_wifi_status, lv_color_hex(0xFFEB3B), LV_PART_MAIN);
        }
        else
        {
            lv_label_set_text(ui->lbl_wifi_status, is_wifi_connected() ? "WiFi: ON" : "WiFi: OFF");
            lv_obj_set_style_text_color(ui->lbl_wifi_status,
                                        is_wifi_connected() ? lv_color_hex(0x4CAF50) : lv_color_hex(0xFF5722),
                                        LV_PART_MAIN);
        }
    }
}

void ui_update_home_wifi_icon(ui_manager_t *ui)
{
    if (is_wifi_connected())
    {
        if (!ui->lbl_wifi)
        {
            ui->lbl_wifi = lv_label_create(ui->scr_home);
            lv_obj_set_style_text_font(ui->lbl_wifi, &lv_font_montserrat_12, LV_PART_MAIN);
            lv_label_set_text(ui->lbl_wifi, LV_SYMBOL_WIFI);
            lv_obj_set_style_text_color(ui->lbl_wifi, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
            lv_obj_align(ui->lbl_wifi, LV_ALIGN_TOP_RIGHT, -40, 4);
        }
    }
    else
    {
        if (ui->lbl_wifi)
        {
            lv_obj_del(ui->lbl_wifi);
            ui->lbl_wifi = NULL;
        }
    }
}

void ui_manager_init(ui_manager_t *ui)
{
    memset(ui, 0, sizeof(*ui));
    ui->menu_item_count = 7;
    ui->selected_index = 0;
    // Menu items
    ui->menu_items[0] = (menu_item_t){"Notifications", UI_STATE_NOTIFY};
    ui->menu_items[1] = (menu_item_t){"GPS Tracking", UI_STATE_GPS};
    ui->menu_items[2] = (menu_item_t){"Temperature", UI_STATE_TEMP_IDLE};
    ui->menu_items[3] = (menu_item_t){"Heart Rate", UI_STATE_HR};
    ui->menu_items[4] = (menu_item_t){"Dashboard", UI_STATE_DATA};
    ui->menu_items[5] = (menu_item_t){"Wifi", UI_STATE_WIFI};
    ui->menu_items[6] = (menu_item_t){"Bluetooth", UI_STATE_BLUETOOTH};

    // Set dark theme globally
    lv_theme_t *theme = lv_theme_default_init(lv_disp_get_default(),
                                              lv_color_white(),       // Primary color
                                              lv_color_hex(0x333333), // Secondary color
                                              true,                   // Dark mode
                                              LV_FONT_DEFAULT);
    lv_disp_set_theme(lv_disp_get_default(), theme);

    /* ---------- HOME SCREEN - Dark Style ---------- */
    ui->scr_home = lv_obj_create(NULL);

    lv_obj_t *img_bg = lv_img_create(ui->scr_home);
    lv_img_set_src(img_bg, &The_Artists_Garden_at_Eragny);
    lv_obj_align(img_bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(img_bg, 128, 160);
    lv_obj_clear_flag(img_bg, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *lbl_datetime = lv_label_create(ui->scr_home);
    lv_label_set_text(lbl_datetime, "11:11");
    lv_obj_set_style_text_font(lbl_datetime, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl_datetime, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_datetime, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(lbl_datetime, LV_ALIGN_CENTER, 0, 0);

    ui->lbl_battery = lv_label_create(ui->scr_home);
    lv_obj_set_style_text_font(ui->lbl_battery, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_label_set_text(ui->lbl_battery, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(ui->lbl_battery, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(ui->lbl_battery, LV_ALIGN_TOP_RIGHT, -4, 4);

    /* ---------- MENU - Modern Dark Style ---------- */
    ui_create_menu(ui);

    /* ---------- Notifications ---------- */
    ui->scr_notify = lv_obj_create(NULL);

    /* ---------- Dashboard ----------*/
    ui->scr_data = lv_obj_create(NULL);

    /* ---------- TEMPERATURE - Professional Dark ---------- */
    ui->scr_temp = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_temp, lv_color_black(), LV_PART_MAIN);

    // Title with icon and color
    lv_obj_t *lbl_temp_title = lv_label_create(ui->scr_temp);
    lv_label_set_text(lbl_temp_title, "TEMPERATURE");
    lv_obj_set_style_text_color(lbl_temp_title, lv_color_hex(0xFF6B35), LV_PART_MAIN);
    lv_obj_align(lbl_temp_title, LV_ALIGN_TOP_MID, 0, 10);

    // Content area with border
    lv_obj_t *temp_container = lv_obj_create(ui->scr_temp);
    lv_obj_set_size(temp_container, LV_PCT(85), 80);
    lv_obj_set_style_bg_color(temp_container, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_border_color(temp_container, lv_color_hex(0xFF6B35), LV_PART_MAIN);
    lv_obj_set_style_border_width(temp_container, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(temp_container, 10, LV_PART_MAIN);
    lv_obj_center(temp_container);

    ui->lbl_temp = lv_label_create(temp_container);
    lv_label_set_text(ui->lbl_temp, "Press SELECT\nto scan");
    lv_obj_set_style_text_align(ui->lbl_temp, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui->lbl_temp, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(ui->lbl_temp);

    /* ---------- HEART RATE - Modern Dark ---------- */
    ui->scr_hr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_hr, lv_color_black(), LV_PART_MAIN);

    // Title with heart icon
    lv_obj_t *lbl_hr_title = lv_label_create(ui->scr_hr);
    lv_label_set_text(lbl_hr_title, "HEART RATE");
    lv_obj_set_style_text_color(lbl_hr_title, lv_color_hex(0xFF1744), LV_PART_MAIN);
    lv_obj_align(lbl_hr_title, LV_ALIGN_TOP_MID, 0, 10);

    // HR display container
    lv_obj_t *hr_container = lv_obj_create(ui->scr_hr);
    lv_obj_set_size(hr_container, LV_PCT(85), 90);
    lv_obj_set_style_bg_color(hr_container, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_border_color(hr_container, lv_color_hex(0xFF1744), LV_PART_MAIN);
    lv_obj_set_style_border_width(hr_container, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(hr_container, 10, LV_PART_MAIN);
    lv_obj_center(hr_container);

    ui->lbl_hr = lv_label_create(hr_container);
    lv_label_set_text(ui->lbl_hr, "HR: -- bpm");
    lv_obj_set_style_text_color(ui->lbl_hr, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(ui->lbl_hr, LV_ALIGN_CENTER, 0, -15);

    ui->lbl_spo2 = lv_label_create(hr_container);
    lv_label_set_text(ui->lbl_spo2, "SpO2: --%");
    lv_obj_set_style_text_color(ui->lbl_spo2, lv_color_hex(0x00E676), LV_PART_MAIN); // Green
    lv_obj_align(ui->lbl_spo2, LV_ALIGN_CENTER, 0, 5);

    /* ---------- GPS - Tech Style ---------- */
    ui->scr_gps = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_gps, lv_color_black(), LV_PART_MAIN);

    lv_obj_t *lbl_gps_title = lv_label_create(ui->scr_gps);
    lv_label_set_text(lbl_gps_title, LV_SYMBOL_GPS "GPS TRACKING");
    lv_obj_set_style_text_color(lbl_gps_title, lv_color_hex(0x4CAF50), LV_PART_MAIN); // Green
    lv_obj_align(lbl_gps_title, LV_ALIGN_TOP_MID, 0, 10);

    // GPS container
    lv_obj_t *gps_container = lv_obj_create(ui->scr_gps);
    lv_obj_set_size(gps_container, LV_PCT(85), 80);
    lv_obj_set_style_bg_color(gps_container, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_border_color(gps_container, lv_color_hex(0x4CAF50), LV_PART_MAIN);
    lv_obj_set_style_border_width(gps_container, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(gps_container, 10, LV_PART_MAIN);
    lv_obj_center(gps_container);

    ui->lbl_gps = lv_label_create(gps_container);
    lv_label_set_text(ui->lbl_gps, "Searching...");
    lv_obj_set_style_text_align(ui->lbl_gps, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui->lbl_gps, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(ui->lbl_gps);

    /* -------- WIFI --------- */
    ui->scr_wifi = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_wifi, lv_color_black(), LV_PART_MAIN);

    // TITLE
    lv_obj_t *lbl_wifi_title = lv_label_create(ui->scr_wifi);
    lv_label_set_text(lbl_wifi_title, "WIFI SETTINGS");
    lv_obj_set_style_text_color(lbl_wifi_title, lv_color_hex(0x2196F3), LV_PART_MAIN); // Blue
    lv_obj_align(lbl_wifi_title, LV_ALIGN_TOP_MID, 0, 10);

    // Container WiFi status
    lv_obj_t *wifi_container = lv_obj_create(ui->scr_wifi);
    lv_obj_set_size(wifi_container, LV_PCT(85), 80);
    lv_obj_set_style_bg_color(wifi_container, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_border_color(wifi_container, lv_color_hex(0x2196F3), LV_PART_MAIN);
    lv_obj_set_style_border_width(wifi_container, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(wifi_container, 10, LV_PART_MAIN);
    lv_obj_center(wifi_container);

    ui->lbl_wifi_status = lv_label_create(wifi_container);
    lv_label_set_text(ui->lbl_wifi_status, is_wifi_connected() ? "WiFi: ON" : "WiFi: OFF");
    lv_obj_set_style_text_align(ui->lbl_wifi_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui->lbl_wifi_status,
                                is_wifi_connected() ? lv_color_hex(0x4CAF50) : lv_color_hex(0xFF5722),
                                LV_PART_MAIN);
    lv_obj_center(ui->lbl_wifi_status);

    /* -------- BLUETOOTH ---------*/
    ui->scr_bluetooth = lv_obj_create(NULL);

    /* Load home screen */
    ui->current_state = UI_STATE_HOME;
    lv_scr_load(ui->scr_home);
    ESP_LOGI(TAG, "Dark Theme UI Manager initialized");
}

void ui_manager_handle_button(ui_manager_t *ui, button_id_t btn)
{
    // Add comprehensive safety check
    if (ui == NULL)
    {
        ESP_LOGE("UI_MANAGER", "UI manager is NULL in button handler");
        return;
    }

    ESP_LOGI("UI_MANAGER", "Button %d pressed in state %d", btn, ui->current_state);

    TickType_t current_time = xTaskGetTickCount();
    if ((current_time - last_button_time) * portTICK_PERIOD_MS < BUTTON_COOLDOWN_MS) {
        ESP_LOGW(TAG, "Button event ignored due to cooldown");
        return;
    }
    last_button_time = current_time;

    // Add comprehensive safety check
    if (ui == NULL)
    {
        ESP_LOGE(TAG, "UI manager is NULL in button handler");
        return;
    }

    ESP_LOGI(TAG, "Button %d pressed in state %d", btn, ui->current_state);

    switch (ui->current_state)
    {
    case UI_STATE_HOME:
        if (btn == BUTTON_SELECT)
        {
            ui_switch(ui, UI_STATE_MENU);
        }
        break;

    case UI_STATE_WIFI:
        if (btn == BUTTON_SELECT)
        {
            if (is_wifi_connected() || is_wifi_connecting())
            {
                ESP_LOGI("UI_MANAGER", "Turning WiFi OFF");
                esp_err_t ret = wifi_stop();
                if (ret != ESP_OK)
                {
                    ESP_LOGE("UI_MANAGER", "Failed to stop WiFi: %s", esp_err_to_name(ret));
                    lv_label_set_text(ui->lbl_wifi_status, "WiFi: Error");
                    lv_obj_set_style_text_color(ui->lbl_wifi_status, lv_color_hex(0xFF5722), LV_PART_MAIN);
                }
                else
                {
                    ui_update_wifi_status(ui);
                    ui_update_home_wifi_icon(ui);
                }
            }
            else
            {
                ESP_LOGI("UI_MANAGER", "Turning WiFi ON");
                esp_err_t ret = wifi_start();
                if (ret != ESP_OK)
                {
                    ESP_LOGE("UI_MANAGER", "Failed to start WiFi: %s", esp_err_to_name(ret));
                    lv_label_set_text(ui->lbl_wifi_status, "WiFi: Error");
                    lv_obj_set_style_text_color(ui->lbl_wifi_status, lv_color_hex(0xFF5722), LV_PART_MAIN);
                }
                else
                {
                    ui_update_wifi_status(ui);
                    ui_update_home_wifi_icon(ui);
                }
            }
        }
        else if (btn == BUTTON_BACK)
        {
            ESP_LOGI("UI_MANAGER", "Back to menu from WiFi");
            ui_switch(ui, UI_STATE_MENU);
        }
        break;

    case UI_STATE_MENU:
        if (btn == BUTTON_UP)
        {
            if (ui->selected_index > 0)
            {
                ui->selected_index--;
                ui_menu_update_selection(ui);
                ESP_LOGI("UI_MANAGER", "Menu up: index = %d", ui->selected_index);
            }
        }
        else if (btn == BUTTON_DOWN)
        {
            if (ui->selected_index < ui->menu_item_count - 1)
            {
                ui->selected_index++;
                ui_menu_update_selection(ui);
                ESP_LOGI("UI_MANAGER", "Menu down: index = %d", ui->selected_index);
            }
        }
        else if (btn == BUTTON_SELECT)
        {
            if (ui->selected_index >= 0 && ui->selected_index < ui->menu_item_count)
            {
                ESP_LOGI("UI_MANAGER", "Menu select: %s", ui->menu_items[ui->selected_index].name);
                ui_switch(ui, ui->menu_items[ui->selected_index].state);
            }
        }
        else if (btn == BUTTON_BACK)
        {
            ESP_LOGI("UI_MANAGER", "Back to home from menu");
            ui_switch(ui, UI_STATE_HOME);
        }
        break;

    case UI_STATE_TEMP_IDLE:
    case UI_STATE_TEMP_RESULT:
        if (btn == BUTTON_SELECT)
        {
            temperature_init();
            ui->scan_start_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            ui->scan_done = false;
            ui_switch(ui, UI_STATE_TEMP_SCANNING);
            ESP_LOGI(TAG, "Start temperature scanning");
        }
        else if (btn == BUTTON_BACK)
        {
            ESP_LOGI("UI_MANAGER", "Back to menu from temp");
            ui_switch(ui, UI_STATE_MENU);
        }
        break;

    case UI_STATE_TEMP_SCANNING:
        if (btn == BUTTON_BACK)
        {
            ESP_LOGI("UI_MANAGER", "Back to menu from temp scanning");
            ui_switch(ui, UI_STATE_MENU);
        }
        break;

    case UI_STATE_GPS:
    case UI_STATE_HR:
        if (btn == BUTTON_BACK)
        {
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
    if (ui == NULL)
    {
        ESP_LOGE("UI_MANAGER", "UI manager is NULL");
        return;
    }

    // Stop any ongoing animations first
    lv_anim_del_all();

    ui->current_state = new_state;
    lv_obj_t *target = NULL;

    switch (new_state)
    {
    case UI_STATE_HOME:
        target = ui->scr_home;
        break;

    case UI_STATE_MENU:
        target = ui->scr_menu;
        ui->selected_index = 0; // Reset menu selection
        ui_menu_update_selection(ui);
        break;
    case UI_STATE_WIFI:
        target = ui->scr_wifi;
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
    if (target == NULL)
    {
        ESP_LOGE("UI_MANAGER", "Target screen is NULL for state %d", new_state);
        // Force fallback to home
        target = ui->scr_home;
        ui->current_state = UI_STATE_HOME;
    }

    if (target != NULL && lv_obj_is_valid(target))
    {
        // Use simple load instead of animation to avoid race conditions
        lv_scr_load(target);
        ESP_LOGI("UI_MANAGER", "Switched to state %d", new_state);
    }
    else
    {

        ESP_LOGE("UI_MANAGER", "Failed to switch to state %d - invalid target", new_state);
        // Last resort: try to load home screen without animation
        if (ui->scr_home != NULL)
        {
            lv_obj_clean(lv_disp_get_scr_act(NULL));
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
    if (ui->current_state < UI_STATE_TEMP_IDLE || ui->current_state > UI_STATE_TEMP_RESULT)
        return;

    if (ui->current_state == UI_STATE_TEMP_SCANNING)
    {
        lv_label_set_text(ui->lbl_temp, "Measuring...\nPlease wait 10s");
        lv_obj_set_style_text_color(ui->lbl_temp, lv_color_hex(0xFFEB3B), LV_PART_MAIN); // Yellow
    }
    else if (t < -273.0f || isnan(t))
    {
        lv_label_set_text(ui->lbl_temp, "Sensor Error\nPress SELECT");
        lv_obj_set_style_text_color(ui->lbl_temp, lv_color_hex(0xFF5722), LV_PART_MAIN); // Red
    }
    else
    {
        char temp_str[60];
        snprintf(temp_str, sizeof(temp_str), "🌡️ %.1f°C\n\nPress SELECT\nto rescan", t);
        lv_label_set_text(ui->lbl_temp, temp_str);

        // Color based on temperature
        if (t > 37.5)
        {
            lv_obj_set_style_text_color(ui->lbl_temp, lv_color_hex(0xFF5722), LV_PART_MAIN); // Red - fever
        }
        else if (t > 36.0)
        {
            lv_obj_set_style_text_color(ui->lbl_temp, lv_color_hex(0x4CAF50), LV_PART_MAIN); // Green - normal
        }
        else
        {
            lv_obj_set_style_text_color(ui->lbl_temp, lv_color_hex(0x2196F3), LV_PART_MAIN); // Blue - low
        }
    }
}

void ui_update_hr(ui_manager_t *ui, int hr, int spo2)
{
    if (hr > 0 && spo2 > 0)
    {
        lv_label_set_text_fmt(ui->lbl_hr, "%d BPM", hr);
        lv_label_set_text_fmt(ui->lbl_spo2, "%d%% SpO2", spo2);

        // Color coding for HR
        if (hr > 100)
        {
            lv_obj_set_style_text_color(ui->lbl_hr, lv_color_hex(0xFF5722), LV_PART_MAIN); // Red - high
        }
        else if (hr >= 60)
        {
            lv_obj_set_style_text_color(ui->lbl_hr, lv_color_hex(0x4CAF50), LV_PART_MAIN); // Green - normal
        }
        else
        {
            lv_obj_set_style_text_color(ui->lbl_hr, lv_color_hex(0xFFC107), LV_PART_MAIN); // Yellow - low
        }
    }
    else
    {
        lv_label_set_text(ui->lbl_hr, LV_SYMBOL_LOOP "Scanning...");
        lv_label_set_text(ui->lbl_spo2, "Please wait");
        lv_obj_set_style_text_color(ui->lbl_hr, lv_color_hex(0xFFEB3B), LV_PART_MAIN);
    }
}

void ui_update_gps(ui_manager_t *ui, float lat, float lon, bool valid)
{
    if (valid)
    {
        lv_label_set_text_fmt(ui->lbl_gps, "Lat: %.4f\nLon: %.4f", lat, lon);
    }
    else
    {
        lv_label_set_text(ui->lbl_gps, "No GPS signal");
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