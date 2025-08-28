#pragma once
#include "lvgl.h"
#include "button.h"

#define COLOR_BG_NORMAL lv_color_hex(0x242424)
#define COLOR_BG_SELECTED lv_color_hex(0x919191)
#define COLOR_TEXT_NORMAL lv_color_white()
#define COLOR_TEXT_SELECTED lv_color_white()
#define COLOR_TEXT lv_color_hex(0xFFFFFF)

extern const lv_img_dsc_t gps_icon;
extern const lv_img_dsc_t temp_icon;
extern const lv_img_dsc_t heart_icon;
extern const lv_img_dsc_t data_icon;
extern const lv_img_dsc_t notify_icon;
extern const lv_img_dsc_t wifi_icon;
extern const lv_img_dsc_t bluetooth_icon;

#define GPS_ICON (&gps_icon)
#define TEMP_ICON (&temp_icon)
#define HEART_ICON (&heart_icon)
#define DATA_ICON (&data_icon)
#define NOTIFY_ICON (&notify_icon)

#define SCREEN_TIMEOUT_MS 30000
#define TFT_BL_PIN 19

typedef enum
{
    UI_STATE_HOME,
    UI_STATE_MENU,
    UI_STATE_NOTIFY,
    UI_STATE_TEMP_IDLE,
    UI_STATE_TEMP_SCANNING,
    UI_STATE_TEMP_RESULT,
    UI_STATE_HR,
    UI_STATE_GPS,
    UI_STATE_DATA,
    UI_STATE_SETTING,
    UI_STATE_WIFI,
    UI_STATE_BLUETOOTH
} ui_state_t;

typedef struct
{
    int battery;
    char date[11];
} ui_info_t;

typedef struct
{
    const char *name;
    ui_state_t state;
} menu_item_t;

typedef struct
{
    /* Screens */
    lv_obj_t *scr_home;
    lv_obj_t *scr_menu;
    lv_obj_t *scr_temp;
    lv_obj_t *scr_hr;
    lv_obj_t *scr_gps;
    lv_obj_t *scr_wifi;
    lv_obj_t *scr_bluetooth;
    lv_obj_t *scr_notify;
    lv_obj_t *scr_data;
    lv_obj_t *scr_settings;

    /* Common widgets */
    lv_obj_t *lbl_battery;
    lv_obj_t *lbl_date;
    lv_obj_t *lbl_wifi;
    lv_obj_t *lbl_bluetooth;
    lv_obj_t *lbl_hr_dashboard;
    lv_obj_t *lbl_spo2_dashboard;
    lv_obj_t *lbl_temp_dashboard;
    lv_obj_t *lbl_time;
    lv_obj_t *battery_container;
    lv_obj_t *lbl_battery_icon;
    lv_obj_t *lbl_battery_fill;
    lv_obj_t *lbl_battery_percent;
    lv_obj_t *lbl_time_menu, *lbl_battery_percent_menu, *lbl_battery_icon_menu;
    lv_obj_t *lbl_time_notify, *lbl_battery_percent_notify, *lbl_battery_icon_notify;
    lv_obj_t *lbl_time_temp, *lbl_battery_percent_temp, *lbl_battery_icon_temp;
    lv_obj_t *lbl_time_hr, *lbl_battery_percent_hr, *lbl_battery_icon_hr;
    lv_obj_t *lbl_time_data, *lbl_battery_percent_data, *lbl_battery_icon_data;
    lv_obj_t *lbl_time_settings, *lbl_battery_percent_settings, *lbl_battery_icon_settings;
    lv_obj_t *lbl_time_wifi, *lbl_battery_percent_wifi, *lbl_battery_icon_wifi;
    lv_obj_t *lbl_time_bluetooth, *lbl_battery_percent_bluetooth, *lbl_battery_icon_bluetooth;

    lv_obj_t *chart_spo2;
    lv_obj_t *chart_hr_main;

    lv_obj_t *bar_temp;
    lv_obj_t *bar_hr;
    lv_obj_t *bar_spo2;

    // Temperature arc gauge
    lv_obj_t *arc_temp;
    lv_obj_t *lbl_temp_arc_value;
    
    // HR/SpO2 chart
    lv_obj_t *chart_hr;
    lv_chart_series_t *ser_hr;
    lv_chart_series_t *ser_spo2;
    int hr_data_points[20]; 
    int spo2_data_points[20]; 
    int data_index;

    /* Status for wifi and bluetooth*/
    lv_obj_t *lbl_wifi_status;
    lv_obj_t *lbl_bluetooth_status;

    /* Home */
    lv_obj_t *img_bg;
    lv_obj_t *lbl_datetime;

    /* Menu (single lv_list) */
    lv_obj_t *list_menu;
    lv_obj_t *list_settings;
    /* Menu data */
    menu_item_t menu_items[10];
    int menu_item_count;
    int selected_index;
    int settings_selected_index;

    /* Temp screen */
    lv_obj_t *lbl_temp;

    /* HR screen */
    lv_obj_t *lbl_hr;
    lv_obj_t *lbl_spo2;
    lv_obj_t *img_heart;

    /* GPS screen */
    lv_obj_t *lbl_gps;

    /* State & info */
    ui_state_t current_state;
    ui_info_t info;
    uint32_t scan_start_time_ms;
    bool scan_done;
} ui_manager_t;

void ui_reset_screen_state(ui_manager_t *ui, ui_state_t state);

void ui_manager_init(ui_manager_t *ui);

void ui_manager_handle_button(ui_manager_t *ui, button_id_t btn);

void ui_switch(ui_manager_t *ui, ui_state_t new_state);

void ui_set_date(ui_manager_t *ui, const char *date);

void ui_set_battery(ui_manager_t *ui, int percent);

void ui_update_temp(ui_manager_t *ui, float t);

void ui_update_hr(ui_manager_t *ui, int hr, int spo2);

void ui_update_gps(ui_manager_t *ui, float lat, float lon, bool valid);

void ui_update_wifi_status(ui_manager_t *ui);

void ui_update_home_wifi_icon(ui_manager_t *ui);

void ui_update_home_bluetooth_icon(ui_manager_t *ui);

void ui_update_bluetooth_status(ui_manager_t *ui);

void ui_update_dashboard(ui_manager_t *ui);

void ui_create_status_bar(lv_obj_t *screen, lv_obj_t **time_label, lv_obj_t **battery_percent, lv_obj_t **battery_icon);

void ui_update_all_status_bars(ui_manager_t *ui, const char *time_str, int battery_percent);

void ui_add_hr_data_point(ui_manager_t *ui, int hr);
