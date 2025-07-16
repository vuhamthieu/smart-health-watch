#include <stdio.h>
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "temp.h"
#include "driver/i2c.h"

#define I2C_MASTER_NUM I2C_NUM_0

void app_main(void)
{
    display_init();  
    temp_init();   

    // Scan I2C bus
    printf("Scanning I2C bus on I2C_NUM_0...\n");
    for (int addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            printf("Found I2C device at address: 0x%02X\n", addr);
            }
    }


    while (1) {
        float t = temp_read_celsius();
        display_show_temp(t);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
