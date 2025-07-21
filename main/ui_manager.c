#include "ui_manager.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "u8g2_esp32_hal.h"
#include "button.h"
#include "gps_tracker.h"
#include "health_tracker.h"
#include "temperature_task.h"

static const char *TAG = "UI_MANAGER";

void ui_manager_init(ui_manager_t *ui, u8g2_t *u8g2)
{
    ui->u8g2 = u8g2;
    ui->current_state = UI_STATE_MENU;
    ui->selected_index = 0;
    ui->menu_item_count = 3;
    memset(ui->info.date, 0, sizeof(ui->info.date));
    ui->info.battery = 100;
    ui->menu_items[0] = (menu_item_t){"GPS Location", UI_STATE_GPS};
    ui->menu_items[1] = (menu_item_t){"Body Temperature", UI_STATE_TEMP};
    ui->menu_items[2] = (menu_item_t){"Heart Rate + SpO2", UI_STATE_HR};
    ESP_LOGI(TAG, "UI Manager initialized");
}

void ui_manager_handle_button(ui_manager_t *ui, button_id_t btn)
{
    switch (ui->current_state)
    {
        case UI_STATE_MENU:
            if (btn == BUTTON_UP && ui->selected_index > 0) {
                ui->selected_index--;
            }
            else if (btn == BUTTON_DOWN && ui->selected_index < ui->menu_item_count - 1) {
                ui->selected_index++;
            }
            else if (btn == BUTTON_SELECT) {
                ui_state_t new_state = ui->menu_items[ui->selected_index].state;
                ui->current_state = new_state;
                if (new_state == UI_STATE_TEMP) {
                    temperature_init();
                } else if (new_state == UI_STATE_HR) {
                    health_init();
                } else if (new_state == UI_STATE_GPS) {
                    gps_init();
                }
            }
            break;

        case UI_STATE_GPS:
        case UI_STATE_TEMP:
        case UI_STATE_HR:
            if (btn == BUTTON_BACK) {
                ui->current_state = UI_STATE_MENU;
            }
            break;
    }
    ESP_LOGI(TAG, "Button %d -> State %d, Index %d",
             btn, ui->current_state, ui->selected_index);
}

void ui_manager_update_display(ui_manager_t *ui)
{
    u8g2_ClearBuffer(ui->u8g2);
    u8g2_SetFont(ui->u8g2, u8g2_font_ncenB08_tr);
    char buf[50];

    if (strlen(ui->info.date) > 0) {
        u8g2_DrawStr(ui->u8g2, 0, 10, ui->info.date);
    } else {
        u8g2_DrawStr(ui->u8g2, 0, 10, "2026-01-01");
    }

    int battery = ui->info.battery;
    // battery frame: 16x8 pixel
    u8g2_DrawFrame(ui->u8g2, 100, 2, 16, 8);
    // + polar: 2x4 pixel
    u8g2_DrawBox(ui->u8g2, 116, 4, 2, 4);
    // battery pảrt: 4 part, 3x6 pixel each
    int bars = (battery + 24) / 25; 
    for (int i = 0; i < bars && i < 4; i++) {
        u8g2_DrawBox(ui->u8g2, 102 + i * 3, 4, 2, 4);
    }

    switch (ui->current_state)
    {
        case UI_STATE_MENU:
            for (int i = 0; i < ui->menu_item_count; i++) {
                if (i == ui->selected_index) {
                    u8g2_DrawStr(ui->u8g2, 0, (i+1)*15 + 15, "->");
                    u8g2_DrawStr(ui->u8g2, 20, (i+1)*15 + 15, ui->menu_items[i].name);
                } else {
                    u8g2_DrawStr(ui->u8g2, 20, (i+1)*15 + 15, ui->menu_items[i].name);
                }
            }
            break;

        case UI_STATE_GPS: {
            gps_data_t gps;
            gps_get_data(&gps);
            u8g2_DrawStr(ui->u8g2, 0, 30, "GPS LOCATION");
            if (gps.valid) {
                snprintf(buf, sizeof(buf), "Lat: %.6f", gps.latitude);
                u8g2_DrawStr(ui->u8g2, 0, 50, buf);
                snprintf(buf, sizeof(buf), "Lon: %.6f", gps.longitude);
                u8g2_DrawStr(ui->u8g2, 0, 65, buf);
            } else {
                u8g2_DrawStr(ui->u8g2, 0, 50, "No signal");
            }
            break;
        }

        case UI_STATE_TEMP: {
            float t = temperature_get_data();
            u8g2_DrawStr(ui->u8g2, 0, 30, "BODY TEMPERATURE");
            if (t <= -273.15f) {
                u8g2_DrawStr(ui->u8g2, 0, 50, "Scanning ...");
            } else {
                snprintf(buf, sizeof(buf), "Temp: %.2f C", t);
                u8g2_DrawStr(ui->u8g2, 0, 50, buf);
            }
            break;
        }

        case UI_STATE_HR: {
            health_data_t hd;
            health_get_data(&hd);
            u8g2_DrawStr(ui->u8g2, 0, 30, "HEART RATE + SPO2");
            if (hd.heart_rate > 0 && hd.spo2 > 0) {
                snprintf(buf, sizeof(buf), "HR: %d bpm", hd.heart_rate);
                u8g2_DrawStr(ui->u8g2, 0, 50, buf);
                snprintf(buf, sizeof(buf), "SpO2: %d%%", hd.spo2);
                u8g2_DrawStr(ui->u8g2, 0, 65, buf);
            } else {
                u8g2_DrawStr(ui->u8g2, 0, 50, "Scanning ...");
            }
            break;
        }
    }

    u8g2_SendBuffer(ui->u8g2);
}