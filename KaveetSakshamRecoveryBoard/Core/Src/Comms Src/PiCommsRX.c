/*
 * PiCommsRX.c
 *
 *  Created on: Aug 17, 2023
 *      Author: Kaveet
 */


#include "Comms Inc/PiCommsRX.h"
#include "Lib Inc/threads.h"
#include "main.h"
#include "stm32u5xx_hal_uart.h"

//static function helpers.
static bool stopRecovery(bool * isActive);
static bool stopGPSCollection(bool * isActive);

//External variables
extern UART_HandleTypeDef huart2;
extern Thread_HandleTypeDef threads[NUM_THREADS];

//Data buffer for receives
uint8_t dataBuffer[256] = {0};

TX_EVENT_FLAGS_GROUP pi_comms_event_flags_group;

void comms_UART_RxCpltCallback(UART_HandleTypeDef *huart){

	if (dataBuffer[0] == PI_COMMS_START_CHAR){
		tx_event_flags_set(&pi_comms_event_flags_group, PI_COMMS_VALID_START_FLAG, TX_OR);
	}
	else {
		tx_event_flags_set(&pi_comms_event_flags_group, PI_COMMS_BAD_START_FLAG, TX_OR);
	}

}
void pi_comms_rx_thread_entry(ULONG thread_input){

	tx_event_flags_create(&pi_comms_event_flags_group, "Pi Comms RX Event Flags");

	HAL_UART_RegisterCallback(&huart2, HAL_UART_RX_COMPLETE_CB_ID, comms_UART_RxCpltCallback);
	HAL_GPIO_WritePin(APRS_PD_GPIO_Port, APRS_PD_Pin, GPIO_PIN_SET);
	while (1) {

		//Start a non-blocking 1 byte UART read. Let the RX complete callback handle the rest.
		HAL_UART_Receive_IT(&huart2, (uint8_t *) dataBuffer, 1);

		ULONG actual_flags;

		//Wait for the Interrupt callback to fire and set the flag (telling us what to do further)
		tx_event_flags_get(&pi_comms_event_flags_group, PI_COMMS_VALID_START_FLAG | PI_COMMS_BAD_START_FLAG, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);

		if (actual_flags & PI_COMMS_VALID_START_FLAG){

			//Read message ID and data length
			HAL_UART_Receive(&huart2, &dataBuffer[1], 2, HAL_MAX_DELAY);

			//Now read the entire payload
			HAL_UART_Receive(&huart2, &dataBuffer[3], dataBuffer[2], HAL_MAX_DELAY);

			//parse the message and act accordingly
			pi_comms_parse_message(dataBuffer[1], &dataBuffer[3], dataBuffer[2]);
		}

	}

}

void pi_comms_parse_message(Message_IDs message_id, uint8_t * payload_pointer, uint8_t payload_length){

	static bool isRecovery = false;
	static bool isGPSCollect = false;

	switch (message_id) {

		case START_RECOVERY:

			//Stop GPS data collection if its running (function will handle checking if its active or not)
			stopGPSCollection(&isGPSCollect);

			//Start recovery
			tx_thread_resume(&threads[APRS_THREAD].thread);
			isRecovery = true;

			/*if GPS collection is active, suspend it (so only one thread that accesses the GPS hardware is active at a time)
			if (isGPSCollect) {
				tx_thread_suspend(&threads[GPS_COLLECTION_THREAD].thread);
				isGPSCollect = false;
			}*/

			break;

		case STOP:

			//Stop recovery or GPS collection (function will handle checking if its active or not)
			stopRecovery(&isRecovery);
			stopGPSCollection(&isGPSCollect);

			break;

		case START_GPS_COLLECTION:

			//Call to suspend recovery if it is active (function will handle checking if its active or not)
			stopRecovery(&isRecovery);

			//Start Normal GPS collection
			tx_thread_resume(&threads[GPS_COLLECTION_THREAD].thread);
			isGPSCollect = true;

			//if APRS recovery is active, suspend it (so only one thread that accesses the GPS hardware is active at a time)
			/*if (isRecovery) {
				tx_thread_suspend(&threads[APRS_THREAD].thread);
				isRecovery = false;
			}*/

			break;
		default:
			//Bad message ID - do nothing
			break;

	}

}

//Checks to see if recovery is active, if it is, we stop it.
static bool stopRecovery(bool * isActive){

	//if APRS recovery is active, suspend it (so only one thread that accesses the GPS hardware is active at a time)
	if (*isActive){
		tx_thread_suspend(&threads[APRS_THREAD].thread);
		*isActive = false;
	}

}

//Checks to see if GPS data collection is active, if it is, we stop it.
static bool stopGPSCollection(bool * isActive){

	//if GPS data collecion is active, suspend it (so only one thread that accesses the GPS hardware is active at a time)
	if (*isActive){
		tx_thread_suspend(&threads[GPS_COLLECTION_THREAD].thread);
		*isActive = false;
	}

}


