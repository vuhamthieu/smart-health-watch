#pragma once

#include "u8g2.h"
#include "button.h"

typedef enum
{
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