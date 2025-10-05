#include "driver/timer.h"
#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TIMER_DIVIDER     8
#define TIMER_INTERVAL    20

#define TIMER_FREQ_KHZ  (80 * 1000 * 1000/(TIMER_DIVIDER)/(TIMER_INTERVAL))

#define TIMER_GROUP_ID        TIMER_GROUP_0
#define TIMER_ID              TIMER_0

static const char *TAG = "TIMER_APP";

volatile uint32_t counter = 0;

bool IRAM_ATTR onTimer(void *args)
{
    counter++;
    if (counter > 30*000)
    {
        
        
    }
    return true;
}

void app_main()
{
    ESP_LOGI(TAG, "Starting timer example. Freq is: %u", TIMER_FREQ_KHZ);

    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_8, 1);

    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true,
        .intr_type = TIMER_INTR_LEVEL,
        .clk_src = TIMER_SRC_CLK_APB,
    };
    timer_init(TIMER_GROUP_ID, TIMER_ID, &config);
    timer_set_counter_value(TIMER_GROUP_ID, TIMER_ID, 0);
    timer_set_alarm_value(TIMER_GROUP_ID, TIMER_ID, TIMER_INTERVAL);
    timer_enable_intr(TIMER_GROUP_ID, TIMER_ID);

    timer_isr_callback_add(TIMER_GROUP_ID, TIMER_ID, onTimer, NULL, 0);

    ESP_LOGI(TAG, "Starting the timer"); 
    timer_start(TIMER_GROUP_ID, TIMER_ID);

    ESP_LOGI(TAG, "Timer was started!");
    while (1) {
        ESP_LOGI(TAG, "Timer count: %lu", counter);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
