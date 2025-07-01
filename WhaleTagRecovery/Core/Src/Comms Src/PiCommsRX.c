/*
 * PiCommsRX.c
 *
 *  Created on: Aug 17, 2023
 *      Author: Kaveet
 */


#include "Comms Inc/PiComms.h"
#include "Lib Inc/state_machine.h"
#include "Recovery Inc/VHF.h"
#include "config.h"
#include "main.h"
#include "stm32u5xx_hal_uart.h"

//External variables
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef handle_GPDMA1_Channel2;


extern TX_EVENT_FLAGS_GROUP state_machine_event_flags_group;

//Private typedef


//Data buffer for receives
uint8_t raw_buffer[PI_COMM_RX_BUFFER_COUNT * PI_COMM_RX_BUFFER_SIZE] = {0};
uint8_t pi_comm_rx_buffer[PI_COMM_RX_BUFFER_COUNT][PI_COMM_RX_BUFFER_SIZE] = {0}; //max rx size = 3 + 4 = 7
volatile uint_fast8_t pi_comm_rx_buffer_start = 0;
volatile uint_fast8_t pi_comm_rx_buffer_end = 0;
volatile bool pi_comm_rx_buffer_overflow = false;

void Pi_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){
	int pi_comm_start = -1;
	int new = 0;
	for (int i = 0; i < Size; ) {
		// find command start character
		if (raw_buffer[i] != PI_COMMS_START_CHAR) {
			i++;
			continue;
		}

		// grab header
		pi_comm_start = i;
		i += sizeof(PiCommHeader);
		if (i > Size) {
			// incomplete message
			break;
		}

		// grab message
		i += ((PiCommHeader *)&raw_buffer[pi_comm_start])->length;
		if (i > Size) {
			// incomplete message
			break;
		}

		// move full message in buffer
		memcpy(&pi_comm_rx_buffer[pi_comm_rx_buffer_end][0], &raw_buffer[pi_comm_start], (i - pi_comm_start));
		pi_comm_rx_buffer_end = (pi_comm_rx_buffer_end + 1) % PI_COMM_RX_BUFFER_COUNT;
		new = 1;
		pi_comm_start = -1;
	}

	// shift remaining data
	int remaining = 0;
	if (pi_comm_start > 0) {
		remaining = sizeof(raw_buffer) - pi_comm_start;
		memmove(&raw_buffer[0], &raw_buffer[pi_comm_start], remaining);
	}

	// indicate new messages available
	if(new) {
		tx_event_flags_set(&state_machine_event_flags_group, STATE_COMMS_MESSAGE_AVAILABLE_FLAG, TX_OR);
	}

	//restart DMA
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, &raw_buffer[remaining], sizeof(raw_buffer) - remaining); //initiate next transfer
	__HAL_DMA_DISABLE_IT(&handle_GPDMA1_Channel2,DMA_IT_HT);//we don't want the half done transaction interrupt
}

void pi_comms_rx_init(void){
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, &raw_buffer[0], sizeof(raw_buffer)); //initiate next transfer
	__HAL_DMA_DISABLE_IT(&handle_GPDMA1_Channel2, DMA_IT_HT);//we don't want the half done transaction interrupt
}

