#include "driver/rmt.h"
#include "esp_log.h"

#define RMT_CHANNEL    RMT_CHANNEL_0
#define GPIO_OUT       18
#define CLK_DIV        1           // 80 MHz base clock
#define TICK_US        (1.0 / 80)  // 12.5 ns per tick
#define PERIOD_US      10          // 10 µs period = 100 kHz
#define PERIOD_TICKS   (uint16_t)(PERIOD_US / TICK_US)  // ~800 ticks

void app_main(void)
{
    rmt_config_t config = {
        .rmt_mode = RMT_MODE_TX,
        .channel = RMT_CHANNEL,
        .gpio_num = GPIO_OUT,
        .clk_div = CLK_DIV,
        .mem_block_num = 1,
        .tx_config = {
            .loop_en = false,
            .carrier_en = false,
            .idle_level = RMT_IDLE_LEVEL_LOW,
            .idle_output_en = true
        }
    };
    rmt_config(&config);
    rmt_driver_install(RMT_CHANNEL, 0, 0);

    rmt_item32_t item;

    while (1) {
        // 70% duty (nominal)
        uint16_t high_ticks = (uint16_t)(PERIOD_TICKS * 0.7);
        uint16_t low_ticks  = PERIOD_TICKS - high_ticks;
        item.duration0 = high_ticks;
        item.level0 = 1;
        item.duration1 = low_ticks;
        item.level1 = 0;
        rmt_write_items(RMT_CHANNEL, &item, 1, true);

        // simulate a phase shift modulation: move falling edge ±10%
        high_ticks = (uint16_t)(PERIOD_TICKS * 0.6); // phase advanced
        low_ticks  = PERIOD_TICKS - high_ticks;
        item.duration0 = high_ticks;
        item.level0 = 1;
        item.duration1 = low_ticks;
        item.level1 = 0;
        rmt_write_items(RMT_CHANNEL, &item, 1, true);

        high_ticks = (uint16_t)(PERIOD_TICKS * 0.8); // phase delayed
        low_ticks  = PERIOD_TICKS - high_ticks;
        item.duration0 = high_ticks;
        item.level0 = 1;
        item.duration1 = low_ticks;
        item.level1 = 0;
        rmt_write_items(RMT_CHANNEL, &item, 1, true);
    }
}
