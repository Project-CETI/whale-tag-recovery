/*
 * PiCommsRX.h
 *
 *  Created on: Aug 17, 2023
 *      Author: Kaveet
 *
 *	The header file to handle communication FROM the Pi, to this board.
 *
 *	It will handle receiving incoming messages over UART, and then raise apprpriate flags to the rest of the code so we can act accordingly.
 *
 *	E.g., when it receives a start command from the PI, it will start the APRS flag.
 */

#ifndef INC_COMMS_INC_PICOMMSRX_H_
#define INC_COMMS_INC_PICOMMSRX_H_

#include "tx_api.h"
#include <stdint.h>

#define PI_COMMS_START_CHAR 0x24 //"$"

#define PI_COMMS_END_CHAR 0x40 //"@"

//Our max payload length, which is just the size of our data buffer minus the start/end character, message id and message_length field
#define PI_COMMS_MAX_DATA_PAYLOAD 252

//Flags to indicate if we had a good start character or not
#define PI_COMMS_VALID_START_FLAG 0x1
#define PI_COMMS_BAD_START_FLAG 0x2

typedef enum __PI_COMMS_RX_MESSAGES {
	BAD_MESSAGE = 0,
	START_RECOVERY = 1,
	STOP = 2,
	START_GPS_COLLECTION = 3,
	GPS_DATA_MESSAGE = 4,
	NUM_RX_MESSAGES //ALL ADDED MESSAGES SHOULD BE PUT ABOVE THIS ELEMENT. NUM_RX_MESSAGES SHOULD ALWAYS BE THE LAST ELEMENT IN THE ENUM.
}Message_IDs;

typedef struct __PI_COMMS_PACKET {

	//Message ID corresponding to ENUM above
	Message_IDs message_id;

	//Message lengths are limited to 252 bytes (see above).
	uint8_t data_length;

	//Data buffer for incoming messages
	uint8_t data_buffer[PI_COMMS_MAX_DATA_PAYLOAD];

}RX_Message;

void pi_comms_parse_message(Message_IDs message_id, uint8_t * payload_pointer, uint8_t payload_length);

void pi_comms_rx_thread_entry(ULONG thread_input);

#endif /* INC_COMMS_INC_PICOMMSRX_H_ */
