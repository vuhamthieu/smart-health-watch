#include <stdbool.h>

#pragma once

typedef struct {
    int heart_rate;
    int spo2;
    bool valid;
} health_data_t;

void health_get_data(health_data_t *data);
void health_task(void *pv);