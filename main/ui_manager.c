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
        u8g2_DrawStr(ui->u8g2, 0, 60, "Please wait 30 seconds");
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
