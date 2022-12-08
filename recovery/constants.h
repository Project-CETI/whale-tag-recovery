#ifndef _RECOVERY_CONSTANTS_H_
#define _RECOVERY_CONSTANTS_H_

#include "aprs/aprs.h"
#include "aprs/aprs_sleep.h"
#include "gps/gps.h"
#include "stdbool.h"
#include "stdint.h"
#include "tag.h"

// Callsign and SSIS configuration
#define DEFAULT_LAT 15.31383
#define DEFAULT_LON -61.30075

// Pre-processor flags
/// Number of values in the sinValues DAC output array
#define NUM_SINES 32

#define MAX_TAG_MSG_LEN 20

typedef struct clock_config_t {
    uint scb_orig;
    uint clock0_orig;
    uint clock1_orig;
} clock_config_s;

static clock_config_s clock_config;

static const char DEFAULT_FREQ[9] = "144.3900";
static const char DOMINICA_FREQ[9] = "145.0500";
static const char LOCALIZATION_FREQ[9] = "149.0000";

static const uint32_t aprs_timing_startup = 20000;  // in milliseconds
static const uint16_t aprs_timing_delay = 10000;

// BUSY_SLEEP = sleep_ms/busy wait sleep
// DAY_NIGHT_SLEEP = sleep with ability to wake up on GPS trigger, but
// alternates between different timing during the day vs night for sleeping
// DEAD_SLEEP = sleep until woken up with power cycle
enum SleepType {
    BUSY_SLEEP = 0,
    DAY_NIGHT_SLEEP = 1,
    DEAD_SLEEP = 2,
    TAG_SLEEP = 3
};
// Static constants
/// Location of the LED pin
static const float VIN_SLEEP_LIMIT = 0.75;
static const uint8_t MAX_APRS_TX_ATTEMPTS = 3;

// reading/writing from/to tag
static char tag_rd_buf[MAX_TAG_MSG_LEN];
static const uint32_t ackWaitT_us = 1000;
static const uint32_t ackWaitT_ms = 1000;
static const int numTries = 3;

// VHF configuration //
static const uint16_t VHF_WAKE_TIME_MS = 1000;
// Frequency of tranmission pulse
static const uint16_t VHF_HZ = 440;
/** @brief GPIO pinmask used to drive input to the VHF module.
 * The dra818v module modulates an analog input with the carrier frequency
 * defined during the latest configuration. The recovery board simulates that
 * input using a resistor ladder DAC, and drives that ladder using a set of GPIO
 * pins.
 *
 * The pinmask is the value 0b00000011111111000000000000000000, which represents
 * which of the GPIO pins 0-31 are to be used. We are using GPIO pins 18-25 for
 * the DAC. */
static const uint32_t VHF_DACMASK = 0xFF;  // 0x3fc0000;
/** Value to left-shift DAC masks to match GPIO pinmask.
 * Using the DAC requires passing an 8-bit pinmask 0b00000000 matching pins
 * 18-25, LSB=18 MSB=25. To match how the RP2040 SDK sets GPIO pins via
 * pinmasks, this DAC mask needs to be left-shifted to match the pin positions.
 */
static const uint8_t VHF_DACSHIFT = 0;  // 18;

/// Set how long the pure VHF pulse should be
static const uint16_t VHF_TX_LEN = 800;

// VHF control pins
// Defines the VHF's power level pin.
static const uint8_t VHF_POWER_LEVEL = 15;
// Defines the VHF's push to talk pin.
static const uint8_t VHF_PTT = 16;
static const uint8_t VHF_SLEEP = 14;  // Defines the VHF's sleep pin.
// Defines the VHF's UART TX pin (for configuration).
static const uint8_t VHF_TX = 12;
// Defines the VHF's UART RX pin (for configuration).
static const uint8_t VHF_RX = 13;
static const uint8_t APRS_LED_PIN = 29;
static const uint8_t GPS_TX_PIN = 19;
static const uint8_t GPS_RX_PIN = 18;

static const uint8_t TAG_TX_PIN = 23;
static const uint8_t TAG_RX_PIN = 22;

/// Defines the delay between each VHF configuration step.
static const uint32_t vhfEnableDelay = 1000;
/** Array of sine values for the DAC output.
 * Corresponds to one whole 8-bit sine output.
 *
 * User provided, in a readable form. Then gets remasked into the proper form to
 * match the DAC output mask
 */
static const uint8_t sineValues[NUM_SINES] = {
    152, 176, 198, 217, 233, 245, 252, 255, 252, 245, 233,
    217, 198, 176, 152, 127, 103, 79,  57,  38,  22,  10,
    3,   1,   3,   10,  22,  38,  57,  79,  103, 128};

static uint8_t remaskedValues[NUM_SINES] = {0};

// static const uint8_t sinValues[NUM_SINS] = {
//     63, 75, 87, 98, 108, 116, 122, 125, 127, 125, 122, 116, 108, 98, 87, 75,
//     63, 51, 39, 28, 18,  10,  4,   1,   0,   1,   4,   10,  18,  28, 39, 51};

// Static helper util functions
static void setLed(uint8_t pin, bool state) { gpio_put(pin, state); }

static void initLed(uint8_t pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
}

/**
 * Need to reorder the DAC input values to match the weird DAC mask.
 * The pin values go 0, 2, 4, 6, 1, 3, 5, 7 instead of just 0-7 like on the old
 * board versions, so run this function once as part of the setup to take the
 * user readable sine wave input and convert it to its new form to be fed into
 * the DAC correctly.
 *
 * This function uses the static const @param sineValues as the original sine
 * values and then also uses the static @param remaskedValues as the output of
 * the sine values restructured to match the DAC pins:
 *
 * [7 5 3 1 6 4 2 0]
 * [7 6 5 4 3 2 1 0]
 *
 * 0 = LSB
 * 7 = MSB
 */

static void remaskDAC() {
    for (uint8_t index = 0; index < NUM_SINES; index++) {
        uint8_t val = sineValues[index];
        uint8_t remasked = ((val & 0x02) << 3) | ((val & 0x04) >> 1) |
                           ((val & 0x08) << 2) | ((val & 0x10) >> 2) |
                           ((val & 0x20) << 1) | ((val & 0x40) >> 3) |
                           (val & 0x80);
        remaskedValues[index] = remasked;
    }
}

#endif  // _RECOVERY_CONSTANTS_H_
