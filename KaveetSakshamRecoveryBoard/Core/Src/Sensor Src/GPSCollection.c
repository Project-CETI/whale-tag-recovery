/*
 * GpsGeofencing.c
 *
 *  Created on: Aug 9, 2023
 *      Author: Kaveet
 */

#include "Sensor Inc/GPSCollection.h"
#include "Recovery Inc/GPS.h"
#include "main.h"

//External variables defined in other C files (uart handler and state machine event flag)
extern UART_HandleTypeDef huart3;

void gps_collection_thread_entry(ULONG thread_input){

	//Initialize GPS struct
	GPS_HandleTypeDef gps;
	initialize_gps(&huart3, &gps);

	//Thread infinite loop entry
	while (1){

		//Create struct to hold gps data
		GPS_Data gps_data;

		//Try to get a GPS lock
		if (get_gps_lock(&gps, &gps_data)){


		}

		//Go to sleep and try again later
		tx_thread_sleep(GPS_COLLECTION_SLEEP_PERIOD_TICKS);
	}

}
