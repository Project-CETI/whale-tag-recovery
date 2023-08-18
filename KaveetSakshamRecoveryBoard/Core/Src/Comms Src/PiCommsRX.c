/*
 * PiCommsRX.c
 *
 *  Created on: Aug 17, 2023
 *      Author: Kaveet
 */


#include "Comms Inc/PiCommsRX.h"
#include "main.h"
#include "stm32u5xx_hal_uart.h"

extern UART_HandleTypeDef huart2;
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
		HAL_UART_Receive_IT(&huart2, dataBuffer, 15);

		char transmit_data[100];


			//Start with the VHF handshake to confirm module is setup correctly
			sprintf(transmit_data, "AT+DMOCONNECT \r\n");

			HAL_UART_Transmit(&huart2, (uint8_t*) transmit_data, 16, HAL_MAX_DELAY);

		ULONG actual_flags;

		//Wait for the Interrupt callback to fire and set the flag (telling us what to do further)
		tx_event_flags_get(&pi_comms_event_flags_group, PI_COMMS_VALID_START_FLAG | PI_COMMS_BAD_START_FLAG, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);

		if (actual_flags & PI_COMMS_VALID_START_FLAG){

			//Read message ID and data length
			HAL_UART_Receive(&huart2, &dataBuffer[1], 2, HAL_MAX_DELAY);

			//Now read the entire payload
			HAL_UART_Receive(&huart2, &dataBuffer[3], &dataBuffer[2], HAL_MAX_DELAY);
		}

	}

}
