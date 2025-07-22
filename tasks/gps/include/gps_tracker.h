#pragma once
#include <stdbool.h>

typedef struct {
    float latitude;
    float longitude;
    bool valid;
} gps_data_t;

// Call once at startup to configure UART & ISR
void gps_init(void);

// Do one “read NMEA line → parse → update” pass
void gps_update(void);

// Read out the most‑recent result
void gps_get_data(gps_data_t *out);
    