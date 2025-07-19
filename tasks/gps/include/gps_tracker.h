#pragma once
#include <stdbool.h>

typedef struct {
    float latitude;
    float longitude;
    bool valid;
} gps_data_t;

// Task chính sẽ khởi tạo UART và đọc dữ liệu
void gps_task(void *pvParameters);

// Lấy ra dữ liệu GPS mới nhất (thread‑safe đủ cho use case đơn giản)
void gps_get_data(gps_data_t *out);
