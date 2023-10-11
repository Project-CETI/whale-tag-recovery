/*
 * state_machine.h
 *
 *  Created on: Aug 22, 2023
 *      Author: Kaveet
 */

#ifndef INC_LIB_INC_STATE_MACHINE_H_
#define INC_LIB_INC_STATE_MACHINE_H_

#include "tx_api.h"
#include <stdint.h>

//Should correspond with the state types enum below
#define STARTING_STATE STATE_WAITING

//Flags inside of our state machine event flags
#define STATE_COMMS_MESSAGE_AVAILABLE_FLAG (1 << 0)
#define STATE_CRITICAL_LOW_BATTERY_FLAG (1 << 1)

#define ALL_STATE_FLAGS (STATE_COMMS_MESSAGE_AVAILABLE_FLAG | STATE_CRITICAL_LOW_BATTERY_FLAG)

typedef enum {
	STATE_CRITICAL = 0, //Do nothing and be in super low power
	STATE_WAITING = 1, //Do the bare minimum (comms and battery monitoring)
	STATE_APRS = 2, //APRS Transmissions
	STATE_GPS_COLLECT = 3, //Just GPS data collection
	STATE_FISHTRACKER, //Just VHF Beacon
	NUM_STATES //Add new states ABOVE this line
}State;

//Main thread entry for the state machine thread
void state_machine_thread_entry(ULONG thread_input);

#endif /* INC_LIB_INC_STATE_MACHINE_H_ */
