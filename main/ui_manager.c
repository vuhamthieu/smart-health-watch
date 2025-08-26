#include "ui_manager.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "button.h"
#include "health_tracker.h"
#include <math.h>
#include "temperature_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gps_icon.c"
#include "temp_icon.c"
#include "heart_icon.c"
#include "data_icon.c"
#include "notify_icon.c"
#include "wifi.h"
#include "bluetooth.h"

static const char *TAG = "UI_MANAGER";

static TickType_t last_button_time = 0;

#define BUTTON_COOLDOWN_MS 200

// Thêm biểu tượng ">" vào cuối mỗi menu item
static const char *labels[] = {"Notifications", "Body Temp", "Heart Rate", "Dashboard", "Settings"};

static const char *settings_labels[] = {"WiFi", "Bluetooth"};

void ui_menu_update_selection(ui_manager_t *ui, lv_obj_t *list, int selected_index)
{
    if (!list)
        return;

    uint16_t cnt = lv_obj_get_child_cnt(list);
    for (uint16_t i = 0; i < cnt; i++)
    {
        lv_obj_t *item = lv_obj_get_child(list, i);

        if (i == selected_index)
        {
            lv_obj_set_style_border_color(item, lv_color_hex(0xBBF527), LV_PART_MAIN);
            lv_obj_set_style_border_width(item, 1, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_text_color(item, lv_color_white(), LV_PART_MAIN);
        }
        else
        {
            lv_obj_set_style_border_width(item, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_text_color(item, lv_color_white(), LV_PART_MAIN);
        }
    }
}

void ui_create_menu(ui_manager_t *ui)
{
    ui->scr_menu = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_menu, lv_color_black(), LV_PART_MAIN);

    ui->list_menu = lv_obj_create(ui->scr_menu);
    lv_obj_set_size(ui->list_menu, 128, 160);
    lv_obj_set_pos(ui->list_menu, 0, 0);
    lv_obj_set_style_bg_opa(ui->list_menu, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui->list_menu, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui->list_menu, 8, LV_PART_MAIN);

    lv_obj_clear_flag(ui->list_menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->list_menu, LV_DIR_NONE);

    lv_obj_set_flex_flow(ui->list_menu, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui->list_menu, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    int item_height = 20;

    static lv_style_t style_menu_item;
    lv_style_init(&style_menu_item);
    lv_style_set_radius(&style_menu_item, 2);
    lv_style_set_border_width(&style_menu_item, 0);
    lv_style_set_pad_left(&style_menu_item, 6);
    lv_style_set_pad_right(&style_menu_item, 6);
    lv_style_set_pad_top(&style_menu_item, 3);
    lv_style_set_pad_bottom(&style_menu_item, 3);

    for (int i = 0; i < ui->menu_item_count; i++)
    {
        // Create container for each menu item
        lv_obj_t *item_container = lv_obj_create(ui->list_menu);
        lv_obj_set_size(item_container, LV_PCT(95), item_height);
        lv_obj_add_style(item_container, &style_menu_item, 0);
        lv_obj_set_style_bg_opa(item_container, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(item_container, 0, LV_PART_MAIN);

        // Use flex to position text and arrow
        lv_obj_set_flex_flow(item_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(item_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Create label for menu text
        lv_obj_t *item_label = lv_label_create(item_container);
        lv_obj_set_style_text_font(item_label, &lv_font_montserrat_10, LV_PART_MAIN);
        lv_label_set_text(item_label, labels[i]);
        lv_obj_set_style_text_color(item_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_align(item_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);

        // Create label for arrow at the end
        lv_obj_t *arrow_label = lv_label_create(item_container);
        lv_obj_set_style_text_font(arrow_label, &lv_font_montserrat_10, LV_PART_MAIN);
        lv_label_set_text(arrow_label, ">");
        lv_obj_set_style_text_color(arrow_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_align(arrow_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    }

    ui_menu_update_selection(ui, ui->list_menu, ui->selected_index);
}

void ui_create_settings_menu(ui_manager_t *ui)
{
    ui->scr_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_settings, lv_color_black(), LV_PART_MAIN);

    lv_obj_t *lbl_settings_title = lv_label_create(ui->scr_settings);
    lv_label_set_text(lbl_settings_title, "Settings");
    // lv_obj_set_style_text_color(lbl_settings_title, lv_color_hex(0x2196F3), LV_PART_MAIN);
    lv_obj_align(lbl_settings_title, LV_ALIGN_TOP_MID, 0, 10);

    ui->list_settings = lv_obj_create(ui->scr_settings);
    lv_obj_set_size(ui->list_settings, 128, 130);
    lv_obj_set_pos(ui->list_settings, 0, 30);
    lv_obj_set_style_bg_opa(ui->list_settings, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui->list_settings, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui->list_settings, 8, LV_PART_MAIN);

    lv_obj_clear_flag(ui->list_settings, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->list_settings, LV_DIR_NONE);

    lv_obj_set_flex_flow(ui->list_settings, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui->list_settings, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    int item_height = 20;

    static lv_style_t style_menu_item;
    lv_style_init(&style_menu_item);
    lv_style_set_radius(&style_menu_item, 2);
    lv_style_set_border_width(&style_menu_item, 0);
    lv_style_set_pad_left(&style_menu_item, 6);
    lv_style_set_pad_right(&style_menu_item, 6);
    lv_style_set_pad_top(&style_menu_item, 3);
    lv_style_set_pad_bottom(&style_menu_item, 3);

    for (int i = 0; i < 2; i++)
    {
        // Create container for each settings menu item
        lv_obj_t *item_container = lv_obj_create(ui->list_settings);
        lv_obj_set_size(item_container, LV_PCT(95), item_height);
        lv_obj_add_style(item_container, &style_menu_item, 0);
        lv_obj_set_style_bg_opa(item_container, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(item_container, 0, LV_PART_MAIN);

        // Use flex to position text and arrow
        lv_obj_set_flex_flow(item_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(item_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Create label for menu text
        lv_obj_t *item_label = lv_label_create(item_container);
        lv_obj_set_style_text_font(item_label, &lv_font_montserrat_10, LV_PART_MAIN);
        lv_label_set_text(item_label, settings_labels[i]);
        lv_obj_set_style_text_color(item_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_align(item_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);

        // Create label for arrow at the end
        lv_obj_t *arrow_label = lv_label_create(item_container);
        lv_obj_set_style_text_font(arrow_label, &lv_font_montserrat_10, LV_PART_MAIN);
        lv_label_set_text(arrow_label, ">");
        lv_obj_set_style_text_color(arrow_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_align(arrow_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    }

    ui_menu_update_selection(ui, ui->list_settings, ui->settings_selected_index);
}

void ui_update_wifi_status(ui_manager_t *ui)
{
    if (ui->lbl_wifi_status)
    {
        if (is_wifi_connecting())
        {
            lv_label_set_text(ui->lbl_wifi_status, "WiFi: Connecting...");
            lv_obj_set_style_text_color(ui->lbl_wifi_status, lv_color_hex(0xFFEB3B), LV_PART_MAIN);
        }
        else if (is_wifi_connected())
        {
            lv_label_set_text(ui->lbl_wifi_status, "WiFi: Connected");
            lv_obj_set_style_text_color(ui->lbl_wifi_status, lv_color_hex(0x4CAF50), LV_PART_MAIN);
        }
        else if (!is_wifi_connected() && !is_wifi_connecting() && !is_wifi_connect_failed())
        {
            lv_label_set_text(ui->lbl_wifi_status, "WiFi: OFF");
            lv_obj_set_style_text_color(ui->lbl_wifi_status, lv_color_hex(0xFF5722), LV_PART_MAIN);
        }
        else if (is_wifi_connect_failed())
        {
            lv_label_set_text(ui->lbl_wifi_status, "Failed to connect");
            lv_obj_set_style_text_color(ui->lbl_wifi_status, lv_color_hex(0xFF5722), LV_PART_MAIN);
        }
        else
        {
            lv_label_set_text(ui->lbl_wifi_status, "WiFi: OFF");
            lv_obj_set_style_text_color(ui->lbl_wifi_status, lv_color_hex(0xFF5722), LV_PART_MAIN);
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

void ui_update_home_bluetooth_icon(ui_manager_t *ui)
{
    if (bluetooth_is_connected())
    {
        if (!ui->lbl_bluetooth)
        {
            ui->lbl_bluetooth = lv_label_create(ui->scr_home);
            lv_obj_set_style_text_font(ui->lbl_bluetooth, &lv_font_montserrat_12, LV_PART_MAIN);
            lv_label_set_text(ui->lbl_bluetooth, LV_SYMBOL_BLUETOOTH);
            lv_obj_set_style_text_color(ui->lbl_bluetooth, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
            lv_obj_align(ui->lbl_bluetooth, LV_ALIGN_TOP_RIGHT, -30, 4);
            ESP_LOGI("UI_MANAGER", "Bluetooth icon created on home screen");
        }
    }
    else
    {
        if (ui->lbl_bluetooth)
        {
            lv_obj_del(ui->lbl_bluetooth);
            ui->lbl_bluetooth = NULL;
            ESP_LOGI("UI_MANAGER", "Bluetooth icon removed from home screen");
        }
    }
}

void ui_update_bluetooth_status(ui_manager_t *ui)
{
    if (ui->lbl_bluetooth_status)
    {
        if (bluetooth_is_connected())
        {
            lv_label_set_text(ui->lbl_bluetooth_status, "Bluetooth: ON");
            lv_obj_set_style_text_color(ui->lbl_bluetooth_status, lv_color_hex(0x4CAF50), LV_PART_MAIN);
        }
        else if (bluetooth_is_advertising())
        {
            lv_label_set_text(ui->lbl_bluetooth_status, "Bluetooth: Advertising");
            lv_obj_set_style_text_color(ui->lbl_bluetooth_status, lv_color_hex(0xFFEB3B), LV_PART_MAIN);
        }
        else
        {
            lv_label_set_text(ui->lbl_bluetooth_status, "Bluetooth: OFF");
            lv_obj_set_style_text_color(ui->lbl_bluetooth_status, lv_color_hex(0xFF5722), LV_PART_MAIN);
        }
    }
    ui_update_home_bluetooth_icon(ui);
}

void ui_manager_init(ui_manager_t *ui)
{
    memset(ui, 0, sizeof(*ui));
    ui->menu_item_count = 5;
    ui->selected_index = 0;
    ui->settings_selected_index = 0;

    // Menu items
    ui->menu_items[0] = (menu_item_t){"Notifications", UI_STATE_NOTIFY};
    ui->menu_items[1] = (menu_item_t){"Temperature", UI_STATE_TEMP_IDLE};
    ui->menu_items[2] = (menu_item_t){"Heart Rate", UI_STATE_HR};
    ui->menu_items[3] = (menu_item_t){"Dashboard", UI_STATE_DATA};
    ui->menu_items[4] = (menu_item_t){"Settings", UI_STATE_SETTING};

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
    lv_obj_align(img_bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(img_bg, 128, 160);
    lv_obj_clear_flag(img_bg, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *lbl_datetime = lv_label_create(ui->scr_home);
    lv_label_set_text(lbl_datetime, "11:11");
    lv_obj_set_style_text_font(lbl_datetime, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl_datetime, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_datetime, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(lbl_datetime, LV_ALIGN_CENTER, 0, 0);

    ui->lbl_time = lv_label_create(ui->scr_home);
    lv_label_set_text(ui->lbl_time, "17:36");
    lv_obj_set_style_text_font(ui->lbl_time, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui->lbl_time, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(ui->lbl_time, LV_ALIGN_TOP_LEFT, 4, 4);

    ui->lbl_battery_percent = lv_label_create(ui->scr_home);
    lv_obj_set_style_text_font(ui->lbl_battery_percent, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_label_set_text(ui->lbl_battery_percent, "80%");
    lv_obj_set_style_text_color(ui->lbl_battery_percent, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(ui->lbl_battery_percent, LV_ALIGN_TOP_RIGHT, -20, 5);

    ui->lbl_battery_icon = lv_label_create(ui->scr_home);
    lv_obj_set_style_text_font(ui->lbl_battery_icon, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_label_set_text(ui->lbl_battery_icon, LV_SYMBOL_BATTERY_3);
    lv_obj_set_style_text_color(ui->lbl_battery_icon, lv_color_hex(0x41D958), LV_PART_MAIN);
    lv_obj_align(ui->lbl_battery_icon, LV_ALIGN_TOP_RIGHT, -4, 3.5);

    /* ---------- MENU - Modern Dark Style ---------- */
    ui_create_menu(ui);

    /* ---------- Notifications ---------- */
    ui->scr_notify = lv_obj_create(NULL);

    /* ---------- DASHBOARD - Modern Clean Style ----------*/
    ui->scr_data = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_data, lv_color_black(), LV_PART_MAIN);

    lv_obj_t *lbl_data_title = lv_label_create(ui->scr_data);
    lv_label_set_text(lbl_data_title, "Dashboard");
    lv_obj_set_style_text_color(lbl_data_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_data_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(lbl_data_title, LV_ALIGN_TOP_MID, 0, 8);

    // Temperature bar (directly on screen, no container)
    ui->bar_temp = lv_bar_create(ui->scr_data);
    lv_obj_set_size(ui->bar_temp, 100, 8);
    lv_bar_set_range(ui->bar_temp, 0, 50);
    lv_bar_set_value(ui->bar_temp, 0, LV_ANIM_OFF);
    lv_obj_align(ui->bar_temp, LV_ALIGN_TOP_MID, 0, 35);
    static lv_style_t bar_style_temp;
    lv_style_init(&bar_style_temp);
    lv_style_set_bg_color(&bar_style_temp, lv_color_hex(0x2a2a2a));
    lv_style_set_bg_opa(&bar_style_temp, LV_OPA_COVER);
    lv_style_set_border_width(&bar_style_temp, 0);
    lv_style_set_radius(&bar_style_temp, 4);
    lv_obj_add_style(ui->bar_temp, &bar_style_temp, 0);

    // HR bar (left side)
    ui->bar_hr = lv_bar_create(ui->scr_data);
    lv_obj_set_size(ui->bar_hr, 55, 8);
    lv_bar_set_range(ui->bar_hr, 0, 200);
    lv_bar_set_value(ui->bar_hr, 0, LV_ANIM_OFF);
    lv_obj_align(ui->bar_hr, LV_ALIGN_TOP_LEFT, 8, 75);
    static lv_style_t bar_style_hr;
    lv_style_init(&bar_style_hr);
    lv_style_set_bg_color(&bar_style_hr, lv_color_hex(0x2a2a2a));
    lv_style_set_bg_opa(&bar_style_hr, LV_OPA_COVER);
    lv_style_set_border_width(&bar_style_hr, 0);
    lv_style_set_radius(&bar_style_hr, 4);
    lv_obj_add_style(ui->bar_hr, &bar_style_hr, 0);

    // SpO2 bar (right side)
    ui->bar_spo2 = lv_bar_create(ui->scr_data);
    lv_obj_set_size(ui->bar_spo2, 55, 8);
    lv_bar_set_range(ui->bar_spo2, 0, 100);
    lv_bar_set_value(ui->bar_spo2, 0, LV_ANIM_OFF);
    lv_obj_align(ui->bar_spo2, LV_ALIGN_TOP_RIGHT, -8, 75);
    static lv_style_t bar_style_spo2;
    lv_style_init(&bar_style_spo2);
    lv_style_set_bg_color(&bar_style_spo2, lv_color_hex(0x2a2a2a));
    lv_style_set_bg_opa(&bar_style_spo2, LV_OPA_COVER);
    lv_style_set_border_width(&bar_style_spo2, 0);
    lv_style_set_radius(&bar_style_spo2, 4);
    lv_obj_add_style(ui->bar_spo2, &bar_style_spo2, 0);

    // Temperature label (below temp bar)
    ui->lbl_temp_dashboard = lv_label_create(ui->scr_data);
    lv_label_set_text(ui->lbl_temp_dashboard, "Temp: -- °C");
    lv_obj_set_style_text_color(ui->lbl_temp_dashboard, lv_color_hex(0xFF6B35), LV_PART_MAIN);
    lv_obj_align(ui->lbl_temp_dashboard, LV_ALIGN_TOP_MID, 0, 50);

    // HR label (below HR bar, left side)
    ui->lbl_hr_dashboard = lv_label_create(ui->scr_data);
    lv_label_set_text(ui->lbl_hr_dashboard, "HR: --");
    lv_obj_set_style_text_color(ui->lbl_hr_dashboard, lv_color_hex(0xFF1744), LV_PART_MAIN);
    lv_obj_align(ui->lbl_hr_dashboard, LV_ALIGN_TOP_LEFT, 8, 90);

    // SpO2 label (below SpO2 bar, right side - different line)
    ui->lbl_spo2_dashboard = lv_label_create(ui->scr_data);
    lv_label_set_text(ui->lbl_spo2_dashboard, "SpO2: --%");
    lv_obj_set_style_text_color(ui->lbl_spo2_dashboard, lv_color_hex(0x00E676), LV_PART_MAIN);
    lv_obj_align(ui->lbl_spo2_dashboard, LV_ALIGN_TOP_RIGHT, -8, 105);

    /* ---------- TEMPERATURE - Professional Dark ---------- */
    ui->scr_temp = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_temp, lv_color_black(), LV_PART_MAIN);

    // Title
    lv_obj_t *lbl_temp_title = lv_label_create(ui->scr_temp);
    lv_label_set_text(lbl_temp_title, "Temperature");
    lv_obj_set_style_text_color(lbl_temp_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_temp_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(lbl_temp_title, LV_ALIGN_TOP_MID, 0, 10);

    // Content area - simplified
    ui->lbl_temp = lv_label_create(ui->scr_temp);
    lv_label_set_text(ui->lbl_temp, "Press SELECT\nto scan");
    lv_obj_set_style_text_align(ui->lbl_temp, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui->lbl_temp, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->lbl_temp, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(ui->lbl_temp);

    /* ---------- HEART RATE - Clean Black & White ---------- */
    ui->scr_hr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_hr, lv_color_black(), LV_PART_MAIN);

    // Title
    lv_obj_t *lbl_hr_title = lv_label_create(ui->scr_hr);
    lv_label_set_text(lbl_hr_title, "Heart rate");
    lv_obj_set_style_text_color(lbl_hr_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_hr_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(lbl_hr_title, LV_ALIGN_TOP_MID, 0, 10);

    // HR display
    ui->lbl_hr = lv_label_create(ui->scr_hr);
    lv_label_set_text(ui->lbl_hr, "HR: -- bpm");
    lv_obj_set_style_text_color(ui->lbl_hr, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->lbl_hr, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(ui->lbl_hr, LV_ALIGN_CENTER, 0, -15);

    // SpO2 display
    ui->lbl_spo2 = lv_label_create(ui->scr_hr);
    lv_label_set_text(ui->lbl_spo2, "SpO2: --%");
    lv_obj_set_style_text_color(ui->lbl_spo2, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->lbl_spo2, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(ui->lbl_spo2, LV_ALIGN_CENTER, 0, 5);

    /* -------- WIFI - Clean Black & White --------- */
    ui->scr_wifi = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_wifi, lv_color_black(), LV_PART_MAIN);

    // TITLE
    lv_obj_t *lbl_wifi_title = lv_label_create(ui->scr_wifi);
    lv_label_set_text(lbl_wifi_title, "Wifi setting");
    lv_obj_set_style_text_color(lbl_wifi_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_wifi_title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(lbl_wifi_title, LV_ALIGN_TOP_MID, 0, 10);

    // WiFi status
    ui->lbl_wifi_status = lv_label_create(ui->scr_wifi);
    lv_label_set_text(ui->lbl_wifi_status, is_wifi_connected() ? "WiFi: ON" : "WiFi: OFF");
    lv_obj_set_style_text_align(ui->lbl_wifi_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui->lbl_wifi_status, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->lbl_wifi_status, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(ui->lbl_wifi_status);

    // Add instruction text
    lv_obj_t *lbl_wifi_instruction = lv_label_create(ui->scr_wifi);
    lv_label_set_text(lbl_wifi_instruction, "Press SELECT to toggle");
    lv_obj_set_style_text_align(lbl_wifi_instruction, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_wifi_instruction, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_wifi_instruction, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(lbl_wifi_instruction, LV_ALIGN_BOTTOM_MID, 0, -20);

    /* -------- BLUETOOTH - Clean Black & White ---------*/
    ui->scr_bluetooth = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_bluetooth, lv_color_black(), LV_PART_MAIN);

    // Title
    lv_obj_t *lbl_bluetooth_title = lv_label_create(ui->scr_bluetooth);
    lv_label_set_text(lbl_bluetooth_title, "Bluetooth setting");
    lv_obj_set_style_text_color(lbl_bluetooth_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_bluetooth_title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(lbl_bluetooth_title, LV_ALIGN_TOP_MID, 0, 10);

    // Bluetooth status
    ui->lbl_bluetooth_status = lv_label_create(ui->scr_bluetooth);
    lv_label_set_text(ui->lbl_bluetooth_status, bluetooth_is_connected() ? "Bluetooth: ON" : "Bluetooth: OFF");
    lv_obj_set_style_text_align(ui->lbl_bluetooth_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui->lbl_bluetooth_status, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->lbl_bluetooth_status, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(ui->lbl_bluetooth_status);

    // Add instruction text
    lv_obj_t *lbl_bluetooth_instruction = lv_label_create(ui->scr_bluetooth);
    lv_label_set_text(lbl_bluetooth_instruction, "Press SELECT to toggle");
    lv_obj_set_style_text_align(lbl_bluetooth_instruction, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_bluetooth_instruction, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_bluetooth_instruction, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(lbl_bluetooth_instruction, LV_ALIGN_BOTTOM_MID, 0, -20);

    /* -------- SETTINGS MENU --------- */
    ui_create_settings_menu(ui);

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
    if ((current_time - last_button_time) * portTICK_PERIOD_MS < BUTTON_COOLDOWN_MS)
    {
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

    case UI_STATE_SETTING:
        if (btn == BUTTON_UP)
        {
            if (ui->settings_selected_index > 0)
            {
                ui->settings_selected_index--;
                ui_menu_update_selection(ui, ui->list_settings, ui->settings_selected_index);
                ESP_LOGI("UI_MANAGER", "Settings menu up: index = %d", ui->settings_selected_index);
            }
        }
        else if (btn == BUTTON_DOWN)
        {
            if (ui->settings_selected_index < 1)
            {
                ui->settings_selected_index++;
                ui_menu_update_selection(ui, ui->list_settings, ui->settings_selected_index);
                ESP_LOGI("UI_MANAGER", "Settings menu down: index = %d", ui->settings_selected_index);
            }
        }
        else if (btn == BUTTON_SELECT)
        {
            if (ui->settings_selected_index == 0)
            {
                ui_switch(ui, UI_STATE_WIFI);
            }
            else
            {
                ui_switch(ui, UI_STATE_BLUETOOTH);
            }
        }
        else if (btn == BUTTON_BACK)
        {
            ESP_LOGI("UI_MANAGER", "Back to menu from settings menu");
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
                }
            }
            else
            {
                ESP_LOGI("UI_MANAGER", "Turning WiFi ON");
                esp_err_t ret = wifi_start();
                if (ret != ESP_OK)
                {
                    ESP_LOGE("UI_MANAGER", "Failed to start WiFi: %s", esp_err_to_name(ret));
                }
            }
            ui_update_wifi_status(ui);
            ui_update_home_wifi_icon(ui);
        }
        else if (btn == BUTTON_BACK)
        {
            ESP_LOGI("UI_MANAGER", "Back to settings menu from WiFi");
            ui_switch(ui, UI_STATE_SETTING);
        }
        break;

    case UI_STATE_BLUETOOTH:
        if (btn == BUTTON_SELECT)
        {
            if (bluetooth_is_connected())
            {
                ESP_LOGI("UI_MANAGER", "Disconnecting Bluetooth");
                bluetooth_disconnect();
            }
            else if (bluetooth_is_advertising())
            {
                ESP_LOGI("UI_MANAGER", "Stopping Bluetooth advertising");
                bluetooth_stop_advertising();
            }
            else
            {
                ESP_LOGI("UI_MANAGER", "Starting Bluetooth advertising");
                bluetooth_start_advertising();
            }
            vTaskDelay(pdMS_TO_TICKS(500));
            ui_update_bluetooth_status(ui);
        }
        else if (btn == BUTTON_BACK)
        {
            ESP_LOGI("UI_MANAGER", "Back to settings menu from Bluetooth");
            ui_switch(ui, UI_STATE_SETTING);
        }
        break;

    case UI_STATE_MENU:
        if (btn == BUTTON_UP)
        {
            if (ui->selected_index > 0)
            {
                ui->selected_index--;
                ui_menu_update_selection(ui, ui->list_menu, ui->selected_index);
                ESP_LOGI("UI_MANAGER", "Menu up: index = %d", ui->selected_index);
            }
        }
        else if (btn == BUTTON_DOWN)
        {
            if (ui->selected_index < ui->menu_item_count - 1)
            {
                ui->selected_index++;
                ui_menu_update_selection(ui, ui->list_menu, ui->selected_index);
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

    case UI_STATE_HR:
        if (btn == BUTTON_BACK)
        {
            ESP_LOGI("UI_MANAGER", "Back to menu from HR");
            ui_switch(ui, UI_STATE_MENU);
        }
        break;

    case UI_STATE_DATA:
        if (btn == BUTTON_BACK)
        {
            ESP_LOGI("UI_MANAGER", "Back to menu from dashboard");
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
        ui_menu_update_selection(ui, ui->list_menu, ui->selected_index);
        break;

    case UI_STATE_SETTING:
        target = ui->scr_settings;
        ui_menu_update_selection(ui, ui->list_settings, ui->settings_selected_index);
        break;

    case UI_STATE_WIFI:
        target = ui->scr_wifi;
        ui_update_wifi_status(ui);
        break;

    case UI_STATE_BLUETOOTH:
        target = ui->scr_bluetooth;
        ui_update_bluetooth_status(ui);
        break;

    case UI_STATE_TEMP_IDLE:
    case UI_STATE_TEMP_SCANNING:
    case UI_STATE_TEMP_RESULT:
        target = ui->scr_temp;
        break;

    case UI_STATE_HR:
        target = ui->scr_hr;
        break;

    case UI_STATE_DATA:
        target = ui->scr_data;
        ui_update_dashboard(ui);
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
        snprintf(temp_str, sizeof(temp_str), "%.1f°C\n\nPress SELECT\nto rescan", t);
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

void ui_update_dashboard(ui_manager_t *ui)
{
    if (ui->current_state != UI_STATE_DATA)
        return;

    float temperature = temperature_get_data();
    if (temperature > -273.0f && !isnan(temperature))
    {
        lv_bar_set_value(ui->bar_temp, (int)temperature, LV_ANIM_ON);
        char temp_str[20];
        snprintf(temp_str, sizeof(temp_str), "Temp: %.2f °C", temperature);
        lv_label_set_text(ui->lbl_temp_dashboard, temp_str);
        if (temperature > 37.5)
        {
            lv_obj_set_style_text_color(ui->lbl_temp_dashboard, lv_color_hex(0xFF5722), LV_PART_MAIN);
        }
        else if (temperature > 36.0)
        {
            lv_obj_set_style_text_color(ui->lbl_temp_dashboard, lv_color_hex(0x4CAF50), LV_PART_MAIN);
        }
        else
        {
            lv_obj_set_style_text_color(ui->lbl_temp_dashboard, lv_color_hex(0x2196F3), LV_PART_MAIN);
        }
    }
    else
    {
        lv_bar_set_value(ui->bar_temp, 0, LV_ANIM_OFF);
        lv_label_set_text(ui->lbl_temp_dashboard, "Temp: -- °C");
        lv_obj_set_style_text_color(ui->lbl_temp_dashboard, lv_color_hex(0xFFEB3B), LV_PART_MAIN);
    }

    health_data_t hd;
    health_get_data(&hd);
    if (hd.heart_rate > 0 && hd.spo2 > 0)
    {
        lv_bar_set_value(ui->bar_hr, hd.heart_rate, LV_ANIM_ON);
        lv_bar_set_value(ui->bar_spo2, hd.spo2, LV_ANIM_ON);
        lv_label_set_text_fmt(ui->lbl_hr_dashboard, "HR: %d bpm", hd.heart_rate);
        lv_label_set_text_fmt(ui->lbl_spo2_dashboard, "SpO2: %d%%", hd.spo2);
        if (hd.heart_rate > 100)
        {
            lv_obj_set_style_text_color(ui->lbl_hr_dashboard, lv_color_hex(0xFF5722), LV_PART_MAIN);
        }
        else if (hd.heart_rate >= 60)
        {
            lv_obj_set_style_text_color(ui->lbl_hr_dashboard, lv_color_hex(0x4CAF50), LV_PART_MAIN);
        }
        else
        {
            lv_obj_set_style_text_color(ui->lbl_hr_dashboard, lv_color_hex(0xFFC107), LV_PART_MAIN);
        }
    }
    else
    {
        lv_bar_set_value(ui->bar_hr, 0, LV_ANIM_OFF);
        lv_bar_set_value(ui->bar_spo2, 0, LV_ANIM_OFF);
        lv_label_set_text(ui->lbl_hr_dashboard, "HR: -- bpm");
        lv_label_set_text(ui->lbl_spo2_dashboard, "SpO2: --%");
        lv_obj_set_style_text_color(ui->lbl_hr_dashboard, lv_color_hex(0xFFEB3B), LV_PART_MAIN);
    }
}