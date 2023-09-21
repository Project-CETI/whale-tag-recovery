/*
 * PiCommsTX.c
 *
 *  Created on: Aug 18, 2023
 *      Author: Kaveet
 */

#include "Comms Inc/PiComms.h"
#include "Recovery Inc/GPS.h"
#include <stdlib.h>
#include <stdint.h>

//Queue to share data between this and the GPS threads
TX_QUEUE gps_tx_queue;

//External variables (uart handler)
extern UART_HandleTypeDef huart2;

void pi_comms_tx_forward_gps(uint8_t *buffer, uint8_t len){
	PiCommHeader gps_header = {
			.start_byte = PI_COMMS_START_CHAR,
			.id = PI_COMM_MSG_GPS_PACKET,
			.length = len,
	};
	HAL_UART_Transmit(&huart2, (uint8_t *) &gps_header, sizeof(PiCommHeader), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, buffer, len, HAL_MAX_DELAY);
}

void pi_comms_tx_thread_entry(ULONG thread_input){

	/*GPS_Data z = {.is_dominica = true, .is_valid_data = true, .latitude = 0, .longitude = 0, .timestamp = {0, 0, 0}, .quality = 5, .msg_type = GPS_SIM};
	GPS_TX_Message y = {.message_id = GPS_MESSAGE, .data = {.is_dominica = true, .is_valid_data = true, .latitude = 0, .longitude = 0, .timestamp = {0x3210, 0x7654, 0xBA98}, .quality = 0xFEDCAB98, .msg_type = GPS_SIM}};
	ULONG * x =  (ULONG*) &y;
	uint8_t * w = (uint8_t*) x;
	HAL_Delay(1000);*/

	//Allocate a portion of memory for our queue data
	uint8_t * gps_tx_queue_start = malloc(GPS_TX_QUEUE_SIZE_BYTES);

	//Setup receive queue for GPS data
	tx_queue_create(&gps_tx_queue, "GPS TX Data Queue", GPS_TX_DATA_SIZE_BLOCKS, gps_tx_queue_start, GPS_TX_QUEUE_SIZE_BYTES);

	while (1){

		/*
		 * Note, since we only send one message to the Pi, this implementation is very simple.
		 * If there ever is a need for multiple messages, we can control the flow with event flags.
		 * I.e., the publishing thread will signal a flag, then send the data to the queue.
		 * This thread will monitor the flags, and then depending on which flag is set, read the data from the appropriate queue.
		 */

		//Variable to hold the incoming data
		GPS_TX_Message gps_msg;

		//Go to sleep forever until we receive some data.
		tx_queue_receive(&gps_tx_queue, &gps_msg.data, TX_WAIT_FOREVER);

		//Fill in our start byte, ID and length
		gps_msg.start_byte = PI_COMMS_START_CHAR;
		gps_msg.message_id = PI_COMM_MSG_GPS_PACKET;
//		gps_msg.message_length = GPS_TX_DATA_SIZE_BYTES;
		gps_msg.message_length = sizeof(GPS_Data);

		HAL_UART_Transmit(&huart2, (uint8_t *) &gps_msg, sizeof(GPS_TX_Message), HAL_MAX_DELAY);

	}

}

