#include "driver/rmt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RMT_CHANNEL    RMT_CHANNEL_0
#define GPIO_OUT       8
#define CLK_DIV        80
#define HIGH_US        500 * 5000      // HIGH duration = 500 µs
#define LOW0_US        250 * 5000      // LOW for bit 0 = 250 µs
#define LOW1_US        750 * 5000      // LOW for bit 1 = 750 µs

// Fill one RMT item for a bit
void fill_psk_bit(rmt_item32_t* item, int bit) {
    item->level0 = 1;
    item->duration0 = HIGH_US;
    item->level1 = 0;
    item->duration1 = (bit) ? LOW1_US : LOW0_US;
}

// Send a string using this PSK-style encoding
void send_psk_string(char * data, size_t len) 
{
    rmt_item32_t items[len * 8];  // 8 bits per char

    int idx = 0;
    for (size_t i = 0; i < len; i++) {
        for (int b = 7; b >= 0; b--) {  // MSB first
            int bit = (data[i] >> b) & 1;
            fill_psk_bit(&items[idx], bit);
            idx++;
        }
    }

    rmt_write_items(RMT_CHANNEL, items, idx, true); // blocking for simplicity
}

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

    char str[] = {0b10100100};
    while (1) {
        send_psk_string(str, sizeof(str));
        vTaskDelay(pdMS_TO_TICKS(1000)); // wait 1s between sends
    }
}
