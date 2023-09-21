/*
 * PiComms.h
 *
 *  Created on: Aug 17, 2023
 *      Authors: Kaveet
 *      	Michael Salino-Hugg (msalinohugg@seas.harvard.edu)
 *
 * This header handles communicating between this board and the Pi.
 *
 * It will receive messages using the ThreadX queue functions, and then send that data over UART to the Pi.
 *
 * Other threads (e.g., GPS collection) should feed the data into this thread (from the queue), and then this thread will do the UART transmission to the Pi.

 *
 *	It will handle receiving incoming messages over UART, and then raise apprpriate flags to the rest of the code so we can act accordingly.
 *
 *	E.g., when it receives a start command from the PI, it will start the APRS flag.
 */
#ifndef INC_COMMS_INC_PICOMMS_H_
#define INC_COMMS_INC_PICOMMS_H_

#include "tx_api.h"
#include <stdint.h>
#include "Recovery Inc/GPS.h"

/*** MACROS ******************************************************************/

#define PI_COMMS_START_CHAR '$' //"$"

//Our max payload length, which is just the size of our data buffer minus the start/end character, message id and message_length field
#define PI_COMMS_MAX_DATA_PAYLOAD 255

//Flags to indicate if we had a good start character or not
#define PI_COMMS_VALID_START_FLAG 0x1
#define PI_COMMS_BAD_START_FLAG 0x2

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

#define GPS_TX_MESSAGE_SIZE (sizeof(GPS_TX_Message))



/*** TYPE DEFINITIONS ********************************************************/

typedef enum pi_comms_message_id_e {
	PI_COMM_MSG_BAD          = 0,
	PI_COMM_MSG_START        = 1, //pi --> rec: sets rec into recovery state
	PI_COMM_MSG_STOP         = 2, //pi --> rec: sets rec into waiting state
	PI_COMM_MSG_COLLECT_ONLY = 3, //pi --> rec: sets rec into rx gps state
	PI_COMM_MSG_CRITICAL     = 4, //pi --> rec: sets rec into critical state
	PI_COMM_MSG_GPS_PACKET   = 5, //rec --> pi: raw gps packet
    PI_COMM_MSG_CONFIG       = 6, //pi --> rec: rec board configuration
	NUM_RX_MESSAGES //NUM_RX_MESSAGES SHOULD ALWAYS BE THE LAST ELEMENT.
}PiCommsMessageID;

typedef struct __PI_COMMS_PACKET {
	uint8_t message_id; //PiCommsMessageID
	uint8_t data_length;
	uint8_t data_buffer[PI_COMMS_MAX_DATA_PAYLOAD];
}RX_Message;

typedef struct pi_comm_header_t {
	uint8_t start_byte; //'$'
	uint8_t id;         //PiCommsMessageID
	uint8_t length;
}PiCommHeader;

typedef struct __GPS_TX_MESSAGE {
	uint8_t start_byte; //'$'
	uint8_t message_id; //PiCommsMessageID
	uint8_t message_length;
	GPS_Data data;
}GPS_TX_Message;


/*** FUNCTION DECLARATIONS ***************************************************/

void pi_comms_parse_message(PiCommsMessageID message_id, uint8_t * payload_pointer, uint8_t payload_length);
void pi_comms_rx_thread_entry(ULONG thread_input);

//forward gps
void pi_comms_tx_forward_gps(uint8_t *buffer, uint8_t len);
void pi_comms_tx_thread_entry(ULONG thread_entry);

#endif //INC_COMMS_INC_PICOMMS_H_
