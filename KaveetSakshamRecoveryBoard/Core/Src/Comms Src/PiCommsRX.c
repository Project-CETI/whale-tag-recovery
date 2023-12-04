/*
 * PiCommsRX.c
 *
 *  Created on: Aug 17, 2023
 *      Author: Kaveet
 */


#include "Comms Inc/PiComms.h"
#include "Lib Inc/state_machine.h"
#include "Lib Inc/threads.h"
#include "Recovery Inc/VHF.h"
#include "config.h"
#include "main.h"
#include "stm32u5xx_hal_uart.h"

//External variables
extern UART_HandleTypeDef huart2;
extern Thread_HandleTypeDef threads[NUM_THREADS];
extern TX_EVENT_FLAGS_GROUP state_machine_event_flags_group;

//Private typedef


//Data buffer for receives
uint8_t pi_comm_rx_buffer[PI_COMM_RX_BUFFER_COUNT][PI_COMM_RX_BUFFER_SIZE] = {0}; //max rx size = 3 + 4 = 7
volatile uint_fast8_t pi_comm_rx_buffer_start = 0;
volatile uint_fast8_t pi_comm_rx_buffer_end = 0;
volatile bool pi_comm_rx_buffer_overflow = false;



TX_EVENT_FLAGS_GROUP pi_comms_event_flags_group;

void comms_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if (pi_comm_rx_buffer[pi_comm_rx_buffer_end][0] == PI_COMMS_START_CHAR){
		tx_event_flags_set(&pi_comms_event_flags_group, PI_COMMS_VALID_START_FLAG, TX_OR);
	}

}

void pi_comms_rx_thread_entry(ULONG thread_input){

	#if UART_ENABLED
	tx_event_flags_create(&pi_comms_event_flags_group, "Pi Comms RX Event Flags");

	HAL_UART_RegisterCallback(&huart2, HAL_UART_RX_COMPLETE_CB_ID, comms_UART_RxCpltCallback);
	//Start a non-blocking 1 byte UART read. Let the RX complete callback handle the rest.
	// HAL_UART_Receive_IT(&huart2, (uint8_t *) dataBuffer, 1);


	while (1) {
		uint8_t *end_buffer = pi_comm_rx_buffer[pi_comm_rx_buffer_end];
		ULONG actual_flags = 0;

		//Wait for start symbol
		while(!(actual_flags & PI_COMMS_VALID_START_FLAG)){
			HAL_UART_Receive_IT(&huart2, end_buffer, 1);
			//Wait for the Interrupt callback to fire and set the flag (telling us what to do further)
			tx_event_flags_get(&pi_comms_event_flags_group, PI_COMMS_VALID_START_FLAG , TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
		}

		//receive rest of header
		HAL_UART_Receive(&huart2, end_buffer + 1, sizeof(PiCommHeader) - 1, HAL_MAX_DELAY);

		//receive packet
		uint_fast8_t message_length = ((PiCommHeader *)end_buffer)->length;
		if(message_length){ //??? Is this if needed or is it checked inside `HAL_UART_Receive` ?
			HAL_UART_Receive(&huart2, end_buffer + 4, message_length, HAL_MAX_DELAY);
		}

		//indicate parsing needed
		uint_fast8_t next_end = (pi_comm_rx_buffer_end + 1) % PI_COMM_RX_BUFFER_COUNT;
		if(next_end == pi_comm_rx_buffer_start){
//			tx_event_flags_set(&state_machine_event_flags_group, STATE_COMMS_MESSAGE_AVAILABLE_FLAG, TX_OR);
			//ToDo: buffer overflow handling!!!
		} else {
			pi_comm_rx_buffer_end = next_end; //advance end of buffer
			tx_event_flags_set(&state_machine_event_flags_group, STATE_COMMS_MESSAGE_AVAILABLE_FLAG, TX_OR);
		}
	}
	#endif

}

