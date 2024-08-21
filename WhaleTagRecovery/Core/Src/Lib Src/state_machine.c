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
#include "Recovery Inc/AprsPacket.h"
#include "Recovery Inc/Aprs.h"

//Event flags for signaling changes in state
TX_EVENT_FLAGS_GROUP state_machine_event_flags_group;

//If simulating, set the simulation state defined in the header file, else, enter data capture as a default
static State state = STARTING_STATE;

//Threads array
extern Thread_HandleTypeDef threads[NUM_THREADS];
extern VHF_HandleTypdeDef vhf;

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
			HAL_PWREx_EnterSHUTDOWNMode();
			break;

		case STATE_WAITING:
			gps_sleep(); 	//GPS: OFF
			aprs_sleep();	//VHF: OFF
			//ToDo: Low Power Mode - UART wakeup
			break;

		case STATE_APRS:
			gps_wake();		//GPS: ON
			tx_thread_resume(&threads[APRS_THREAD].thread);
			break;

		case STATE_GPS_COLLECT:
			gps_wake();		//GPS: ON
			aprs_sleep();	//VHF: OFF
			tx_thread_resume(&threads[GPS_COLLECTION_THREAD].thread);
			break;

		case STATE_FISHTRACKER:
			gps_sleep();	//GPS: OFF
			tx_thread_resume(&threads[FISHTRACKER_THREAD].thread);
			break;

		default:
			break;
	}
	state = new_state;
}


/*
 * state_machine_thread_entry
 * state machine update thread.
 */
void state_machine_thread_entry(ULONG thread_input){
	
	//Event flags for triggering state changes
	tx_event_flags_create(&state_machine_event_flags_group, "State Machine Event Flags");

	//Check the initial state and start in the appropriate state
	state_machine_set_state(state);
	vhf_set_freq(&vhf, g_config.aprs_freq);

#if BATTERY_MONITOR_ENABLED
	tx_thread_resume(&threads[BATTERY_MONITOR_THREAD].thread);
#endif
#if UART_ENABLED
	tx_thread_resume(&threads[PI_COMMS_RX_THREAD].thread);
#endif
#if RTC_ENABLED
	tx_thread_resume(&threads[RTC_THREAD].thread);
#endif
	tx_thread_resume(&threads[GPS_BUFFER_THREAD].thread);

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
				//State Change Message
				case PI_COMM_MSG_START: {
					//enter recovery
					state_machine_set_state(STATE_APRS);
					break;
				}

				case PI_COMM_MSG_STOP: {
					//stop (and wait for more commands)
					state_machine_set_state(STATE_WAITING);
					break;
				}

				case PI_COMM_MSG_COLLECT_ONLY: {
					//enter regular GPS collection
					state_machine_set_state(STATE_GPS_COLLECT);
					break;
				}

				case PI_COMM_MSG_CRITICAL: {
					//enter a critical low power state (nothing runs)
					state_machine_set_state(STATE_CRITICAL);
					break;
				}

				case PI_COMM_MSG_APRS_MESSAGE: {
					char msg_buffer[257];
					size_t len = message->header.length;
					len = (67 < len) ? 67 : len;

					memcpy(msg_buffer, &message->data, len);
					msg_buffer[len] = '\0';
					aprs_tx_message(msg_buffer, strlen(msg_buffer));
					break;
				}

				case PI_COMM_PING: {
					pi_comms_tx_pong();
					break;
				}

				//Configuration change message
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
						vhf_set_power_level(&vhf, g_config.vhf_power);
					}
					break;

				case PI_COMM_MSG_CONFIG_APRS_FREQUENCY: {
						if(message->header.length < sizeof(PiCommAPRSFreq))
							break; //ToDo: return error
						g_config.aprs_freq = message->data.aprs_freq_MHz.value;
						vhf_set_freq(&vhf, g_config.aprs_freq);
					}
					break;

				case PI_COMM_MSG_CONFIG_APRS_CALLSIGN: {
					char callsign_buffer[7];
					size_t len = message->header.length;
					len = (6 < len) ? 6 : len;
					memcpy(callsign_buffer, &message->data, len);
					callsign_buffer[len] = '\0';

					aprs_set_callsign(callsign_buffer);
					break;
				}

				case PI_COMM_MSG_CONFIG_APRS_COMMENT: {
					char comment_buffer[257];
					size_t len = message->header.length;

					memcpy(comment_buffer, &message->data, len);
					comment_buffer[len] = '\0';

					//ToDo: implement APRS comment assignment
					break;
				}

				case PI_COMM_MSG_CONFIG_APRS_SSID: {
					aprs_set_ssid(message->data.u8_pkt);
					break;
				}

				case PI_COMM_MSG_CONFIG_MSG_RCPT_CALLSIGN: {
					char callsign_buffer[7];
					size_t len = message->header.length;
					len = (6 < len) ? 6 : len;
					memcpy(callsign_buffer, &message->data, len);
					callsign_buffer[len] = '\0';

					aprs_set_msg_recipient_callsign(callsign_buffer);
					break;
				}

				case PI_COMM_MSG_CONFIG_MSG_RCPT_SSID: {
					aprs_set_msg_recipient_ssid(message->data.u8_pkt);
					break;
				}

				case PI_COMM_MSG_CONFIG_HOSTNAME: {
					size_t len = message->header.length;
					len = (len > sizeof(g_config.pi_hostname) - 1) ? (sizeof(g_config.pi_hostname) - 1) : len;
					memcpy(g_config.pi_hostname, &message->data, len);
					g_config.pi_hostname[len] = 0;
					break;
				}

				case PI_COMM_MSG_QUERY_STATE: {
					//ToDo: return recovery board state to pi
					break;
				}

				case PI_COMM_MSG_QUERY_CRITICAL_VOLTAGE: {
					//ToDo: return critical voltage setting to pi
					break;
				}

				case PI_COMM_MSG_QUERY_VHF_POWER_LEVEL: {
					//ToDo: return critical voltage setting to pi
					break;
				}

				case PI_COMM_MSG_QUERY_APRS_FREQ: {
					//ToDo: implement
					break;
				}

				case PI_COMM_MSG_QUERY_APRS_CALLSIGN: {
					static char callsign[7] = "";
					aprs_get_callsign(callsign);
					pi_comms_tx_callsign(callsign);
					break;
				}

				case PI_COMM_MSG_QUERY_APRS_MESSAGE: {
					//ToDo: implement
					break;
				}

				case PI_COMM_MSG_QUERY_APRS_SSID: {
					uint8_t ssid;
					aprs_get_ssid(&ssid);
					pi_comms_tx_ssid(ssid);
					break;
				}

				default:
					//Bad message ID - do nothing
					break;
			}
			pi_comm_rx_buffer_start =  (pi_comm_rx_buffer_start + 1) % PI_COMM_RX_BUFFER_COUNT;
		}
	}
}





