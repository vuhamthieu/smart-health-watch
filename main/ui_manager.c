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

    ui_create_status_bar(ui->scr_menu, &ui->lbl_time_menu, &ui->lbl_battery_percent_menu, &ui->lbl_battery_icon_menu);

    ui->list_menu = lv_obj_create(ui->scr_menu);
    lv_obj_set_size(ui->list_menu, 128, 135);
    lv_obj_set_pos(ui->list_menu, 0, 25);
    lv_obj_set_style_bg_opa(ui->list_menu, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui->list_menu, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui->list_menu, 8, LV_PART_MAIN);

    lv_obj_clear_flag(ui->list_menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->list_menu, LV_DIR_NONE);

    lv_obj_set_style_border_width(ui->list_menu, 0, LV_PART_SCROLLBAR);

    lv_obj_set_flex_flow(ui->list_menu, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui->list_menu, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    int item_height = 15;

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
        lv_obj_set_size(item_container, LV_PCT(100), item_height);
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

    ui_create_status_bar(ui->scr_settings, &ui->lbl_time_settings, &ui->lbl_battery_percent_settings, &ui->lbl_battery_icon_settings);

    lv_obj_t *lbl_settings_title = lv_label_create(ui->scr_settings);
    lv_label_set_text(lbl_settings_title, "Settings");
    // lv_obj_set_style_text_color(lbl_settings_title, lv_color_hex(0x2196F3), LV_PART_MAIN);
    lv_obj_align(lbl_settings_title, LV_ALIGN_TOP_MID, 0, 25);

    ui->list_settings = lv_obj_create(ui->scr_settings);
    lv_obj_set_size(ui->list_settings, 128, 110);
    lv_obj_set_pos(ui->list_settings, 0, 45);
    lv_obj_set_style_bg_opa(ui->list_settings, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui->list_settings, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui->list_settings, 8, LV_PART_MAIN);

    lv_obj_clear_flag(ui->list_settings, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->list_settings, LV_DIR_NONE);

    lv_obj_set_flex_flow(ui->list_settings, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui->list_settings, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    int item_height = 15;

    static lv_style_t style_menu_item;
    lv_style_init(&style_menu_item);
    lv_style_set_radius(&style_menu_item, 2);
    lv_style_set_border_width(&style_menu_item, 0);
    lv_style_set_pad_left(&style_menu_item, 6);
    lv_style_set_pad_right(&style_menu_item, 6);
    lv_style_set_pad_top(&style_menu_item, 2);
    lv_style_set_pad_bottom(&style_menu_item, 2);

    for (int i = 0; i < 2; i++)
    {

        lv_obj_t *item_container = lv_obj_create(ui->list_settings);
        lv_obj_set_size(item_container, LV_PCT(100), item_height);
        lv_obj_add_style(item_container, &style_menu_item, 0);
        lv_obj_set_style_bg_opa(item_container, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(item_container, 0, LV_PART_MAIN);

        lv_obj_set_flex_flow(item_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(item_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

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

    lv_theme_t *theme = lv_theme_default_init(lv_disp_get_default(),
                                              lv_color_white(),
                                              lv_color_hex(0x333333),
                                              true,
                                              LV_FONT_DEFAULT);
    lv_disp_set_theme(lv_disp_get_default(), theme);

    /* ---------- HOME SCREEN---------- */
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

    /* ---------- MENU---------- */
    ui_create_menu(ui);

    /* ---------- Notifications ---------- */
    ui->scr_notify = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_notify, lv_color_black(), LV_PART_MAIN);
    ui_create_status_bar(ui->scr_notify, &ui->lbl_time_notify, &ui->lbl_battery_percent_notify, &ui->lbl_battery_icon_notify);

    /* ---------- DASHBOARD ----------*/
ui->scr_data = lv_obj_create(NULL);
lv_obj_set_style_bg_color(ui->scr_data, lv_color_black(), LV_PART_MAIN);

ui_create_status_bar(ui->scr_data, &ui->lbl_time_data, &ui->lbl_battery_percent_data, &ui->lbl_battery_icon_data);

// Dashboard title
lv_obj_t *lbl_data_title = lv_label_create(ui->scr_data);
lv_label_set_text(lbl_data_title, "Dashboard");
lv_obj_set_style_text_color(lbl_data_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
lv_obj_set_style_text_font(lbl_data_title, &lv_font_montserrat_14, LV_PART_MAIN);
lv_obj_align(lbl_data_title, LV_ALIGN_TOP_MID, 0, 25);

// Create main container for better organization
lv_obj_t *data_container = lv_obj_create(ui->scr_data);
lv_obj_set_size(data_container, 130, 100);
lv_obj_set_style_bg_opa(data_container, LV_OPA_TRANSP, LV_PART_MAIN);
lv_obj_set_style_border_width(data_container, 0, LV_PART_MAIN);
lv_obj_set_style_pad_all(data_container, 4, LV_PART_MAIN);
lv_obj_align(data_container, LV_ALIGN_CENTER, 0, 10);

// ===== TEMPERATURE SECTION =====
// Temperature label
ui->lbl_temp_dashboard = lv_label_create(data_container);
lv_label_set_text(ui->lbl_temp_dashboard, "Temp");
lv_obj_set_style_text_color(ui->lbl_temp_dashboard, lv_color_hex(0xFF6B35), LV_PART_MAIN);
lv_obj_set_style_text_font(ui->lbl_temp_dashboard, &lv_font_montserrat_10, LV_PART_MAIN);
lv_obj_align(ui->lbl_temp_dashboard, LV_ALIGN_TOP_LEFT, 0, 0);

// Temperature value
lv_obj_t *lbl_temp_value = lv_label_create(data_container);
lv_label_set_text(lbl_temp_value, "-- °C");
lv_obj_set_style_text_color(lbl_temp_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
lv_obj_set_style_text_font(lbl_temp_value, &lv_font_montserrat_12, LV_PART_MAIN);
lv_obj_align(lbl_temp_value, LV_ALIGN_TOP_RIGHT, 0, 0);

// Temperature bar
ui->bar_temp = lv_bar_create(data_container);
lv_obj_set_size(ui->bar_temp, 122, 4);
lv_bar_set_range(ui->bar_temp, 0, 50);
lv_bar_set_value(ui->bar_temp, 0, LV_ANIM_OFF);
lv_obj_align(ui->bar_temp, LV_ALIGN_TOP_MID, 0, 15);

// Temperature bar style
static lv_style_t bar_style_temp;
lv_style_init(&bar_style_temp);
lv_style_set_bg_color(&bar_style_temp, lv_color_hex(0x2a2a2a));
lv_style_set_bg_opa(&bar_style_temp, LV_OPA_COVER);
lv_style_set_border_width(&bar_style_temp, 0);
lv_style_set_radius(&bar_style_temp, 3);
lv_obj_add_style(ui->bar_temp, &bar_style_temp, LV_PART_MAIN);

// Temperature bar indicator style
static lv_style_t bar_indic_temp;
lv_style_init(&bar_indic_temp);
lv_style_set_bg_color(&bar_indic_temp, lv_color_hex(0xFF6B35));
lv_style_set_radius(&bar_indic_temp, 3);
lv_obj_add_style(ui->bar_temp, &bar_indic_temp, LV_PART_INDICATOR);

// ===== HEART RATE SECTION =====
// HR label
ui->lbl_hr_dashboard = lv_label_create(data_container);
lv_label_set_text(ui->lbl_hr_dashboard, "HR");
lv_obj_set_style_text_color(ui->lbl_hr_dashboard, lv_color_hex(0xFF1744), LV_PART_MAIN);
lv_obj_set_style_text_font(ui->lbl_hr_dashboard, &lv_font_montserrat_10, LV_PART_MAIN);
lv_obj_align(ui->lbl_hr_dashboard, LV_ALIGN_TOP_LEFT, 0, 30);

// HR value
lv_obj_t *lbl_hr_value = lv_label_create(data_container);
lv_label_set_text(lbl_hr_value, "--");
lv_obj_set_style_text_color(lbl_hr_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
lv_obj_set_style_text_font(lbl_hr_value, &lv_font_montserrat_12, LV_PART_MAIN);
lv_obj_align(lbl_hr_value, LV_ALIGN_TOP_RIGHT, 0, 30);

// HR bar
ui->bar_hr = lv_bar_create(data_container);
lv_obj_set_size(ui->bar_hr, 122, 4);
lv_bar_set_range(ui->bar_hr, 0, 200);
lv_bar_set_value(ui->bar_hr, 0, LV_ANIM_OFF);
lv_obj_align(ui->bar_hr, LV_ALIGN_TOP_MID, 0, 45);

// HR bar style
static lv_style_t bar_style_hr;
lv_style_init(&bar_style_hr);
lv_style_set_bg_color(&bar_style_hr, lv_color_hex(0x2a2a2a));
lv_style_set_bg_opa(&bar_style_hr, LV_OPA_COVER);
lv_style_set_border_width(&bar_style_hr, 0);
lv_style_set_radius(&bar_style_hr, 3);
lv_obj_add_style(ui->bar_hr, &bar_style_hr, LV_PART_MAIN);

// HR bar indicator style
static lv_style_t bar_indic_hr;
lv_style_init(&bar_indic_hr);
lv_style_set_bg_color(&bar_indic_hr, lv_color_hex(0xFF1744));
lv_style_set_radius(&bar_indic_hr, 3);
lv_obj_add_style(ui->bar_hr, &bar_indic_hr, LV_PART_INDICATOR);

// ===== SPO2 SECTION =====
// SpO2 label
ui->lbl_spo2_dashboard = lv_label_create(data_container);
lv_label_set_text(ui->lbl_spo2_dashboard, "SpO2");
lv_obj_set_style_text_color(ui->lbl_spo2_dashboard, lv_color_hex(0x00E676), LV_PART_MAIN);
lv_obj_set_style_text_font(ui->lbl_spo2_dashboard, &lv_font_montserrat_10, LV_PART_MAIN);
lv_obj_align(ui->lbl_spo2_dashboard, LV_ALIGN_TOP_LEFT, 0, 60);

// SpO2 value
lv_obj_t *lbl_spo2_value = lv_label_create(data_container);
lv_label_set_text(lbl_spo2_value, "--%");
lv_obj_set_style_text_color(lbl_spo2_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
lv_obj_set_style_text_font(lbl_spo2_value, &lv_font_montserrat_12, LV_PART_MAIN);
lv_obj_align(lbl_spo2_value, LV_ALIGN_TOP_RIGHT, 0, 60);

// SpO2 bar
ui->bar_spo2 = lv_bar_create(data_container);
lv_obj_set_size(ui->bar_spo2, 122, 4);
lv_bar_set_range(ui->bar_spo2, 0, 100);
lv_bar_set_value(ui->bar_spo2, 0, LV_ANIM_OFF);
lv_obj_align(ui->bar_spo2, LV_ALIGN_TOP_MID, 0, 75);

// SpO2 bar style
static lv_style_t bar_style_spo2;
lv_style_init(&bar_style_spo2);
lv_style_set_bg_color(&bar_style_spo2, lv_color_hex(0x2a2a2a));
lv_style_set_bg_opa(&bar_style_spo2, LV_OPA_COVER);
lv_style_set_border_width(&bar_style_spo2, 0);
lv_style_set_radius(&bar_style_spo2, 3);
lv_obj_add_style(ui->bar_spo2, &bar_style_spo2, LV_PART_MAIN);

// SpO2 bar indicator style
static lv_style_t bar_indic_spo2;
lv_style_init(&bar_indic_spo2);
lv_style_set_bg_color(&bar_indic_spo2, lv_color_hex(0x00E676));
lv_style_set_radius(&bar_indic_spo2, 3);
lv_obj_add_style(ui->bar_spo2, &bar_indic_spo2, LV_PART_INDICATOR);

    /* ---------- TEMPERATURE---------- */
    ui->scr_temp = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_temp, lv_color_black(), LV_PART_MAIN);
    ui_create_status_bar(ui->scr_temp, &ui->lbl_time_temp, &ui->lbl_battery_percent_temp, &ui->lbl_battery_icon_temp);

    // Title
    lv_obj_t *lbl_temp_title = lv_label_create(ui->scr_temp);
    lv_label_set_text(lbl_temp_title, "Temperature");
    lv_obj_set_style_text_color(lbl_temp_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_temp_title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(lbl_temp_title, LV_ALIGN_TOP_MID, 0, 25);

    // Temperature Arc Gauge
    ui->arc_temp = lv_arc_create(ui->scr_temp);
    lv_obj_set_size(ui->arc_temp, 80, 80);
    lv_obj_align(ui->arc_temp, LV_ALIGN_CENTER, 0, -5);
     lv_obj_align(ui->arc_temp, LV_ALIGN_TOP_MID, 0, 45);
    lv_arc_set_range(ui->arc_temp, 30, 45);
    lv_arc_set_value(ui->arc_temp, 36);
    lv_arc_set_bg_angles(ui->arc_temp, 135, 45);

    // Style for arc background
    static lv_style_t style_arc_bg;
    lv_style_init(&style_arc_bg);
    lv_style_set_arc_width(&style_arc_bg, 8);
    lv_style_set_arc_color(&style_arc_bg, lv_color_hex(0x333333));
    lv_obj_add_style(ui->arc_temp, &style_arc_bg, LV_PART_MAIN);

    // Style for arc indicator
    static lv_style_t style_arc_indic;
    lv_style_init(&style_arc_indic);
    lv_style_set_arc_width(&style_arc_indic, 8);
    lv_style_set_arc_color(&style_arc_indic, lv_color_hex(0xFF6B35));
    lv_obj_add_style(ui->arc_temp, &style_arc_indic, LV_PART_INDICATOR);

    lv_obj_clear_flag(ui->arc_temp, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(ui->arc_temp, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(ui->arc_temp, 0, LV_PART_KNOB);

    // Temperature value label in center of arc
    ui->lbl_temp_arc_value = lv_label_create(ui->scr_temp);
    lv_label_set_text(ui->lbl_temp_arc_value, "36.5°C");
    lv_obj_set_style_text_font(ui->lbl_temp_arc_value, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui->lbl_temp_arc_value, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui->lbl_temp_arc_value, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ui->lbl_temp_arc_value, LV_ALIGN_TOP_MID, 0, 65);
    lv_obj_align(ui->lbl_temp_arc_value, LV_ALIGN_CENTER, 0, -5);

    // Instruction text below arc
    ui->lbl_temp = lv_label_create(ui->scr_temp);
    lv_label_set_text(ui->lbl_temp, "Press SELECT to scan");
    lv_obj_set_style_text_align(ui->lbl_temp, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui->lbl_temp, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->lbl_temp, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(ui->lbl_temp, LV_ALIGN_CENTER, 0, 50);

    /* ---------- HEART RATE ---------- */
    ui->scr_hr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_hr, lv_color_black(), LV_PART_MAIN);
    ui_create_status_bar(ui->scr_hr, &ui->lbl_time_hr, &ui->lbl_battery_percent_hr, &ui->lbl_battery_icon_hr);

    // Title
    lv_obj_t *lbl_hr_title = lv_label_create(ui->scr_hr);
    lv_label_set_text(lbl_hr_title, "Heart Rate Monitor");
    lv_obj_set_style_text_color(lbl_hr_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_hr_title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(lbl_hr_title, LV_ALIGN_TOP_MID, 0, 25);

    // ========== HR SECTION ==========
    // HR display label
    ui->lbl_hr = lv_label_create(ui->scr_hr);
    lv_label_set_text(ui->lbl_hr, "Heart Rate: -- bpm");
    lv_obj_set_style_text_color(ui->lbl_hr, lv_color_hex(0xFF1744), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->lbl_hr, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(ui->lbl_hr, LV_ALIGN_TOP_LEFT, 8, 42);

    // Create HR chart 
    ui->chart_hr = lv_chart_create(ui->scr_hr);
    lv_obj_set_size(ui->chart_hr, 112, 35);
    lv_obj_align(ui->chart_hr, LV_ALIGN_TOP_MID, 0, 55);
    lv_chart_set_type(ui->chart_hr, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ui->chart_hr, 20);
    lv_chart_set_range(ui->chart_hr, LV_CHART_AXIS_PRIMARY_Y, 40, 160);

    // Style for HR chart
    static lv_style_t style_chart_hr;
    lv_style_init(&style_chart_hr);
    lv_style_set_bg_color(&style_chart_hr, lv_color_hex(0x2a1a1a));
    lv_style_set_bg_opa(&style_chart_hr, LV_OPA_COVER);
    lv_style_set_border_width(&style_chart_hr, 1);
    lv_style_set_border_color(&style_chart_hr, lv_color_hex(0xFF1744));
    lv_style_set_pad_all(&style_chart_hr, 3);
    lv_obj_add_style(ui->chart_hr, &style_chart_hr, 0);

    // Create series for HR 
    ui->ser_hr = lv_chart_add_series(ui->chart_hr, lv_color_hex(0xFF1744), LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_line_width(ui->chart_hr, 1, LV_PART_ITEMS);

    // ========== SPO2 SECTION ==========
    // SpO2 display label
    ui->lbl_spo2 = lv_label_create(ui->scr_hr);
    lv_label_set_text(ui->lbl_spo2, "Blood Oxygen: --%");
    lv_obj_set_style_text_color(ui->lbl_spo2, lv_color_hex(0x00E676), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->lbl_spo2, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(ui->lbl_spo2, LV_ALIGN_TOP_LEFT, 8, 97);

    // Create SpO2 chart 
    ui->chart_spo2 = lv_chart_create(ui->scr_hr);
    lv_obj_set_size(ui->chart_spo2, 112, 35);
    lv_obj_align(ui->chart_spo2, LV_ALIGN_TOP_MID, 0, 110);
    lv_chart_set_type(ui->chart_spo2, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ui->chart_spo2, 20);
    lv_chart_set_range(ui->chart_spo2, LV_CHART_AXIS_PRIMARY_Y, 85, 100);

    // Style for SpO2 chart
    static lv_style_t style_chart_spo2;
    lv_style_init(&style_chart_spo2);
    lv_style_set_bg_color(&style_chart_spo2, lv_color_hex(0x1a2a1a));
    lv_style_set_bg_opa(&style_chart_spo2, LV_OPA_COVER);
    lv_style_set_border_width(&style_chart_spo2, 1);
    lv_style_set_border_color(&style_chart_spo2, lv_color_hex(0x00E676));
    lv_style_set_pad_all(&style_chart_spo2, 3);
    lv_obj_add_style(ui->chart_spo2, &style_chart_spo2, 0);

    // Create series for SpO2 (green line)
    ui->ser_spo2 = lv_chart_add_series(ui->chart_spo2, lv_color_hex(0x00E676), LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_line_width(ui->chart_spo2, 1, LV_PART_ITEMS);


    // Initialize data arrays
    ui->data_index = 0;
    for (int i = 0; i < 20; i++)
    {
        ui->hr_data_points[i] = 0;
        ui->spo2_data_points[i] = 95;
    }
    /* -------- WIFI--------- */
    ui->scr_wifi = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_wifi, lv_color_black(), LV_PART_MAIN);
    ui_create_status_bar(ui->scr_wifi, &ui->lbl_time_wifi, &ui->lbl_battery_percent_wifi, &ui->lbl_battery_icon_wifi);

    // TITLE
    lv_obj_t *lbl_wifi_title = lv_label_create(ui->scr_wifi);
    lv_label_set_text(lbl_wifi_title, "Wifi setting");
    lv_obj_set_style_text_color(lbl_wifi_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_wifi_title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(lbl_wifi_title, LV_ALIGN_TOP_MID, 0, 25);

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

    /* -------- BLUETOOTH ---------*/
    ui->scr_bluetooth = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->scr_bluetooth, lv_color_black(), LV_PART_MAIN);
    ui_create_status_bar(ui->scr_bluetooth, &ui->lbl_time_bluetooth, &ui->lbl_battery_percent_bluetooth, &ui->lbl_battery_icon_bluetooth);

    // Title
    lv_obj_t *lbl_bluetooth_title = lv_label_create(ui->scr_bluetooth);
    lv_label_set_text(lbl_bluetooth_title, "Bluetooth setting");
    lv_obj_set_style_text_color(lbl_bluetooth_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_bluetooth_title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(lbl_bluetooth_title, LV_ALIGN_TOP_MID, 0, 25);

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
    if (ui == NULL)
    {
        ESP_LOGE("UI_MANAGER", "UI manager is NULL");
        return;
    }

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

    if (target == NULL)
    {
        ESP_LOGE("UI_MANAGER", "Target screen is NULL for state %d", new_state);
        target = ui->scr_home;
        ui->current_state = UI_STATE_HOME;
    }

    if (target != NULL && lv_obj_is_valid(target))
    {
        lv_scr_load(target);
        ESP_LOGI("UI_MANAGER", "Switched to state %d", new_state);
    }
    else
    {
        ESP_LOGE("UI_MANAGER", "Failed to switch to state %d - invalid target", new_state);
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

void ui_add_hr_data_point(ui_manager_t *ui, int hr)
{
    if (!ui || ui->current_state != UI_STATE_HR)
        return;

    // Store HR data point
    ui->hr_data_points[ui->data_index] = hr;

    // Update HR chart with all data points
    lv_chart_set_all_value(ui->chart_hr, ui->ser_hr, 0);
    for (int i = 0; i < 20; i++)
    {
        int idx = (ui->data_index + i + 1) % 20;
        lv_chart_set_next_value(ui->chart_hr, ui->ser_hr, ui->hr_data_points[idx]);
    }
    lv_chart_refresh(ui->chart_hr);
}

void ui_add_spo2_data_point(ui_manager_t *ui, int spo2)
{
    if (!ui || ui->current_state != UI_STATE_HR)
        return;

    // Store SpO2 data point
    ui->spo2_data_points[ui->data_index] = spo2;

    // Update SpO2 chart with all data points
    lv_chart_set_all_value(ui->chart_spo2, ui->ser_spo2, 0);
    for (int i = 0; i < 20; i++)
    {
        int idx = (ui->data_index + i + 1) % 20;
        lv_chart_set_next_value(ui->chart_spo2, ui->ser_spo2, ui->spo2_data_points[idx]);
    }
    lv_chart_refresh(ui->chart_spo2);
}

void ui_update_temp(ui_manager_t *ui, float t)
{
    if (ui->current_state < UI_STATE_TEMP_IDLE || ui->current_state > UI_STATE_TEMP_RESULT)
        return;

    if (ui->current_state == UI_STATE_TEMP_SCANNING)
    {
        lv_label_set_text(ui->lbl_temp, "Measuring... Please wait");
        lv_obj_set_style_text_color(ui->lbl_temp, lv_color_hex(0xFFEB3B), LV_PART_MAIN);
        lv_label_set_text(ui->lbl_temp_arc_value, "Scanning...");
        lv_obj_set_style_text_color(ui->lbl_temp_arc_value, lv_color_hex(0xFFEB3B), LV_PART_MAIN);

        // Animate arc during scanning
        static lv_style_t style_arc_scanning;
        lv_style_init(&style_arc_scanning);
        lv_style_set_arc_color(&style_arc_scanning, lv_color_hex(0xFFEB3B));
        lv_obj_add_style(ui->arc_temp, &style_arc_scanning, LV_PART_INDICATOR);
    }
    else if (t < -273.0f || isnan(t))
    {
        lv_label_set_text(ui->lbl_temp, "Sensor Error - Press SELECT");
        lv_obj_set_style_text_color(ui->lbl_temp, lv_color_hex(0xFF5722), LV_PART_MAIN);
        lv_label_set_text(ui->lbl_temp_arc_value, "ERROR");
        lv_obj_set_style_text_color(ui->lbl_temp_arc_value, lv_color_hex(0xFF5722), LV_PART_MAIN);
        lv_arc_set_value(ui->arc_temp, 30);

        static lv_style_t style_arc_error;
        lv_style_init(&style_arc_error);
        lv_style_set_arc_color(&style_arc_error, lv_color_hex(0xFF5722));
        lv_obj_add_style(ui->arc_temp, &style_arc_error, LV_PART_INDICATOR);
    }
    else
    {
        char temp_str[20];
        snprintf(temp_str, sizeof(temp_str), "%.1f°C", t);
        lv_label_set_text(ui->lbl_temp_arc_value, temp_str);
        lv_label_set_text(ui->lbl_temp, "Press SELECT to rescan");
        lv_obj_set_style_text_color(ui->lbl_temp, lv_color_hex(0xAAAAAA), LV_PART_MAIN);

        // Update arc value
        int arc_value = (int)t;
        if (arc_value < 30)
            arc_value = 30;
        if (arc_value > 45)
            arc_value = 45;
        lv_arc_set_value(ui->arc_temp, arc_value);

        lv_color_t temp_color;
        if (t > 37.5)
        {
            temp_color = lv_color_hex(0xFF5722); // Red
        }
        else if (t > 36.0)
        {
            temp_color = lv_color_hex(0x4CAF50); // Green
        }
        else
        {
            temp_color = lv_color_hex(0x2196F3); // Blue
        }

        lv_obj_set_style_text_color(ui->lbl_temp_arc_value, temp_color, LV_PART_MAIN);

        static lv_style_t style_arc_temp;
        lv_style_init(&style_arc_temp);
        lv_style_set_arc_color(&style_arc_temp, temp_color);
        lv_obj_add_style(ui->arc_temp, &style_arc_temp, LV_PART_INDICATOR);
    }
}

void ui_update_hr(ui_manager_t *ui, int hr, int spo2)
{
    if (hr > 0 && spo2 > 0)
    {
        lv_label_set_text_fmt(ui->lbl_hr, "Heart Rate: %d bpm", hr);
        lv_label_set_text_fmt(ui->lbl_spo2, "Blood Oxygen: %d%%", spo2);

        // Add data points to both graphs
        ui_add_hr_data_point(ui, hr);
        ui_add_spo2_data_point(ui, spo2);

        // Update data index for next reading
        ui->data_index = (ui->data_index + 1) % 20;

        // Color coding for HR
        if (hr > 100)
        {
            lv_obj_set_style_text_color(ui->lbl_hr, lv_color_hex(0xFF5722), LV_PART_MAIN);
        }
        else if (hr >= 60)
        {
            lv_obj_set_style_text_color(ui->lbl_hr, lv_color_hex(0x4CAF50), LV_PART_MAIN);
        }
        else
        {
            lv_obj_set_style_text_color(ui->lbl_hr, lv_color_hex(0xFFC107), LV_PART_MAIN);
        }

        // Color coding for SpO2
        if (spo2 >= 95)
        {
            lv_obj_set_style_text_color(ui->lbl_spo2, lv_color_hex(0x4CAF50), LV_PART_MAIN);
        }
        else if (spo2 >= 90)
        {
            lv_obj_set_style_text_color(ui->lbl_spo2, lv_color_hex(0xFFC107), LV_PART_MAIN);
        }
        else
        {
            lv_label_set_text(ui->lbl_hr, "HR: Scanning...");
            lv_label_set_text(ui->lbl_spo2, "SpO2: Please wait");
            lv_obj_set_style_text_color(ui->lbl_hr, lv_color_hex(0xFFEB3B), LV_PART_MAIN);
            lv_obj_set_style_text_color(ui->lbl_spo2, lv_color_hex(0xFFEB3B), LV_PART_MAIN);
        }
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

void ui_update_all_status_bars(ui_manager_t *ui, const char *time_str, int battery_percent)
{
    // Update home screen
    if (ui->lbl_time)
    {
        lv_label_set_text(ui->lbl_time, time_str);
    }
    if (ui->lbl_battery_percent)
    {
        lv_label_set_text_fmt(ui->lbl_battery_percent, "%d%%", battery_percent);
    }

    // Update menu screen
    if (ui->lbl_time_menu)
    {
        lv_label_set_text(ui->lbl_time_menu, time_str);
    }
    if (ui->lbl_battery_percent_menu)
    {
        lv_label_set_text_fmt(ui->lbl_battery_percent_menu, "%d%%", battery_percent);
    }

    // Update notify screen
    if (ui->lbl_time_notify)
    {
        lv_label_set_text(ui->lbl_time_notify, time_str);
    }
    if (ui->lbl_battery_percent_notify)
    {
        lv_label_set_text_fmt(ui->lbl_battery_percent_notify, "%d%%", battery_percent);
    }

    // Update temp screen
    if (ui->lbl_time_temp)
    {
        lv_label_set_text(ui->lbl_time_temp, time_str);
    }
    if (ui->lbl_battery_percent_temp)
    {
        lv_label_set_text_fmt(ui->lbl_battery_percent_temp, "%d%%", battery_percent);
    }

    // Update HR screen
    if (ui->lbl_time_hr)
    {
        lv_label_set_text(ui->lbl_time_hr, time_str);
    }
    if (ui->lbl_battery_percent_hr)
    {
        lv_label_set_text_fmt(ui->lbl_battery_percent_hr, "%d%%", battery_percent);
    }

    // Update data screen
    if (ui->lbl_time_data)
    {
        lv_label_set_text(ui->lbl_time_data, time_str);
    }
    if (ui->lbl_battery_percent_data)
    {
        lv_label_set_text_fmt(ui->lbl_battery_percent_data, "%d%%", battery_percent);
    }

    // Update settings screen
    if (ui->lbl_time_settings)
    {
        lv_label_set_text(ui->lbl_time_settings, time_str);
    }
    if (ui->lbl_battery_percent_settings)
    {
        lv_label_set_text_fmt(ui->lbl_battery_percent_settings, "%d%%", battery_percent);
    }

    // Update WiFi screen
    if (ui->lbl_time_wifi)
    {
        lv_label_set_text(ui->lbl_time_wifi, time_str);
    }
    if (ui->lbl_battery_percent_wifi)
    {
        lv_label_set_text_fmt(ui->lbl_battery_percent_wifi, "%d%%", battery_percent);
    }

    // Update Bluetooth screen
    if (ui->lbl_time_bluetooth)
    {
        lv_label_set_text(ui->lbl_time_bluetooth, time_str);
    }
    if (ui->lbl_battery_percent_bluetooth)
    {
        lv_label_set_text_fmt(ui->lbl_battery_percent_bluetooth, "%d%%", battery_percent);
    }

    // Update battery icon color based on percentage for all screens
    lv_color_t battery_color;
    const char *battery_icon;

    if (battery_percent > 75)
    {
        battery_color = lv_color_hex(0x41D958); // Green
        battery_icon = LV_SYMBOL_BATTERY_FULL;
    }
    else if (battery_percent > 50)
    {
        battery_color = lv_color_hex(0xFFEB3B); // Yellow
        battery_icon = LV_SYMBOL_BATTERY_3;
    }
    else if (battery_percent > 25)
    {
        battery_color = lv_color_hex(0xFF9800); // Orange
        battery_icon = LV_SYMBOL_BATTERY_2;
    }
    else
    {
        battery_color = lv_color_hex(0xFF5722); // Red
        battery_icon = LV_SYMBOL_BATTERY_1;
    }

    // Update all battery icons
    lv_obj_t *battery_icons[] = {
        ui->lbl_battery_icon,
        ui->lbl_battery_icon_menu,
        ui->lbl_battery_icon_notify,
        ui->lbl_battery_icon_temp,
        ui->lbl_battery_icon_hr,
        ui->lbl_battery_icon_data,
        ui->lbl_battery_icon_settings,
        ui->lbl_battery_icon_wifi,
        ui->lbl_battery_icon_bluetooth};

    for (int i = 0; i < sizeof(battery_icons) / sizeof(battery_icons[0]); i++)
    {
        if (battery_icons[i])
        {
            lv_obj_set_style_text_color(battery_icons[i], battery_color, LV_PART_MAIN);
            lv_label_set_text(battery_icons[i], battery_icon);
        }
    }
}

void ui_create_status_bar(lv_obj_t *screen, lv_obj_t **time_label, lv_obj_t **battery_percent, lv_obj_t **battery_icon)
{
    // Time label
    *time_label = lv_label_create(screen);
    lv_label_set_text(*time_label, "17:36");
    lv_obj_set_style_text_font(*time_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(*time_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(*time_label, LV_ALIGN_TOP_LEFT, 4, 4);

    // Battery percent
    *battery_percent = lv_label_create(screen);
    lv_obj_set_style_text_font(*battery_percent, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_label_set_text(*battery_percent, "80%");
    lv_obj_set_style_text_color(*battery_percent, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(*battery_percent, LV_ALIGN_TOP_RIGHT, -20, 5);

    // Battery icon
    *battery_icon = lv_label_create(screen);
    lv_obj_set_style_text_font(*battery_icon, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_label_set_text(*battery_icon, LV_SYMBOL_BATTERY_3);
    lv_obj_set_style_text_color(*battery_icon, lv_color_hex(0x41D958), LV_PART_MAIN);
    lv_obj_align(*battery_icon, LV_ALIGN_TOP_RIGHT, -4, 3.5);
}