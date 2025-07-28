#pragma once
#include "lvgl.h"
#include "button.h"

#define COLOR_BG_NORMAL lv_color_hex(0x857B7B)  
#define COLOR_BG_SELECTED lv_color_hex(0x4A90E2) 
#define COLOR_TEXT_NORMAL lv_color_white()
#define COLOR_TEXT_SELECTED lv_color_white()

typedef enum {
    UI_STATE_HOME,
    UI_STATE_MENU,
    UI_STATE_TEMP_IDLE,
    UI_STATE_TEMP_SCANNING,  
    UI_STATE_TEMP_RESULT,
    UI_STATE_HR,
    UI_STATE_GPS
} ui_state_t;

typedef struct {
    int battery;
    char date[11];
} ui_info_t;

typedef struct {
    const char *name;
    ui_state_t state;  
} menu_item_t;

typedef struct {
    /* LVGL screen objects */
    lv_obj_t *scr_home;
    lv_obj_t *scr_menu;
    lv_obj_t *scr_temp;
    lv_obj_t *scr_hr;
    lv_obj_t *scr_gps;
    
    /* LVGL widget objects */
    lv_obj_t *lbl_battery;
    lv_obj_t *lbl_date;
    lv_obj_t *lbl_temp;
    lv_obj_t *lbl_hr;
    lv_obj_t *lbl_spo2;
    lv_obj_t *lbl_gps;
    lv_obj_t *img_heart;
    lv_obj_t *list_menu;
    
    /* Menu management*/
    lv_obj_t *menu_buttons[10];   
    int menu_item_count;
    int selected_index;
    int visible_start;
    int max_visible_items;
    
    /* State management */
    ui_state_t current_state;
    menu_item_t menu_items[10];      
    ui_info_t info;
    uint32_t scan_start_time_ms;
    bool scan_done;
} ui_manager_t;

/* API functions */
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