#ifndef _RECOVERY_CONSTANTS_H_
#define _RECOVERY_CONSTANTS_H_

#include "aprs/aprs.h"
#include "gps/gps.h"
#include "stdbool.h"
#include "stdint.h"
#include "tag.h"
#include "aprs/aprs_sleep.h"

// Callsign and SSIS configuration
#define DEFAULT_LAT 15.31383
#define DEFAULT_LON -61.30075

// Pre-processor flags
/// Number of values in the sinValues DAC output array
#define NUM_SINS 32

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
enum SleepType { BUSY_SLEEP = 0, DAY_NIGHT_SLEEP = 1, DEAD_SLEEP = 2, TAG_SLEEP = 3 };
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
 * The pinmask is the value 0b11111111, which represents
 * which of the GPIO pins 0-31 are to be used. We are using GPIO pins 0-7 for
 * the DAC. */

static const uint32_t VHF_DACMASK = 0xFF;

// Set how long the pure VHF pulse should be
static const uint16_t VHF_TX_LEN = 800;

// VHF control pins
static const uint8_t VHF_POWER_LEVEL =
    15;                               ///< Defines the VHF's power level pin.
static const uint8_t VHF_PTT = 16;    ///< Defines the VHF's sleep pin.
static const uint8_t VHF_SLEEP = 14;  ///< Defines the VHF's sleep pin.
static const uint8_t VHF_TX =
    10;  ///< Defines the VHF's UART TX pin (for configuration).
static const uint8_t VHF_RX =
    11;  ///< Defines the VHF's UART RX pin (for configuration).
static const uint8_t APRS_LED_PIN = 29;
static const uint8_t GPS_TX_PIN = 19;
static const uint8_t GPS_RX_PIN = 18;

static const uint8_t TAG_TX_PIN = 23;
static const uint8_t TAG_RX_PIN = 22;

/// Defines the delay between each VHF configuration step.
static const uint32_t vhfEnableDelay = 1000;

/**The constant 'sinValues' is the sequence of values to be output on the 8 bit 
 * DAC to generate a sin wave. Since the DAC pins are not in 
 * order from 0-7, these values must be adjusted so that we 
 * can drive the 8 gpios simultaneously with a single 8 bit 
 * value. Based on the schemaic of the board, the values were
 * remapped so that 
 *  b0->gpio0
 *  b1->gpio4
 *  b2->gpio1
 *  b3->gpio5
 *  b4->gpio2
 *  b5->gpio6
 *  b6->gpio3
 *  b7->gpio7
 *  
 * instead of the previous 1-to-1 mapping.
 * 
 * Each value has been adjusted accordingly in 
 * 'remaskedSinValues'
 **/


// static const uint8_t sinValues[NUM_SINS] = {
//     152, 176, 198, 217, 233, 245, 252, 255, 252, 245, 233,
//     217, 198, 176, 152, 127, 103, 79,  57,  38,  22,  10,
//     3,   1,   3,   10,  22,  38,  57,  79,  103, 128};


static const uint8_t remaskedSinValues[NUM_SINS] = {
    164, 196, 154, 173, 233, 207, 238, 255, 238, 207, 233,
    173, 154, 196, 164, 127, 91, 59, 101, 82, 22, 48, 17,
    1, 17, 48, 22, 82, 101, 59, 91, 128};


#endif  // _RECOVERY_CONSTANTS_H_
