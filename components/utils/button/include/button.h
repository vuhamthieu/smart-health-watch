#pragma once

#include "driver/gpio.h"

typedef enum {
    BUTTON_SELECT,   // Chọn mục / xác nhận
    BUTTON_BACK,     // Quay lại
    BUTTON_UP,       // Di chuyển lên
    BUTTON_DOWN,     // Di chuyển xuống
    BUTTON_COUNT
} button_id_t;

typedef void (*button_callback_t)(button_id_t btn);

void button_init(button_callback_t cb);