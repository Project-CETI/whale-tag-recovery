/*
 * GpsGeofencing.c
 *
 *  Created on: Aug 9, 2023
 *      Author: Kaveet
 */

#include "Sensor Inc/GPSCollection.h"
#include "Recovery Inc/GPS.h"
#include "Comms Inc/PiCommsTX.h"
#include "main.h"

//External variables defined in other C files (uart handler and queue for comms threads)
extern UART_HandleTypeDef huart3;
extern TX_QUEUE gps_tx_queue;

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

			//DEBUG
			gps_data.quality = 0x89ABCDEF;
			gps_data.timestamp[0] = 0x3210;
			gps_data.timestamp[1] = 0x7654;
			gps_data.timestamp[2] = 0xBA98;

			tx_queue_send(&gps_tx_queue, &gps_data, TX_WAIT_FOREVER);

		}

		//Go to sleep and try again later
		tx_thread_sleep(GPS_COLLECTION_SLEEP_PERIOD_TICKS);
	}

}
