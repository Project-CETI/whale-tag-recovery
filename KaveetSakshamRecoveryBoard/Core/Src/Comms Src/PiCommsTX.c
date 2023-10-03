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


