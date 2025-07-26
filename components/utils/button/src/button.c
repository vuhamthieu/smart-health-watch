#include "button.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#define TAG "BUTTON"

#define BUTTON_SELECT_GPIO  GPIO_NUM_14
#define BUTTON_BACK_GPIO    GPIO_NUM_15 
#define BUTTON_UP_GPIO      GPIO_NUM_13
#define BUTTON_DOWN_GPIO    GPIO_NUM_12
#define DEBOUNCE_TIME_MS    50

static QueueHandle_t gpio_evt_queue = NULL;
static button_callback_t user_callback = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg) {
    button_id_t btn_id = (button_id_t)(uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &btn_id, NULL);
}

static void button_task(void *arg) {
    button_id_t btn_id;
    static TickType_t last_press_time[BUTTON_COUNT] = {0};

    while (1) {
        if (xQueueReceive(gpio_evt_queue, &btn_id, portMAX_DELAY)) {
            TickType_t current_time = xTaskGetTickCount();
            if ((current_time - last_press_time[btn_id]) > pdMS_TO_TICKS(DEBOUNCE_TIME_MS)) {
                if (user_callback) {
                    user_callback(btn_id);
                }
                last_press_time[btn_id] = current_time;
            }
        }
    }
}

void button_init(button_callback_t cb) {
    user_callback = cb;

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pin_bit_mask = (1ULL << BUTTON_SELECT_GPIO) |
                        (1ULL << BUTTON_BACK_GPIO) |
                        (1ULL << BUTTON_UP_GPIO) |
                        (1ULL << BUTTON_DOWN_GPIO)
    };
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(button_id_t));
    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_SELECT_GPIO, gpio_isr_handler, (void *)BUTTON_SELECT);
    gpio_isr_handler_add(BUTTON_BACK_GPIO, gpio_isr_handler, (void *)BUTTON_BACK);
    gpio_isr_handler_add(BUTTON_UP_GPIO, gpio_isr_handler, (void *)BUTTON_UP);
    gpio_isr_handler_add(BUTTON_DOWN_GPIO, gpio_isr_handler, (void *)BUTTON_DOWN);

    ESP_LOGI(TAG, "Button initialized");
}