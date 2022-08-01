#include "tag.h"
#include "hardware/gpio.h"
#include "pico/time.h"
// TAG DEFS [START] ---------------------------------------------------
#define MAX_TAG_MSG_LEN 20
// TAG DEFS [END] -----------------------------------------------------

// TAG VARS [START] -----------------------------------------------------
// do any of these operations?
uart_inst_t *uartTag;
// reading/writing from/to tag
char tag_rd_buf[MAX_TAG_MSG_LEN];
uint32_t tMsgSent = 0;
uint32_t ackWaitT_us= 1000;
uint32_t ackWaitT_ms= 1000;
int numTries = 3;
bool writeDataToTag = false;
// TAG VARS [END] -------------------------------------------------------

// TAG FUNCTIONS [START] ------------------------------------------------
int parseTagStatus(void) {
  return 0;
}

void writeGpsToTag(char *lastGps, char *lastDt) {
  uart_putc(uartTag, 'g');
  uart_puts(uartTag, lastGps);
  uart_puts(uartTag, lastDt);
  uart_putc(uartTag, '\n');
}

void detachTag(void) {
  tMsgSent = to_ms_since_boot(get_absolute_time());
  while(to_ms_since_boot(get_absolute_time()) - tMsgSent < ackWaitT_ms) {
    if (!uart_is_writable(uartTag)) uart_tx_wait_blocking(uartTag);
    uart_putc(uartTag, 'D');
    uart_putc(uartTag, '\n');
    if (uart_is_readable_within_us(uartTag, ackWaitT_us)) {
      uart_read_blocking(uartTag, tag_rd_buf, 1);
      if (tag_rd_buf[0] == 'd') break;
    }
    busy_wait_ms(ackWaitT_ms/numTries);
  }
}

void reqTagState(void) {
  tMsgSent = to_ms_since_boot(get_absolute_time());
  uart_putc(uartTag, 'T');
  uart_putc(uartTag, '\n');
  
  while (to_ms_since_boot(get_absolute_time()) - tMsgSent < ackWaitT_ms) {
    if (uart_is_readable_within_us(uartTag, ackWaitT_us)) {
      uart_read_blocking(uartTag, tag_rd_buf, 5);
      parseTagStatus();
      break;
    }
  }
}

// Init functions
void initTagComm(int txPin, int rxPin, int baudrate, uart_inst_t *uartNum) {
  uartTag = uartNum;
  uart_init(uartTag, baudrate);
  gpio_set_function(txPin, GPIO_FUNC_UART);
  gpio_set_function(rxPin, GPIO_FUNC_UART);
}
// TAG FUNCTIONS [END] --------------------------------------------------
