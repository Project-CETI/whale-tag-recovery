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
#define PI_COMMS_VALID_START_FLAG (1 << 0)
#define PI_COMMS_OVERFLOW_FLAG	  (1 << 1)
//#define PI_COMMS_BAD_START_FLAG 0x2

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

#define PI_COMM_RX_BUFFER_COUNT 4
#define PI_COMM_RX_BUFFER_SIZE  8

/*** TYPE DEFINITIONS ********************************************************/

typedef enum pi_comms_message_id_e {
    /* Set recovery state*/
    PI_COMM_MSG_START        = 0x01, //pi --> rec: sets rec into recovery state
    PI_COMM_MSG_STOP         = 0x02, //pi --> rec: sets rec into waiting state
    PI_COMM_MSG_COLLECT_ONLY = 0x03, //pi --> rec: sets rec into rx gps state
    PI_COMM_MSG_CRITICAL     = 0x04, //pi --> rec: sets rec into critical state

    /* recovery packet */
    PI_COMM_MSG_GPS_PACKET   = 0x10, //rec --> pi: raw gps packet

    /* recovery configuration */
    PI_COMM_MSG_CONFIG_CRITICAL_VOLTAGE = 0x20,
    PI_COMM_MSG_CONFIG_VHF_POWER_LEVEL = 0x21,
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



/* configuration packets */
typedef struct __attribute__ ((__packed__, scalar_storage_order ("little-endian"))) {
    float value;
}PiCommCritVoltagePkt;

typedef struct __attribute__ ((__packed__, scalar_storage_order ("little-endian"))) {
    uint8_t value;
}PiCommTxLevelPkt;

typedef struct __attribute__ ((__packed__, scalar_storage_order ("little-endian"))) {
    PiCommHeader header;
    union {
        PiCommCritVoltagePkt critical_voltage;
        PiCommTxLevelPkt    vhf_level;
    } data;
}PiRxCommMessage;

/*** PUBLIC VARIABLES */
extern uint8_t pi_comm_rx_buffer[PI_COMM_RX_BUFFER_COUNT][PI_COMM_RX_BUFFER_SIZE];
extern volatile uint_fast8_t pi_comm_rx_buffer_start;
extern volatile uint_fast8_t pi_comm_rx_buffer_end;

/*** FUNCTION DECLARATIONS ***************************************************/

void pi_comms_rx_thread_entry(ULONG thread_input);

void pi_comms_tx_forward_gps(uint8_t *buffer, uint8_t len);

#endif //INC_COMMS_INC_PICOMMS_H_
