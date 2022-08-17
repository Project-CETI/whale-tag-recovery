#include "tag.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <stdio.h>

uart_inst_t *uartTag;

// TAG FUNCTIONS [START] ------------------------------------------------
int parseTagStatus(void) { return 0; }

void writeGpsToTag(const tag_config_s *tag_cfg, char *lastGps, char *lastDt) {
    printf("[TAG TX] %s\n", lastGps);
    printf("[TAG TX] %s\n", lastDt);
    // uart_putc_raw(tag_cfg->uart, testByte);
    // uint8_t i = 0;
    if (!uart_is_writable(tag_cfg->uart)) uart_tx_wait_blocking(tag_cfg->uart);
    // while (lastGps[i]) {
    //   printf("%02x ", (uint8_t)lastGps[i]);
    //    uart_putc_raw(tag_cfg->uart, (uint8_t)lastGps[i++]);
    // }
    uart_putc(tag_cfg->uart, 'g');
    uart_puts(tag_cfg->uart, lastGps);
    uart_puts(tag_cfg->uart, lastDt);
    uart_putc(tag_cfg->uart, '\n');
}

void detachTag(const tag_config_s *tag_cfg) {
    char tag_rd_buf[MAX_TAG_MSG_LEN];
    uint32_t tMsgSent = 0;
    tMsgSent = to_ms_since_boot(get_absolute_time());
    while (to_ms_since_boot(get_absolute_time()) - tMsgSent <
           tag_cfg->ackWaitT_ms) {
        if (!uart_is_writable(tag_cfg->uart))
            uart_tx_wait_blocking(tag_cfg->uart);
        uart_putc(tag_cfg->uart, 'D');
        uart_putc(tag_cfg->uart, '\n');
        if (uart_is_readable_within_us(tag_cfg->uart, tag_cfg->ackWaitT_us)) {
            uart_read_blocking(tag_cfg->uart, tag_rd_buf, 1);
            if (tag_rd_buf[0] == 'd') break;
        }
        busy_wait_ms(tag_cfg->ackWaitT_ms / tag_cfg->numTries);
    }
}

void reqTagState(const tag_config_s *tag_cfg) {
    char tag_rd_buf[MAX_TAG_MSG_LEN];
    uint32_t tMsgSent = 0;
    tMsgSent = to_ms_since_boot(get_absolute_time());
    uart_putc(tag_cfg->uart, 'T');
    uart_putc(tag_cfg->uart, '\n');
    // while (to_ms_since_boot(get_absolute_time()) - tMsgSent <
    //        tag_cfg->ackWaitT_ms) {
    if (uart_is_readable_within_us(tag_cfg->uart, tag_cfg->ackWaitT_us)) {
        uart_read_blocking(tag_cfg->uart, tag_rd_buf, MAX_TAG_MSG_LEN);
        printf("[READ TAG] %s\n", tag_rd_buf);
        // parseTagStatus(tag_rd_buf);
        // break;
    }
    // }
}

// Init functions
uint32_t initTagComm(const tag_config_s *tag_cfg) {
    uint32_t actualBaud = uart_init(tag_cfg->uart, tag_cfg->baudrate);

    gpio_set_function(tag_cfg->txPin, GPIO_FUNC_UART);
    gpio_set_function(tag_cfg->rxPin, GPIO_FUNC_UART);
    return actualBaud;
}
// TAG FUNCTIONS [END] --------------------------------------------------
