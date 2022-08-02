/** @file vhf.c
 * @brief Implements low-level VHF operation for the recovery board.
 * @author Shashank Swaminathan
 * @author Louis Adamian
 * @details Implementation is defined on operating the dra818v VHF module, but is theoretically extensible to any VHF module, provided proper mapping of the pinout.
 */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "uart_tx.pio.h"
#include "vhf.h"

// VHF VARS [START] -----------------------------------------------------
// VHF control pins
#define VHF_POWER_LEVEL 15 ///< Defines the VHF's power level pin.
#define VHF_PTT 16 ///< Defines the VHF's sleep pin.
#define VHF_SLEEP 14 ///< Defines the VHF's sleep pin.
#define VHF_TX 12 ///< Defines the VHF's UART TX pin (for configuration).
#define VHF_RX 13 ///< Defines the VHF's UART RX pin (for configuration).
// VHF control params
/// Defines the delay between each VHF configuration step.
const uint vhfEnableDelay = 1000;
/** @brief GPIO pinmask used to drive input to the VHF module.
 * The dra818v module modulates an analog input with the carrier frequency defined during the latest configuration. The recovery board simulates that input using a resistor ladder DAC, and drives that ladder using a set of GPIO pins.
 *
 * The pinmask is the value 0b00000011111111000000000000000000, which represents which of the GPIO pins 0-31 are to be used.
 * We are using GPIO pins 18-25 for the DAC. */
#define VHF_DACMASK 0x3fc0000
/** Value to left-shift DAC masks to match GPIO pinmask.
 * Using the DAC requires passing an 8-bit pinmask 0b00000000 matching pins 18-25, LSB=18.
 * To match how the RP2040 SDK sets GPIO pins via pinmasks, this DAC mask needs to be left-shifted to match the pin positions. */
#define VHF_DACSHIFT 18
// VHF VARS [END] -------------------------------------------------------

// VHF HEADERS [START] --------------------------------------------------
/** Initializes the GPIO pins that drive the VHF input's DAC.
 * Initializes via pinmask using the @link #VHF_DACMASK DAC pinmask @endlink.
 */
void initializeOutput(void) {
  gpio_init_mask(VHF_DACMASK);
  gpio_set_dir_masked(VHF_DACMASK,VHF_DACMASK);
}

/** Sets the DAC output to drive the VHF input.
 * Bitwise ANDs the input left-shifted by #VHF_DACSHIFT, and #VHF_DACMASK
 *
 * Uses the AND output as a pinmask to drive the GPIO pins.
 * @param state 8-bit value. Each bit drives a pin of the DAC, 0->18, 1->19, etc.
 */
void setOutput(uint8_t state) {
  // Use pin mask for added security on gpio
  gpio_put_masked(VHF_DACMASK, (state << VHF_DACSHIFT) & VHF_DACMASK);
}

/** Initializes the dra818v VHF module.
 * Sets the modes of the GPIO pins connected to the VHF module.
 * These are #VHF_PTT, #VHF_SLEEP, #VHF_POWER_LEVEL
 *
 * Waits #vhfEnableDelay after initialization.
 * @param highPower Defines if the module is operating at high power or not.
 */
void initializeDra818v(bool highPower) {
  gpio_init(VHF_PTT); gpio_set_dir(VHF_PTT, GPIO_OUT);
  gpio_put(VHF_PTT, false);
  gpio_init(VHF_SLEEP); gpio_set_dir(VHF_SLEEP, GPIO_OUT);
  gpio_put(VHF_SLEEP, true);

  gpio_init(VHF_POWER_LEVEL);
  if (highPower) {
    gpio_set_dir(VHF_POWER_LEVEL, GPIO_IN);
  }
  else {
    gpio_set_dir(VHF_POWER_LEVEL, GPIO_OUT);
    gpio_put(VHF_POWER_LEVEL, false);
  }

  sleep_ms(vhfEnableDelay);
}

/** Configures the dra818v VHF module.
 * Sends configuration strings over UART. Waits #vhfEnableDelay after each string.
 * The module can be asked to reconfigure at any time; to avoid conflict over uart channels, PIO is used to simulate a UART channel.
 * This uses pio0.
 *
 * Sends four strings: init handshake, group setting, volume, setfilter. Each string must end with a <cr><lf>.
 *
 * See http://www.dorji.com/docs/data/DRA818V.pdf for more information about configuration.
 */
void configureDra818v(float txFrequency, float rxFrequency, uint8_t volume, bool emphasis, bool hpf, bool lpf) {
	// Setup PIO UART
  PIO pio = pio0;
  uint sm = pio_claim_unused_sm(pio, true);
  uint offset = pio_add_program(pio, &uart_tx_program);
  uart_tx_program_init(pio, sm, offset, VHF_TX, 9600);
	// Configuration initialization handshake
  uart_tx_program_puts(pio, sm, "AT+DMOCONNECT\r\n");
  sleep_ms(vhfEnableDelay);
	char temp[100];
	// Set group parameters - TX and RX frequencies, TX CTCSS, squelch, RX CTCSS
	sprintf(temp, "AT+DMOSETGROUP=0,%.04f,%.04f,0000,0,0000\r\n",
					txFrequency, rxFrequency);
  uart_tx_program_puts(pio, sm, temp);
  sleep_ms(vhfEnableDelay);
	// Set volume (1~8)
	if (volume<1 || volume >8) volume=4; // Default to intermediate volume.
	sprintf(temp, "AT+DMOSETVOLUME=%d\r\n",volume);
	uart_tx_program_puts(pio, sm, temp);
  sleep_ms(vhfEnableDelay);
	// Set filters
  sprintf(temp, "AT+SETFILTER=%u,%u,%u\r\n",emphasis,hpf,lpf);
  uart_tx_program_puts(pio, sm, temp);
  sleep_ms(vhfEnableDelay);
	// Clean up
  pio_remove_program(pio, &uart_tx_program, offset);
  pio_sm_unclaim(pio, sm);
  pio_clear_instruction_memory(pio);
	memset(temp, 0, sizeof(temp));
}

/*
At some point this setup needs to be better.

Splitting into two separate functions because I don't want to deal with sprintf

bool configureDra818v14505(float txFrequency, float rxFrequency, bool emphasis, bool hpf, bool lpf) {
  // char temp[100];
  PIO pio = pio0;
  uint sm = pio_claim_unused_sm(pio, true);
  uint offset = pio_add_program(pio, &uart_tx_program);
  uart_tx_program_init(pio, sm, offset, VHF_TX, 9600);
  uart_tx_program_puts(pio, sm, "AT+DMOCONNECT\r\n");
  busy_wait_ms(vhfEnableDelay);
  uart_tx_program_puts(pio, sm, "AT+DMOSETGROUP=0,145.0500,145.0500,0000,0,0000\r\n");
  busy_wait_ms(vhfEnableDelay);
  // sprintf(temp, "AT+SETFILTER=%d,%d,%d\n",emphasis,hpf,lpf);
  uart_tx_program_puts(pio, sm, "AT+SETFILTER=0,0,0\r\n");
  busy_wait_ms(vhfEnableDelay);
  pio_remove_program(pio, &uart_tx_program, offset);
  pio_sm_unclaim(pio, sm);
  pio_clear_instruction_memory(pio);
  return true;
}

bool configureDra818v14439(float txFrequency, float rxFrequency, bool emphasis, bool hpf, bool lpf) {
  // char temp[100];
  PIO pio = pio0;
  uint sm = pio_claim_unused_sm(pio, true);
  uint offset = pio_add_program(pio, &uart_tx_program);
  uart_tx_program_init(pio, sm, offset, VHF_TX, 9600);
  uart_tx_program_puts(pio, sm, "AT+DMOCONNECT\r\n");
  busy_wait_ms(vhfEnableDelay);
  uart_tx_program_puts(pio, sm, "AT+DMOSETGROUP=0,144.3900,144.3900,0000,0,0000\r\n");
  busy_wait_ms(vhfEnableDelay);
  // sprintf(temp, "AT+SETFILTER=%d,%d,%d\n",emphasis,hpf,lpf);
  uart_tx_program_puts(pio, sm, "AT+SETFILTER=0,0,0\r\n");
  busy_wait_ms(vhfEnableDelay);
  pio_remove_program(pio, &uart_tx_program, offset);
  pio_sm_unclaim(pio, sm);
  pio_clear_instruction_memory(pio);
  return true;
}
*/

/** Sets the VHF module's PTT state.
 * @param state Boolean value; if true, PTT is enabled. */
void setPttState(bool state) {gpio_put(VHF_PTT, state);}
/** Sets the VHF module's sleep state.
 * @param state Boolean value; if true, the module sleeps. */
void setVhfState(bool state) {gpio_put(VHF_SLEEP, state);}

/** Completely configures the VHF for initialization.
 * Used for initial setup of the VHF module after power on.
 * Runs the following functions in order: initializeDra818v, configureDra818v, initializeOutput. */
void configureVHF(void) {
    sleep_ms(10000);
    // printf("Configuring DRA818V...\n");
    initializeDra818v(true);
    configureDra818v(144.39,144.39,8,false,false,false);
    setPttState(false);
    setVhfState(true);
    // printf("DRA818V configured.\n");

    // printf("Configuring DAC...\n");
    initializeOutput();
    // printf("DAC configured.\n");
}
// VHF HEADERS [END] ----------------------------------------------------

/** Adds VHF pinout information to the compiled binary. */
void pinDescribe(void) {
  bi_decl(bi_1pin_with_name(VHF_POWER_LEVEL, "VHF Module Power Level Pin"));
  bi_decl(bi_1pin_with_name(VHF_PTT, "VHF Module PTT Pin"));
  bi_decl(bi_1pin_with_name(VHF_SLEEP, "VHF Module Sleep Pin"));
  bi_decl(bi_1pin_with_name(VHF_TX, "VHF Module TX Pin for sPIO"));
  bi_decl(bi_1pin_with_name(VHF_RX, "VHF Module RX Pin for sPIO"));
  bi_decl(bi_pin_mask_with_name(VHF_DACMASK, "DAC Pins"));
}
