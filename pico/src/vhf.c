#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "uart_tx.pio.h"
#include "vhf.h"

// VHF VARS [START] -----------------------------------------------------
// VHF control pins
const uint vhfPowerLevelPin = 15;
const uint vhfPttPin = 16;
const uint vhfSleepPin = 14;
const uint vhfTxPin = 13;
const uint vhfRxPin = 12;
// VHF control params
const uint vhfTimeout = 500;
const uint vhfEnableDelay = 1000;

// VHF Output Pins
const uint out0Pin = 18;
const uint out1Pin = 19;
const uint out2Pin = 20;
const uint out3Pin = 21;
const uint out4Pin = 22;
const uint out5Pin = 23;
const uint out6Pin = 24;
const uint out7Pin = 25;

const uint32_t vhfPinMask = 0x3FC0000; // bits 18-25 are 1, everything else 0
const uint vhfPinShift = 18; // left shift values 18 bits to align properly
// VHF VARS [END] -------------------------------------------------------

// VHF HEADERS [START] --------------------------------------------------
void initializeOutput(void) {
  gpio_init_mask(vhfPinMask);
  gpio_set_dir_masked(vhfPinMask,vhfPinMask);
}

void setOutput(uint8_t state) {
  // Use pin mask for added security on gpio
  gpio_put_masked(vhfPinMask, (state << vhfPinShift) & vhfPinMask);
}

void initializeDra818v(bool highPower) {
  gpio_init(vhfPttPin); gpio_set_dir(vhfPttPin, GPIO_OUT);
  gpio_put(vhfPttPin, false);
  gpio_init(vhfSleepPin); gpio_set_dir(vhfSleepPin, GPIO_OUT);
  gpio_put(vhfSleepPin, true);

  gpio_init(vhfPowerLevelPin);
  if (highPower) {
    gpio_set_dir(vhfPowerLevelPin, GPIO_IN);
  }
  else {
    gpio_set_dir(vhfPowerLevelPin, GPIO_OUT);
    gpio_put(vhfPowerLevelPin, 0);
  }

  sleep_ms(vhfEnableDelay);
}
bool configureDra818v(float txFrequency, float rxFrequency, bool emphasis, bool hpf, bool lpf) {
  // char temp[100];
  PIO pio = pio0;
  uint sm = pio_claim_unused_sm(pio, true);
  uint offset = pio_add_program(pio, &uart_tx_program);
  uart_tx_program_init(pio, sm, offset, vhfTxPin, 9600);
  uart_tx_program_puts(pio, sm, "AT+DMOCONNECT");
  sleep_ms(vhfEnableDelay);
  uart_tx_program_puts(pio, sm, "AT+DMOSETGROUP=0,144.39,144.39,0000,0,0000\n");
  sleep_ms(vhfEnableDelay);
  // sprintf(temp, "AT+SETFILTER=%d,%d,%d\n",emphasis,hpf,lpf);
  uart_tx_program_puts(pio, sm, "AT+SETFILTER=0,0,0\n");
  sleep_ms(vhfEnableDelay);
  return true;
}

void setPttState(bool state) {gpio_put(vhfPttPin, state);}
void setVhfState(bool state) {gpio_put(vhfSleepPin, state);}

void configureVHF(void) {
    sleep_ms(10000);
    printf("Configuring DRA818V...\n");
    initializeDra818v(true);
    configureDra818v(144.39,144.39,false,false,false);
    setPttState(false);
    setVhfState(true);
    printf("DRA818V configured.\n");

    printf("Configuring DAC...\n");
    initializeOutput();
    printf("DAC configured.\n");
}
// VHF HEADERS [END] ----------------------------------------------------

void pinDescribe(void) {
  bi_decl(bi_1pin_with_name(vhfPowerLevelPin, "VHF Module Power Level Pin"));
  bi_decl(bi_1pin_with_name(vhfPttPin, "VHF Module PTT Pin"));
  bi_decl(bi_1pin_with_name(vhfSleepPin, "VHF Module Sleep Pin"));
  bi_decl(bi_1pin_with_name(vhfTxPin, "VHF Module TX Pin for sPIO"));
  bi_decl(bi_1pin_with_name(vhfRxPin, "VHF Module RX Pin for sPIO"));
  bi_decl(bi_1pin_with_name(out0Pin, "DAC Pin 0"));
  bi_decl(bi_1pin_with_name(out1Pin, "DAC Pin 1"));
  bi_decl(bi_1pin_with_name(out2Pin, "DAC Pin 2"));
  bi_decl(bi_1pin_with_name(out3Pin, "DAC Pin 3"));
  bi_decl(bi_1pin_with_name(out4Pin, "DAC Pin 4"));
  bi_decl(bi_1pin_with_name(out5Pin, "DAC Pin 5"));
  bi_decl(bi_1pin_with_name(out6Pin, "DAC Pin 6"));
  bi_decl(bi_1pin_with_name(out7Pin, "DAC Pin 7"));
}
