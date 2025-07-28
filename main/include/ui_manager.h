#pragma once
#include "lvgl.h"
#include "button.h"

#define COLOR_BG_NORMAL    lv_color_hex(0x242424)
#define COLOR_BG_SELECTED  lv_color_hex(0x919191)
#define COLOR_TEXT_NORMAL  lv_color_white()
#define COLOR_TEXT_SELECTED lv_color_white()
#define COLOR_TEXT lv_color_hex(0xFFFFFF)

extern const lv_img_dsc_t gps_icon;
extern const lv_img_dsc_t temp_icon;
extern const lv_img_dsc_t heart_icon;
extern const lv_img_dsc_t data_icon;
extern const lv_img_dsc_t notify_icon;
extern const lv_img_dsc_t wifi_icon;
extern const lv_img_dsc_t bluetooth_icon;


#define GPS_ICON  (&gps_icon)
#define TEMP_ICON  (&temp_icon)
#define HEART_ICON  (&heart_icon)
#define DATA_ICON  (&data_icon)
#define NOTIFY_ICON  (&notify_icon)
#define WIFI_ICON  (&wifi_icon)
#define BLUETOOTH_ICON  (&bluetooth_icon)


typedef enum {
    UI_STATE_HOME,
    UI_STATE_MENU,
    UI_STATE_NOTIFY,
    UI_STATE_TEMP_IDLE,
    UI_STATE_TEMP_SCANNING,
    UI_STATE_TEMP_RESULT,
    UI_STATE_HR,
    UI_STATE_GPS,
    UI_STATE_DATA,
    UI_STATE_WIFI,
    UI_STATE_BLUETOOTH
} ui_state_t;

typedef struct {
    int battery;
    char date[11];
} ui_info_t;

typedef struct {
    const char *name;
    ui_state_t  state;
} menu_item_t;

typedef struct {
    /* Screens */
    lv_obj_t *scr_home;
    lv_obj_t *scr_menu;
    lv_obj_t *scr_temp;
    lv_obj_t *scr_hr;
    lv_obj_t *scr_gps;

    /* Common widgets */
    lv_obj_t *lbl_battery;
    lv_obj_t *lbl_date;

    /* Home */
    lv_obj_t *img_bg;
    lv_obj_t *lbl_datetime;

    /* Menu (single lv_list) */
    lv_obj_t *list_menu;
    /* Menu data */
    menu_item_t menu_items[10];
    int          menu_item_count;
    int          selected_index;

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
    ui_info_t  info;
    uint32_t   scan_start_time_ms;
    bool       scan_done;
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



/*
#pragma once

#include "u8g2.h"
#include "button.h"

typedef enum
{
    UI_STATE_HOME, 
    UI_STATE_MENU,
    UI_STATE_TEMP_IDLE,
    UI_STATE_TEMP_SCANNING,
    UI_STATE_TEMP_RESULT,
    UI_STATE_HR,
    UI_STATE_GPS
} ui_state_t;

typedef struct
{
    int battery;
    char date[11]; // "YYYY-MM-DD"
} ui_info_t;

typedef struct
{
    const char *name;
    ui_state_t state;
} menu_item_t;

typedef struct
{
    u8g2_t *u8g2;
    ui_state_t current_state;
    int selected_index;
    menu_item_t menu_items[3];
    int menu_item_count;
    ui_info_t info;
    uint32_t scan_start_time_ms;
    bool scan_done;

} ui_manager_t;

void ui_manager_init(ui_manager_t *ui, u8g2_t *u8g2);
void ui_manager_handle_button(ui_manager_t *ui, button_id_t btn);
void ui_manager_update_display(ui_manager_t *ui);
*/