#ifndef _RECOVERY_CONSTANTS_H_
#define _RECOVERY_CONSTANTS_H_

#include "stdint.h"

// Callsign and SSIS configuration
#define CALLSIGN "J73MAB"
#define SSID 1
#define DEFAULT_FREQ 144.39

/// Location of the LED pin
#define LED_PIN 29
/// Turns on communications with the main tag (only for fully CONNected tags)
#define TAG_CONNECTED 0

#define MAX_TAG_MSG_LEN 20

// do any of these operations?
static uint32_t tMsgSent = 0;

// reading/writing from/to tag
static char tag_rd_buf[MAX_TAG_MSG_LEN];
static const uint32_t ackWaitT_us = 1000;
static const uint32_t ackWaitT_ms = 1000;
static const int numTries = 3;

// VHF configuration //
#define VHF_WAKE_TIME_MS 200
// Frequency of tranmission pulse
#define VHF_HZ 440
/** @brief GPIO pinmask used to drive input to the VHF module.
 * The dra818v module modulates an analog input with the carrier frequency
 * defined during the latest configuration. The recovery board simulates that
 * input using a resistor ladder DAC, and drives that ladder using a set of GPIO
 * pins.
 *
 * The pinmask is the value 0b00000011111111000000000000000000, which represents
 * which of the GPIO pins 0-31 are to be used. We are using GPIO pins 18-25 for
 * the DAC. */
#define VHF_DACMASK 0x3fc0000
/** Value to left-shift DAC masks to match GPIO pinmask.
 * Using the DAC requires passing an 8-bit pinmask 0b00000000 matching pins
 * 18-25, LSB=18. To match how the RP2040 SDK sets GPIO pins via pinmasks, this
 * DAC mask needs to be left-shifted to match the pin positions. */
#define VHF_DACSHIFT 18
/// Number of values in the sinValues DAC output array
#define NUM_SINS 32
/// Set how long the pure VHF pulse should be
#define VHF_TX_LEN 50

// VHF control pins
#define VHF_POWER_LEVEL 15  ///< Defines the VHF's power level pin.
#define VHF_PTT 16          ///< Defines the VHF's sleep pin.
#define VHF_SLEEP 14        ///< Defines the VHF's sleep pin.
#define VHF_TX 12  ///< Defines the VHF's UART TX pin (for configuration).
#define VHF_RX 13  ///< Defines the VHF's UART RX pin (for configuration).

/// Defines the delay between each VHF configuration step.
static const uint32_t vhfEnableDelay = 1000;
/** Array of sin values for the DAC output.
 * Corresponds to one whole 8-bit sine output.
 */
static const uint8_t sinValues[NUM_SINS] = {
    152, 176, 198, 217, 233, 245, 252, 255, 252, 245, 233,
    217, 198, 176, 152, 127, 103, 79,  57,  38,  22,  10,
    3,   1,   3,   10,  22,  38,  57,  79,  103, 128};

#endif  // _RECOVERY_CONSTANTS_H_
