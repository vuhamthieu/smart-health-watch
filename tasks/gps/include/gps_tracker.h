#pragma once
#include <stdbool.h>

typedef struct {
    float latitude;
    float longitude;
    bool valid;
} gps_data_t;

void gps_task(void *pvParameters);
void gps_get_data(gps_data_t *out);
