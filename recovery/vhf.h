#ifndef VHF_H
#define VHF_H
#include "stdint.h"
/// Length of transmission pulse
#define VHF_TX_LEN 50
/// Frequency of tranmission pulse
#define VHF_HZ 440
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
/// Number of values in the sinValues DAC output array
#define NUM_SINS 32
// VHF HEADERS [START] --------------------------------------------------
void pinDescribe(void);

void initializeOutput(void);
void setOutput(uint8_t state);

void prepFishTx(float txFreq);
bool vhf_pulse_callback(repeating_timer_t *rt);

void initializeDra818v(bool highPower);
void configureDra818v(float txFrequency, float rxFrequency, uint8_t volume, bool emphasis, bool hpf, bool lpf);
void setPttState(bool state);
void setVhfState(bool state);
void configureVHF(void);
// VHF HEADERS [END] ----------------------------------------------------
#endif
