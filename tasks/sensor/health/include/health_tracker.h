#include <stdbool.h>

#pragma once
#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    int heart_rate;
    int spo2;
    bool valid;
} health_data_t;

void health_init(void);

// Read one HR/SpO₂ sample; update the global health_data internally
void health_update(void);

void health_get_data(health_data_t *out);
