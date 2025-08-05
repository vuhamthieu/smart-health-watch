#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <esp_err.h>

typedef struct {
    int data_type; 
    union {
        float temperature;
        struct { int heart_rate; int spo2; } health;
        struct { float lat; float lon; } gps;
    } data;
} http_message_t;

void http_client_init(void);
void http_client_send_temp(float temperature);
void http_client_send_hr_spo2(int heart_rate, int spo2);
void http_client_send_gps(float latitude, float longitude);
void http_client_task(void *pv);

#endif