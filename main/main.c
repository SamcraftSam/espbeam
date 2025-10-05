#include "driver/rmt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define RMT_CHANNEL    RMT_CHANNEL_2
#define GPIO_OUT       8
#define CLK_DIV        80
#define HIGH_US        500 * 5000      // HIGH duration = 500 µs
#define LOW0_US        250 * 5000      // LOW for bit 0 = 250 µs
#define LOW1_US        750 * 5000      // LOW for bit 1 = 750 µs
#define MEM_BLOCK_NUM      1

//CONFIG
#define USE_TX 0 // 1 tx, 0 rx

#define TICKS(us) ((us) * 80 / CLK_DIV)
#define HIGH_TICKS TICKS(HIGH_US)
#define LOW0_TICKS TICKS(LOW0_US)
#define LOW1_TICKS TICKS(LOW1_US)
#define BIT_THRESHOLD_TICKS ((LOW0_TICKS + LOW1_TICKS)/2)

#define IDLE_THRESHOLD_US 2000// Fill one RMT item for a bit

// ===========================================
#if USE_TX

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

static void tx_task(void* arg) {
    char str[] = {0b10100100};
    while (1) {
        send_psk_string(str, sizeof(str));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#else
// ---------------- RX Functions ----------------
int decode_psk_bit(rmt_item32_t item) {
    if (item.duration1 < BIT_THRESHOLD_TICKS) return 0;
    else return 1;
}

static void rx_task(void* arg) {
    RingbufHandle_t rb = NULL;
    rmt_get_ringbuf_handle(RMT_CHANNEL, &rb);
    rmt_rx_start(RMT_CHANNEL, true);

    uint8_t byte_acc = 0;
    int bit_count = 0;

    while (1) {
        size_t rx_size = 0;
        rmt_item32_t* items = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, pdMS_TO_TICKS(100));
        if (items) {
            size_t num_items = rx_size / sizeof(rmt_item32_t);
            for (size_t i = 0; i < num_items; i++) {
                int bit = decode_psk_bit(items[i]);
                byte_acc = (byte_acc << 1) | bit;
                bit_count++;

                if (bit_count == 8) {
                    ESP_LOGI("TIME", "Received byte: 0x%02X\n", byte_acc);
                    byte_acc = 0;
                    bit_count = 0;
                }
            }
            vRingbufferReturnItem(rb, (void*) items);
        }
    }
}
#endif

void app_main(void)
{   
    gpio_reset_pin(3);
    gpio_set_direction(3, GPIO_MODE_INPUT);
    rmt_config_t rmt_conf = {
        .channel = RMT_CHANNEL,
        .clk_div = CLK_DIV,
        .mem_block_num = MEM_BLOCK_NUM
    };

#if USE_TX
    rmt_conf.rmt_mode = RMT_MODE_TX;
    rmt_conf.gpio_num = GPIO_OUT,
    rmt_conf.tx_config.loop_en = false;
    rmt_conf.tx_config.carrier_en = false;
    rmt_conf.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    rmt_conf.tx_config.idle_output_en = true;
#else
    rmt_conf.rmt_mode = RMT_MODE_RX;
    rmt_conf.gpio_num = 3;
    rmt_conf.rx_config.filter_en = false;
    rmt_conf.rx_config.filter_ticks_thresh = 0;
    rmt_conf.rx_config.idle_threshold = TICKS(IDLE_THRESHOLD_US);
#endif

    rmt_config(&rmt_conf);
    rmt_driver_install(RMT_CHANNEL, 1000, 0);

#if USE_TX
    xTaskCreate(tx_task, "tx_task", 4096, NULL, 10, NULL);
#else
    xTaskCreate(rx_task, "rx_task", 4096, NULL, 10, NULL);
#endif
}
