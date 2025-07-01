/*
 * AprsTransmit.h
 *
 *  Created on: May 26, 2023
 *      Author: Kaveet
 *              Michael Salino-Hugg [KC1TUJ] (msalinohugg@seas.harvard.edu)
 *
 *
 * This file handles the transmission of the APRS sine wave to the VHF module.
 *
 * This includes:
 * 			- Generating the sine wave and appropriately apply the frequency modulation to it
 *
 * Parameters:
 * 			- The raw data array to transmit (received from main APRS task)
 *
 * It uses the DAC to transmit a sine wave through DMA. The frequency of the wave is controlled by a hardware timer (changing the period changes the frequency).
 *
 * There is a software timer that cycles through bits, changing the frequency appropriately.
 *
 * A "0" bit indicates a change in frequency, while a "1" bit will keep the same frequency. We toggle between 1200 and 2200 Hz.
 *
 * During the main data/payload, there must be a stuffed bit (forced transition) after 5 consecutive 1's.
 */

#ifndef INC_RECOVERY_INC_APRSTRANSMIT_H_
#define INC_RECOVERY_INC_APRSTRANSMIT_H_

//Includes
#include <stdbool.h>
#include <stdint.h>
#include "tx_api.h"
#include "Lib Inc/timing.h"
#include "stm32u5xx_hal.h"

//Defines
//The time to transmit each bit for
//Each ThreadX tick is 50us -> 17 ticks makes it 850us. Based off the PICO code for the old recovery boards, where the bit time is 832us.
#define APRS_TRANSMIT_BIT_TIME tx_us_to_ticks(850)

//The hardware timer periods for 1200Hz and 2400Hz signals
#define APRS_TRANSMIT_PERIOD_1200HZ 84
#define APRS_TRANSMIT_PERIOD_2200HZ 45

//Max digital input to a dac for 8-bit inputs
#define APRS_TRANSMIT_MAX_DAC_INPUT 256

//Self-explanatory
#define BITS_PER_BYTE 8

//The number of sample points for the output sine wave. The more samples the smoother the wave.
#define APRS_TRANSMIT_NUM_SINE_SAMPLES 100

//Macros
//Reads out a specific bit inside of a byte. Used in source code to iterate through bits.
#define aprs_transmit_read_bit(BYTE, BIT_POS) (((BYTE) >> (BIT_POS)) & 0x01)

//Public functions
void aprs_transmit_init(void);
bool aprs_transmit_send_data(uint8_t * packet_data, uint16_t packet_length);
void aprs_transmit_bit_timer_entry(ULONG bit_timer_input);

#endif /* INC_RECOVERY_INC_APRSTRANSMIT_H_ */
