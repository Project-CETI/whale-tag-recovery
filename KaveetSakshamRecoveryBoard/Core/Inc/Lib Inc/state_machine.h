/*
 * state_machine.h
 *
 *  Created on: Aug 22, 2023
 *      Author: Kaveet
 */

#ifndef INC_LIB_INC_STATE_MACHINE_H_
#define INC_LIB_INC_STATE_MACHINE_H_

#include "tx_api.h"

#define IS_SIMULATING false

//Should correspond with the state types enum below
#define SIMULATING_STATE 1

//Flags inside of our state machine event flags
#define STATE_COMMS_STOP_FLAG 0x1
#define STATE_COMMS_APRS_FLAG 0x2
#define STATE_COMMS_COLLECT_GPS_FLAG 0x4
#define STATE_CRITICAL_LOW_BATTERY_FLAG 0x8

#define ALL_STATE_FLAGS (STATE_COMMS_STOP_FLAG | STATE_COMMS_APRS_FLAG | STATE_COMMS_COLLECT_GPS_FLAG | STATE_CRITICAL_LOW_BATTERY_FLAG)

typedef enum {
	STATE_CRITICAL = 0, //Do nothing and be in super low power
	STATE_WAITING = 1, //Do the bare minimum (comms and battery monitoring)
	STATE_APRS = 2, //APRS Transmissions
	STATE_GPS_COLLECT = 3, //Just GPS data collection
	NUM_STATES //Add new states ABOVE this line
}State;

//Main thread entry for the state machine thread
void state_machine_thread_entry(ULONG thread_input);

//Starts the APRS recovery thread
void enter_aprs_recovery();

//Suspends the APRS recovery thread
void exit_aprs_recovery();

//Starts the GPS data collection thread
void enter_gps_collection();

//Exit GPS data collection thread
void exit_gps_collection();

//Starts the threads that are active while waiting (comms, battery monitoring)
void enter_waiting();

//Exits the waiting threads
void exit_waiting();

//Enters critical low power state (everything off)
void enter_critical();

#endif /* INC_LIB_INC_STATE_MACHINE_H_ */
