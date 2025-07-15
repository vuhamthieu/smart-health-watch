#include <stdio.h>
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "temp.h"
#include "display.h"

void app_main(void)
{
    temp_init();
    display_init();

    while (1) {
        float t = temp_read_celsius();
        display_show_temp(t);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
