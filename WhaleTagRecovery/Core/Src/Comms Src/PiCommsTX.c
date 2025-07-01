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
TX_MUTEX pi_tx_mutex;

//External variables (uart handler)
extern UART_HandleTypeDef huart2;

typedef struct __attribute__ ((__packed__, scalar_storage_order ("little-endian"))) {
	PiCommHeader header;
	uint8_t msg[256];
} Packet;

void pi_comms_tx_init(void) {
    tx_mutex_create(&pi_tx_mutex, "Pi TX mutex", 1);
}

void pi_comms_tx_pong(void){
	PiCommHeader gps_header = {
		.start_byte = PI_COMMS_START_CHAR,
		.id = PI_COMM_PONG,
		.length = 0,
	};
	tx_mutex_get(&pi_tx_mutex,TX_WAIT_FOREVER);
	HAL_UART_Transmit(&huart2, (uint8_t *) &gps_header, sizeof(PiCommHeader), HAL_MAX_DELAY);
	tx_mutex_put(&pi_tx_mutex);
}

void pi_comms_tx_forward_gps(const uint8_t *buffer, uint8_t len){
	PiCommHeader gps_header = {
			.start_byte = PI_COMMS_START_CHAR,
			.id = PI_COMM_MSG_GPS_PACKET,
			.length = len,
	};
	tx_mutex_get(&pi_tx_mutex,TX_WAIT_FOREVER);
	HAL_UART_Transmit(&huart2, (uint8_t *) &gps_header, sizeof(PiCommHeader), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, buffer, len, HAL_MAX_DELAY);
	tx_mutex_put(&pi_tx_mutex);
}

void pi_comms_tx_callsign(const char *callsign){
	Packet pkt = {
			.header = {
				.start_byte = PI_COMMS_START_CHAR,
				.id = PI_COMM_MSG_CONFIG_APRS_CALLSIGN,
				.length = strlen(callsign),
			},
	};
	memcpy(pkt.msg, callsign, strlen(callsign));
	tx_mutex_get(&pi_tx_mutex,TX_WAIT_FOREVER);
	HAL_UART_Transmit(&huart2, (uint8_t *) &pkt, (sizeof(PiCommHeader) + strlen(callsign)), HAL_MAX_DELAY);
	tx_mutex_put(&pi_tx_mutex);
}

void pi_comms_tx_ssid(uint8_t ssid){
	Packet pkt = {
		.header = {
			.start_byte = PI_COMMS_START_CHAR,
			.id = PI_COMM_MSG_CONFIG_APRS_SSID,
			.length = 1,
		},
		.msg = {ssid},
	};
	tx_mutex_get(&pi_tx_mutex,TX_WAIT_FOREVER);
	HAL_UART_Transmit(&huart2, (uint8_t *) &pkt, (sizeof(PiCommHeader) + 1), HAL_MAX_DELAY);
	tx_mutex_put(&pi_tx_mutex);

}
