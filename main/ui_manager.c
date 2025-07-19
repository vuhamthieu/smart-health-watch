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

void ui_manager_init(ui_manager_t *ui, u8g2_t *u8g2) {
    ui->u8g2 = u8g2;
    ui->current_state = UI_STATE_MENU;
    ui->selected_index = 0;
    ui->menu_item_count = 3;
    ui->menu_items[0] = (menu_item_t){"GPS Location", UI_STATE_GPS};
    ui->menu_items[1] = (menu_item_t){"Body Temperature", UI_STATE_TEMP};
    ui->menu_items[2] = (menu_item_t){"Heart Rate + SpO2", UI_STATE_HR};
    ESP_LOGI(TAG, "UI Manager initialized");
}

void ui_manager_handle_button(ui_manager_t *ui, button_id_t btn) {
    switch (ui->current_state) {
        case UI_STATE_MENU:
            if (btn == BUTTON_UP && ui->selected_index > 0) {
                ui->selected_index--;
            } else if (btn == BUTTON_DOWN && ui->selected_index < ui->menu_item_count - 1) {
                ui->selected_index++;
            } else if (btn == BUTTON_SELECT) {
                ui->current_state = ui->menu_items[ui->selected_index].state;
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
    ESP_LOGI(TAG, "Button %d -> State %d, Index %d", btn, ui->current_state, ui->selected_index);
}

void ui_manager_update_display(ui_manager_t *ui) {
    u8g2_ClearBuffer(ui->u8g2);
    u8g2_SetFont(ui->u8g2, u8g2_font_ncenB08_tr);
    char buf[50];

    switch (ui->current_state) {
        case UI_STATE_MENU:
            for (int i = 0; i < ui->menu_item_count; i++) {
                if (i == ui->selected_index) {
                    u8g2_DrawStr(ui->u8g2, 0, (i + 1) * 15, "->");
                    u8g2_DrawStr(ui->u8g2, 20, (i + 1) * 15, ui->menu_items[i].name);
                } else {
                    u8g2_DrawStr(ui->u8g2, 20, (i + 1) * 15, ui->menu_items[i].name);
                }
            }
            break;
        case UI_STATE_GPS: {
            gps_data_t gps_data;
            gps_get_data(&gps_data);
            if (gps_data.valid) {
                snprintf(buf, sizeof(buf), "Lat: %.6f\nLon: %.6f", gps_data.latitude, gps_data.longitude);
            } else {
                snprintf(buf, sizeof(buf), "GPS: No signal");
            }
            u8g2_DrawStr(ui->u8g2, 0, 15, "GPS LOCATION");
            u8g2_DrawStr(ui->u8g2, 0, 35, buf);
            break;
        }
        case UI_STATE_TEMP: {
            float temp = temperature_get_data();
            if (temp == -273.15f) {
                u8g2_DrawStr(ui->u8g2, 0, 35, "Scanning ...");
            } else {
                snprintf(buf, sizeof(buf), "Temp: %.2f C", temp);
                u8g2_DrawStr(ui->u8g2, 0, 35, buf);
            }
            u8g2_DrawStr(ui->u8g2, 0, 15, "BODY TEMPERATURE");
            break;
        }
        case UI_STATE_HR: {
            health_data_t health_data;
            health_get_data(&health_data);
            if (health_data.valid) {
                snprintf(buf, sizeof(buf), "HR: %d bpm\nSpO2: %d%%", health_data.heart_rate, health_data.spo2);
            } else {
                snprintf(buf, sizeof(buf), "No data");
            }
            u8g2_DrawStr(ui->u8g2, 0, 15, "HEART RATE + SPO2");
            u8g2_DrawStr(ui->u8g2, 0, 35, buf);
            break;
        }
    }

    u8g2_SendBuffer(ui->u8g2);
}