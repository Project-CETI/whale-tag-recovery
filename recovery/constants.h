#ifndef _RECOVERY_CONSTANTS_H_
#define _RECOVERY_CONSTANTS_H_

#include "aprs.h"
#include "gps.h"
#include "stdbool.h"
#include "stdint.h"
#include "tag.h"

// Callsign and SSIS configuration
#define CALLSIGN "J75Y"
#define SSID 3
#define DEFAULT_FREQ 144.39
#define DEFAULT_LAT 15.31383
#define DEFAULT_LON -61.30075

// Pre-processor flags
/// Number of values in the sinValues DAC output array
#define NUM_SINS 32

#define MAX_TAG_MSG_LEN 20

static const uint32_t DAY_SLEEP =
    10000;  // 1200000;        // 20 minutes in milliseconds
static const uint32_t NIGHT_SLEEP =
    30000;  // 3600000;      // 60 minutes in milliseconds
static const uint32_t GEOFENCE_SLEEP =
    1000;  // 72000000;  // 20 hours in milliseconds
static const uint32_t FLOATER_VARIANCE = 120000;  // 2 minutes in milliseconds
static const uint32_t TAG_VARIANCE = 120000;      // 2 minutes in milliseconds
static const uint32_t TESTING_VARIANCE = 30000;   // 2 minutes in milliseconds
static const uint32_t TESTING_TIME = 30000;
static const uint8_t DAY_NIGHT_ROLLOVER = 19;  // 7pm
static const uint8_t NIGHT_DAY_ROLLOVER = 5;   // 5am
static const int8_t UTC_OFFSET = -4;

static const uint8_t VHF_PIN = 6;

// Static constants
/// Location of the LED pin
static const uint8_t LED_PIN = 29;

// do any of these operations?
static uint32_t tMsgSent = 0;

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
static const uint32_t VHF_DACMASK = 0x3fc0000;
/** Value to left-shift DAC masks to match GPIO pinmask.
 * Using the DAC requires passing an 8-bit pinmask 0b00000000 matching pins
 * 18-25, LSB=18. To match how the RP2040 SDK sets GPIO pins via pinmasks, this
 * DAC mask needs to be left-shifted to match the pin positions. */
static const uint8_t VHF_DACSHIFT = 18;

/// Set how long the pure VHF pulse should be
static const uint16_t VHF_TX_LEN = 800;

// VHF control pins
static const uint8_t VHF_POWER_LEVEL =
    15;                               ///< Defines the VHF's power level pin.
static const uint8_t VHF_PTT = 16;    ///< Defines the VHF's sleep pin.
static const uint8_t VHF_SLEEP = 14;  ///< Defines the VHF's sleep pin.
static const uint8_t VHF_TX =
    12;  ///< Defines the VHF's UART TX pin (for configuration).
static const uint8_t VHF_RX =
    13;  ///< Defines the VHF's UART RX pin (for configuration).

/// Defines the delay between each VHF configuration step.
static const uint32_t vhfEnableDelay = 1000;
/** Array of sin values for the DAC output.
 * Corresponds to one whole 8-bit sine output.
 */
static const uint8_t sinValues[NUM_SINS] = {
    152, 176, 198, 217, 233, 245, 252, 255, 252, 245, 233,
    217, 198, 176, 152, 127, 103, 79,  57,  38,  22,  10,
    3,   1,   3,   10,  22,  38,  57,  79,  103, 128};
    
// static const uint8_t sinValues[NUM_SINS] = {
//     128, 152, 176, 198, 218, 234, 245, 253, 255, 253, 245,
//     234, 218, 198, 176, 152, 128, 103, 79,  57,  37,  21,
//     10,  2,   0,   2,   10,  21,  37,  57,  79,  103};

#endif  // _RECOVERY_CONSTANTS_H_
