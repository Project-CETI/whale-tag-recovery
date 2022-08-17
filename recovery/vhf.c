/** @file vhf.c
 * @brief Implements low-level VHF operation for the recovery board.
 * @author Shashank Swaminathan
 * @author Louis Adamian
 * @details Implementation is defined on operating the dra818v VHF module, but
 * is theoretically extensible to any VHF module, provided proper mapping of the
 * pinout.
 */
#include "vhf.h"
#include "hardware/pio.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "uart_tx.pio.h"
#include <stdio.h>
#include <string.h>

// VHF HEADERS [START] --------------------------------------------------
/** Initializes the GPIO pins that drive the VHF input's DAC.
 * Initializes via pinmask using the @link #VHF_DACMASK DAC pinmask @endlink.
 */
static void initializeOutput(void) {
    gpio_init_mask(VHF_DACMASK);
    gpio_set_dir_masked(VHF_DACMASK, VHF_DACMASK);
}

/** Sets the DAC output to drive the VHF input.
 * Bitwise ANDs the input left-shifted by #VHF_DACSHIFT, and #VHF_DACMASK
 *
 * Uses the AND output as a pinmask to drive the GPIO pins.
 * @param state 8-bit value. Each bit drives a pin of the DAC, 0->18, 1->19,
 * etc.
 */
void setOutput(uint8_t state) {
    // Use pin mask for added security on gpio
    gpio_put_masked(VHF_DACMASK, (state << VHF_DACSHIFT) & VHF_DACMASK);
}

/** @brief Re-configure the VHF module to tracker transmission frequency
 * @see configureDra818v */
void prepFishTx(float txFreq) {
    configureDra818v(txFreq, txFreq, 8, false, false, false);
}

/** @brief Callback for a repeating timer dictating transmissions.
 * Transmits a pulse for length defined in #YAGI_TX_LEN
 * Callback for repeating_timer functions */
bool vhf_pulse_callback(void) {
    setPttState(true);
    uint32_t stepLen = VHF_HZ * NUM_SINS;
    int numSteps = stepLen * VHF_TX_LEN / 1000;
    printf("[VHF/CB] steps: %d * %d * %d / 1000 = %d\n", VHF_HZ, NUM_SINS,
           VHF_TX_LEN, numSteps);
    stepLen = (int)1000000 / stepLen;
    for (int i = 0; i < numSteps; i++) {
        setOutput(sinValues[i % NUM_SINS]);
        busy_wait_us_32(stepLen);
    }
    setOutput(0x00);
    setPttState(false);
    return true;
}

/** Initializes the dra818v VHF module.
 * Sets the modes of the GPIO pins connected to the VHF module.
 * These are #VHF_PTT, #VHF_SLEEP, #VHF_POWER_LEVEL
 *
 * Waits #vhfEnableDelay after initialization.
 * @param highPower Defines if the module is operating at high power or not.
 */
static void initializeDra818v(bool highPower) {
    gpio_init(VHF_PTT);
    gpio_set_dir(VHF_PTT, GPIO_OUT);
    gpio_put(VHF_PTT, false);
    gpio_init(VHF_SLEEP);
    gpio_set_dir(VHF_SLEEP, GPIO_OUT);
    gpio_put(VHF_SLEEP, true);

    gpio_init(VHF_POWER_LEVEL);
    if (highPower) {
        gpio_set_dir(VHF_POWER_LEVEL, GPIO_IN);
    } else {
        gpio_set_dir(VHF_POWER_LEVEL, GPIO_OUT);
        gpio_put(VHF_POWER_LEVEL, false);
    }

    sleep_ms(vhfEnableDelay);
}

/** Configures the dra818v VHF module.
 * Sends configuration strings over UART. Waits #vhfEnableDelay after each
 * string. The module can be asked to reconfigure at any time; to avoid conflict
 * over uart channels, PIO is used to simulate a UART channel. This uses pio0.
 *
 * Sends four strings: init handshake, group setting, volume, setfilter. Each
 * string must end with a <cr><lf>.
 *
 * See http://www.dorji.com/docs/data/DRA818V.pdf for more information about
 * configuration.
 */
void configureDra818v(float txFrequency, float rxFrequency, uint8_t volume,
                      bool emphasis, bool hpf, bool lpf) {
    // Setup PIO UART
    PIO pio = pio0;
    uint sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &uart_tx_program);
    uart_tx_program_init(pio, sm, offset, VHF_TX, 9600);
    // Configuration initialization handshake
    uart_tx_program_puts(pio, sm, "AT+DMOCONNECT\r\n");
    busy_wait_ms(vhfEnableDelay);
    char temp[100];
    // Set group parameters - TX and RX frequencies, TX CTCSS, squelch, RX CTCSS
    sprintf(temp, "AT+DMOSETGROUP=0,%.04f,%.04f,0000,0,0000\r\n", txFrequency,
            rxFrequency);
    uart_tx_program_puts(pio, sm, temp);
    busy_wait_ms(vhfEnableDelay);
    // Set volume (1~8)
    if (volume < 1 || volume > 8)
        volume = 4;  // Default to intermediate volume.
    sprintf(temp, "AT+DMOSETVOLUME=%d\r\n", volume);
    uart_tx_program_puts(pio, sm, temp);
    busy_wait_ms(vhfEnableDelay);
    // Set filters
    sprintf(temp, "AT+SETFILTER=%u,%u,%u\r\n", emphasis, hpf, lpf);
    uart_tx_program_puts(pio, sm, temp);
    busy_wait_ms(vhfEnableDelay);
    // Clean up
    pio_remove_program(pio, &uart_tx_program, offset);
    pio_sm_unclaim(pio, sm);
    pio_clear_instruction_memory(pio);
    memset(temp, 0, sizeof(temp));
}

/** Sets the VHF module's PTT state.
 * @param state Boolean value; if true, PTT is enabled. */
void setPttState(bool state) { gpio_put(VHF_PTT, state); }

/** Sets the VHF module's sleep state.
 * @param state Boolean value; if true, the module is awake.
 *              if false, the module sleeps.
 *  */
void setVhfState(bool state) { gpio_put(VHF_SLEEP, state); }

void wakeVHF() {
    printf("[VHF] waking \n");
    setVhfState(true);
}

void sleepVHF() { setVhfState(false); }

/** Completely configures the VHF for initialization.
 * Used for initial setup of the VHF module after power on.
 * Runs the following functions in order: initializeDra818v, configureDra818v,
 * initializeOutput. */
void initializeVHF(void) {
    sleep_ms(600);
    // printf("Configuring DRA818V...\n");
    initializeDra818v(true);
    configureDra818v(DEFAULT_FREQ, DEFAULT_FREQ, 4, false, false, false);
    setPttState(false);
    setVhfState(true);
    //  printf("DRA818V configured.\n");

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
