/*
 * GPS.h
 *
 *  Created on: Jun 22, 2023
 *      Author: Kaveet
 *
 * This file contains the driver for the GPS module used to find the position (lat/long) for the APRS recovery mode.
 *
 * GPS is the NEO M9N. See datasheet: https://cdn.sparkfun.com/assets/learn_tutorials/1/1/0/2/NEO-M9N-00B_DataSheet_UBX-19014285.pdf
 *
 * More info is in the Integration Manual and Interface Description Documents.
 *
 * Integration Manual: https://content.u-blox.com/sites/default/files/NEO-M9N_Integrationmanual_UBX-19014286.pdf
 * Interface Description: https://content.u-blox.com/sites/default/files/u-blox-M9-SPG-4.04_InterfaceDescription_UBX-21022436.pdf
 */

#ifndef INC_RECOVERY_INC_GPS_H_
#define INC_RECOVERY_INC_GPS_H_

#include "main.h"
#include "stm32u5xx_hal_uart.h"
#include <stdbool.h>

#define GPS_PACKET_START_CHAR '$'
#define GPS_PACKET_END_CHAR '\r'

#define GPS_UART_TIMEOUT 5000
#define GPS_TRY_LOCK_TIMEOUT 5000


#define GPS_SIMULATION false
#define GPS_SIM_LAT 42.2000
#define GPS_SIM_LON -71.0500

#define DEFAULT_LAT 15.31383
#define DEFAULT_LON -61.30075

#define DOMINICA_LAT_BOUNDARY 17.71468

typedef enum __GPS_MESSAGE_TYPES {
	GPS_SIM = 0,
	GPS_GLL = 1,
	GPS_GGA = 2,
	GPS_RMC = 3,
	GPS_NUM_MSG_TYPES //Should always be the last element in the enum. Represents the number of message types. If you need to add a new type, put it before this element.
}GPS_MsgTypes;

typedef struct __attribute__((__packed__, scalar_storage_order("little-endian"))) __GPS_Data {

	float latitude; //h00-h03
	float longitude; //h04-h07

	uint16_t timestamp[3]; //h08- h0E //0 is hour, 1 is minute, 2 is second

	uint8_t is_dominica; //bool //h0e

	uint8_t is_valid_data; //bool //h0f

	uint8_t msg_type; //GPS_MsgTypes //h10

	uint32_t quality; //h11-h14

}GPS_Data;

typedef struct __GPS_TypeDef {

	//UART handler for communication
	UART_HandleTypeDef* huart;

	bool is_pos_locked;

	GPS_Data data[GPS_NUM_MSG_TYPES];

}GPS_HandleTypeDef;

//Initialized and configures the GPS
HAL_StatusTypeDef initialize_gps(UART_HandleTypeDef* huart, GPS_HandleTypeDef* gps);

//Polls for new data from the GPS, parses it and stores it in the GPS struct. Returns true if it has successfully locked onto a positon.
bool read_gps_data(GPS_HandleTypeDef* gps);

//Repeatedly tries to read the GPS data to get a lock. User should call this function if they want a position lock.
bool get_gps_lock(GPS_HandleTypeDef* gps, GPS_Data* gps_data);

//Checks if a GPS location is in dominica based on the latitude and longitude
bool is_in_dominica(float latitude, float longitude);

//
void gps_sleep(void);
void gps_wake(void);

#endif /* INC_RECOVERY_INC_GPS_H_ */
