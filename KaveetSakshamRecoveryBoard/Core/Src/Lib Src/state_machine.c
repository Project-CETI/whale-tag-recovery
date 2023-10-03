/*
 * state_machine.c
 *
 *  Created on: Aug 22, 2023
 *      Author: Kaveet
 */

#include "config.h"
#include "Lib Inc/state_machine.h"
#include "Lib Inc/threads.h"
#include "Comms Inc/PiComms.h"
#include "main.h"

//Event flags for signaling changes in state
TX_EVENT_FLAGS_GROUP state_machine_event_flags_group;

//If simulating, set the simulation state defined in the header file, else, enter data capture as a default
static State state = STARTING_STATE;

//Threads array
extern Thread_HandleTypeDef threads[NUM_THREADS];

#if USB_BOOTLOADER_ENABLED
static void state_machine_check_usb_boot(ULONG timer_input){
	static uint16_t consecutive = 0;
	if(HAL_GPIO_ReadPin(USB_BOOT_EN_GPIO_Port, USB_BOOT_EN_Pin) == GPIO_PIN_RESET){
		consecutive++;
	}else{
		consecutive = 0;
	}

	/*reset system*/
	if(consecutive == USB_BOOTLOADER_HOLD_TIME_SECONDS){
		HAL_NVIC_SystemReset(); //reset system;
	}
}
#endif

static void priv__state_machine_heartbeat(ULONG heartbeat_input){
	for(int i = 0; i < 15; i++){
				HAL_GPIO_TogglePin(PWR_LED_NEN_GPIO_Port, PWR_LED_NEN_Pin);
				tx_thread_sleep(tx_ms_to_ticks(33));//flash at ~15 Hz for 1 second
	}
	 HAL_GPIO_WritePin(PWR_LED_NEN_GPIO_Port, PWR_LED_NEN_Pin, GPIO_PIN_SET);//ensure light is off after strobe
}

void state_machine_set_state(State new_state){
	//actions to take when exiting current state
	switch(new_state){
		case STATE_CRITICAL:
			break;

		case STATE_WAITING:
			break;

		case STATE_APRS:
			tx_thread_suspend(&threads[APRS_THREAD].thread);
			break;

		case STATE_GPS_COLLECT:
			tx_thread_suspend(&threads[GPS_COLLECTION_THREAD].thread);
			break;

		case STATE_FISHTRACKER:
			tx_thread_suspend(&threads[FISHTRACKER_THREAD].thread);
			break;

		default:
			break;
	}

	//actions to take when entering the new_state
	switch(new_state){
		case STATE_CRITICAL:
			gps_sleep(); 	//GPS: OFF
			aprs_sleep();	//VHF: OFF
			//ToDo: enter very lowpower mode
			break;

		case STATE_WAITING:
			gps_sleep(); 	//GPS: OFF
			aprs_sleep();	//VHF: OFF
			//ToDo: Low Power Mode - UART wakeup
			break;

		case STATE_APRS:
			gps_wake();		//GPS: ON
			aprs_wake();     //VHF: ON
			tx_thread_resume(&threads[APRS_THREAD].thread);
			break;

		case STATE_GPS_COLLECT:
			gps_wake();		//GPS: ON
			aprs_sleep();	//VHF: OFF
			tx_thread_resume(&threads[GPS_COLLECTION_THREAD].thread);
			break;

		case STATE_FISHTRACKER:
			gps_sleep();	//GPS: OFF
			aprs_wake(); 	//GPS: ON
			tx_thread_resume(&threads[FISHTRACKER_THREAD].thread);
			break;

		default:
			break;
	}
	state = new_state;
}

void state_machine_thread_entry(ULONG thread_input){

	//Event flags for triggering state changes
	tx_event_flags_create(&state_machine_event_flags_group, "State Machine Event Flags");

#if USB_BOOTLOADER_ENABLED
	//attach USB_boot_enable
	TX_TIMER bootloader_check_timer;

	tx_timer_create(
			&bootloader_check_timer,
			"USB Bootloader Check",
			state_machine_check_usb_boot, 0,
			1, tx_s_to_ticks(1),
			TX_AUTO_ACTIVATE
	);
#endif

	TX_TIMER heartbeat_timer;
	tx_timer_create(
			&heartbeat_timer,
			"Heartbeat LED Timer",
			priv__state_machine_heartbeat, 0,
			tx_s_to_ticks(30), tx_s_to_ticks(HEARTRATE_SECONDS),
			TX_AUTO_ACTIVATE
	); //heartbeat timer


	//Check the initial state and start in the appropriate state
	state_machine_set_state(state);

#if BATTERY_MONITOR_ENABLED
	tx_thread_resume(&threads[BATTERY_MONITOR_THREAD].thread);
#endif
#if UART_ENABLED
	tx_thread_resume(&threads[PI_COMMS_RX_THREAD].thread);
#endif
#if RTC_ENABLED
	tx_thread_resume(&threads[RTC_THREAD].thread);
#endif

	//Enter main thread execution loop ONLY if we arent simulating

	while (1){

		ULONG actual_flags = 0;

		//wait for message or critical battery
		tx_event_flags_get(&state_machine_event_flags_group, ALL_STATE_FLAGS, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);

		if(actual_flags & STATE_CRITICAL_LOW_BATTERY_FLAG){
			//enter a critical low power state (nothing runs)
			state_machine_set_state(STATE_CRITICAL);
		}
		else if(actual_flags & STATE_COMMS_MESSAGE_AVAILABLE_FLAG){
			//parse message from pi
			PiRxCommMessage *message = (PiRxCommMessage *)pi_comm_rx_buffer[pi_comm_rx_buffer_start];

			switch (message->header.id) {
				case PI_COMM_MSG_START:
					//enter recovery
					state_machine_set_state(STATE_APRS);
					break;

				case PI_COMM_MSG_STOP:
					//stop (and wait for more commands)
					state_machine_set_state(STATE_WAITING);
					break;

				case PI_COMM_MSG_COLLECT_ONLY:
					//enter regular GPS collection
					state_machine_set_state(STATE_GPS_COLLECT);
					break;

				case PI_COMM_MSG_CRITICAL:
					//enter a critical low power state (nothing runs)
					state_machine_set_state(STATE_CRITICAL);
					break;

				case PI_COMM_MSG_CONFIG_CRITICAL_VOLTAGE: {
						if(message->header.length < sizeof(PiCommCritVoltagePkt))
								break; //ToDo: return error

						g_config.critical_voltage = message->data.critical_voltage.value;
					}
					break;

				case PI_COMM_MSG_CONFIG_VHF_POWER_LEVEL: {
						if(message->header.length < sizeof(PiCommTxLevelPkt))
								break; //ToDo: return error
						g_config.vhf_power = message->data.vhf_level.value;
						vhf_set_power_level(g_config.vhf_power);
					}
					break;

				default:
					//Bad message ID - do nothing
					break;
			}
		}
	}
}


//Starts the APRS recovery thread
void enter_aprs_recovery(){

	tx_thread_resume(&threads[APRS_THREAD].thread);

	//TODO: Enable Power FET to turn GPS on
	HAL_GPIO_WritePin(GPS_NEN_GPIO_Port, GPS_NEN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(APRS_PD_GPIO_Port, APRS_PD_Pin, GPIO_PIN_SET);//wake aprs
	tx_thread_sleep(tx_ms_to_ticks(500)); //wait for aprs to wake
}

//Suspends the APRS recovery thread
void exit_aprs_recovery(){

	tx_thread_suspend(&threads[APRS_THREAD].thread);

	//TODO: Turn GPS off (through power FET)
	HAL_GPIO_WritePin(GPS_NEN_GPIO_Port, GPS_NEN_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(APRS_PD_GPIO_Port, APRS_PD_Pin, GPIO_PIN_RESET); //aprs -> sleep mode
}

//Starts the GPS data collection thread
void enter_gps_collection(){

	tx_thread_resume(&threads[GPS_COLLECTION_THREAD].thread);

	//TODO: Enable power FET to turn GPS on
	HAL_GPIO_WritePin(GPS_NEN_GPIO_Port, GPS_NEN_Pin, GPIO_PIN_RESET);
}

//Exit GPS data collection thread
void exit_gps_collection(){

	tx_thread_suspend(&threads[GPS_COLLECTION_THREAD].thread);

	//TODO: Turn GPS off (through power FET)
	HAL_GPIO_WritePin(GPS_NEN_GPIO_Port, GPS_NEN_Pin, GPIO_PIN_SET);
}

//Starts the threads that are active while waiting (comms, battery monitoring)
void enter_waiting(){
#if RTC_ENABLED
	tx_thread_resume(&threads[RTC_THREAD].thread);
#endif
#if BATTERY_MONITOR_ENABLED
	tx_thread_resume(&threads[BATTERY_MONITOR_THREAD].thread);
#endif
#if UART_ENABLED
	tx_thread_resume(&threads[PI_COMMS_RX_THREAD].thread);
#endif
}

//Exits the waiting threads
void exit_waiting(){
#if RTC_ENABLED
	tx_thread_resume(&threads[RTC_THREAD].thread);
#endif
#if BATTERY_MONITOR_ENABLED
	tx_thread_suspend(&threads[BATTERY_MONITOR_THREAD].thread);
#endif
#if UART_ENABLED
	tx_thread_suspend(&threads[PI_COMMS_RX_THREAD].thread);
#endif
}

//Enters critical low power state (everything off). Once we enter this, we cant exit it.
void enter_critical(){

	//Exit everything
	exit_aprs_recovery();
	exit_gps_collection();
	exit_waiting();
}
