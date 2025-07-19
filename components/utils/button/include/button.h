#pragma once

#include "driver/gpio.h"

typedef enum {
    BUTTON_SELECT,   
    BUTTON_BACK,   
    BUTTON_UP,      
    BUTTON_DOWN,     
    BUTTON_COUNT
} button_id_t;

typedef void (*button_callback_t)(button_id_t btn);

void button_init(button_callback_t cb);