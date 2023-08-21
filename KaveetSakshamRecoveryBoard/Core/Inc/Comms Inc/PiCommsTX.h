/*
 * PiCommsTX.h
 *
 *  Created on: Aug 18, 2023
 *      Author: Kaveet
 *
 * This board handles communicating from this board TO the Pi.
 *
 * It will receive messages using the ThreadX queue functions, and then send that data over UART to the Pi.
 *
 * Other threads (e.g., GPS collection) should feed the data into this thread (from the queue), and then this thread will do the UART transmission to the Pi.
 */

#ifndef INC_COMMS_INC_PICOMMSTX_H_
#define INC_COMMS_INC_PICOMMSTX_H_

#include "Recovery Inc/GPS.h"
#include "Comms Inc/PiCommsRX.h"
#include "tx_api.h"

typedef struct __GPS_TX_MESSAGE {

	uint8_t start_byte;

	Message_IDs message_id;

	uint8_t message_length;

	GPS_Data data;
}GPS_TX_Message;

#define GPS_TX_MESSAGE_SIZE (sizeof(GPS_TX_Message))

/*ThreadX queue sizes must always be expressed as 32-bits (ulong), not the normal 8-bits.
 * We receive the GPS_Data struct defined in GPS.h
 *
 * Longitude = 32 bits
 * Latitude = 32 bits
 * timestamp[0] + timestamp[1] = 32 bits
 * timestamp[2] + is_in_dominica + is_valid_data = 32 bits
 * GPS_MsgType = 32 bits
 * quality = 32 Bits
 *
 * Thus, our message size is 6 blocks (32 bits each) or 24 bytes.
 */
#define GPS_TX_DATA_SIZE_BLOCKS 6
#define GPS_TX_DATA_SIZE_BYTES 24

//The total size of our queue (should be a multiple of 28)
#define GPS_TX_QUEUE_SIZE_BYTES 280

void pi_comms_tx_thread_entry(ULONG thread_entry);
#endif /* INC_COMMS_INC_PICOMMSTX_H_ */
