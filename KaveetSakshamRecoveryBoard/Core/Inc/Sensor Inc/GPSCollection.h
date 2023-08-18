/*
 * GPSCollection.h
 *
 *  Created on: Aug 9, 2023
 *      Author: Kaveet
 *
 * The thread is intended to run when the main APRS algorithm is not.
 *
 * It'll attempt to get a GPS lock, and if it does, it will publish the message to the PiCommsTX thread (which sends the message to the Pi).
 *
 * If it doesnt get a GPS lock, it'll go to sleep and try again.
 */

#ifndef INC_SENSOR_INC_GPSCOLLECTION_H_
#define INC_SENSOR_INC_GPSCOLLECTION_H_

#include "Lib Inc/timing.h"
#include "Recovery Inc/GPS.h"
#include "tx_api.h"

#define GPS_COLLECTION_SLEEP_PERIOD_SECONDS 10
#define GPS_COLLECTION_SLEEP_PERIOD_TICKS (tx_s_to_ticks(GPS_COLLECTION_SLEEP_PERIOD_SECONDS))

void gps_collection_thread_entry(ULONG thread_input);

#endif /* INC_SENSOR_INC_GPSCOLLECTION_H_ */
