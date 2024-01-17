/*
 * Aprs.h
 *
 *  Created on: May 26, 2023
 *      Author: Kaveet
 *              Michael Salino-Hugg (msalinohugg@seas.harvard.edu) [KC1TUJ]
 *
 * This file contains the main APRS thread responsible for board recovery.
 *
 * It receives the GPS data from the GPS functions, and then calls the appropriate library functions
 *  to break up the data into an array and transmit it to the VHF module
 *
 * Library Functions/Files:
 *  - AprsTransmit -> handles the transmission of the sine wave
 *  - AprsPacket -> breaks the GPS data into appropriate packets
 */

#ifndef INC_RECOVERY_INC_APRS_H_
#define INC_RECOVERY_INC_APRS_H_

/*

 */
#include "tx_api.h"

#define APRS_PACKET_MAX_LENGTH 255


//send flag for entire ax25 transmit delay time
#define APRS_BIT_RATE_BIT_PER_S (1200)
#define APRS_BYTE_RATE_BYTEP_PER_S (APRS_BIT_RATE_BIT_PER_S / 8)
#define AX25_TXDELAY_MS (500)
#define AX25_FLAG_COUNT (AX25_TXDELAY_MS * APRS_BYTE_RATE_BYTEP_PER_S / 1000)

#define APRS_PACKET_LENGTH ((218 - 150) + AX25_FLAG_COUNT)

#define GPS_SLEEP_LENGTH tx_s_to_ticks(10)

#define APRS_BASE_SLEEP_LENGTH tx_s_to_ticks(60)

#define NUM_TX_ATTEMPTS 3

//Events inside of our aprs state machine 
#define APRS_EVENT_TRANSMIT_POSITION   (1 << 0)
#define APRS_EVENT_RETRANSMIT_POSITION (1 << 1)
#define APRS_EVENT_TRANSMIT_MESSAGE    (1 << 2)

#define ARPS_ALL_EVENT_FLAGS (APRS_EVENT_TRANSMIT_POSITION | APRS_EVENT_RETRANSMIT_POSITION | APRS_EVENT_TRANSMIT_MESSAGE)


//Main thread entry
void aprs_thread_entry(ULONG aprs_thread_input);
void aprs_sleep(void);
void aprs_tx_message(const char* message, size_t message_len);
#endif /* INC_RECOVERY_INC_APRS_H_ */
