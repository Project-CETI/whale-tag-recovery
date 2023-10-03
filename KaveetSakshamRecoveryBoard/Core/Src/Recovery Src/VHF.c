/*
 * VHF.c
 *
 *  Created on: Jun 10, 2023
 *      Author: Kaveet
 * 			    Michael Salino-Hugg (msalinohugg@seas.harvard.edu)
 */

#include "tx_api.h"
#include "Lib Inc/timing.h"
#include "Recovery Inc/VHF.h"
#include <stdio.h>
#include <string.h>

HAL_StatusTypeDef vhf_initialize(UART_HandleTypeDef huart, VHFPowerLevel power_level, char * tx_freq, char * rx_freq){

	//Set the modes of the GPIO pins attached to the vhf module.
	//Leave PTT floating, set appropriate power level and wake chip
	// HAL_Delay(1000);
	tx_thread_sleep(tx_s_to_ticks(1));
	vhf_set_ptt(true);
	vhf_set_power_level(power_level);
	vhf_wake();
	// HAL_Delay(1000);
	tx_thread_sleep(tx_s_to_ticks(1));


	return configure_dra818v(huart, false, false, false, tx_freq, rx_freq);
}

HAL_StatusTypeDef configure_dra818v(UART_HandleTypeDef huart, bool emphasis, bool lpf, bool hpf, char * tx_freq, char * rx_freq){

	//Note: variable tracks failure so that false (0) maps to HAL_OK (also 0)
	bool failed_config = false;

	//Data buffer to hold transmissions and responses
	char transmit_data[100];
	char response_data[100];

	// HAL_Delay(1000);
	tx_thread_sleep(tx_s_to_ticks(1));

	//Start with the VHF handshake to confirm module is setup correctly
	sprintf(transmit_data, "AT+DMOCONNECT \r\n");

	HAL_UART_Transmit(&huart, (uint8_t*) transmit_data, HANDSHAKE_TRANSMIT_LENGTH, HAL_MAX_DELAY);
	HAL_UART_Receive(&huart, (uint8_t*) response_data, HANDSHAKE_RESPONSE_LENGTH, HAL_MAX_DELAY);

	//Ensure the response matches the expected response
	if (strncmp(response_data, VHF_HANDSHAKE_EXPECTED_RESPONSE, HANDSHAKE_RESPONSE_LENGTH) != 0)
		failed_config = true;

	// HAL_Delay(1000);
	tx_thread_sleep(tx_s_to_ticks(1));

	//Now, set the parameters of the module
	sprintf(transmit_data, "AT+DMOSETGROUP=0,%s,%s,0000,0,0000\r\n", tx_freq, rx_freq);

	HAL_UART_Transmit(&huart, (uint8_t*) transmit_data, SET_PARAMETERS_TRANSMIT_LENGTH, HAL_MAX_DELAY);
	HAL_UART_Receive(&huart, (uint8_t*) response_data, SET_PARAMETERS_RESPONSE_LENGTH, HAL_MAX_DELAY);

	//Ensure the response matches the expected response
	if (strncmp(response_data, VHF_SET_PARAMETERS_EXPECTED_RESPONSE, SET_PARAMETERS_RESPONSE_LENGTH) != 0)
		failed_config = true;

	// HAL_Delay(1000);
	tx_thread_sleep(tx_s_to_ticks(1));

	//Set the volume of the transmissions
	sprintf(transmit_data, "AT+DMOSETVOLUME=%d\r\n", VHF_VOLUME_LEVEL);

	HAL_UART_Transmit(&huart, (uint8_t*) transmit_data, SET_VOLUME_TRANSMIT_LENGTH, HAL_MAX_DELAY);
	HAL_UART_Receive(&huart, (uint8_t*) response_data, SET_VOLUME_RESPONSE_LENGTH, HAL_MAX_DELAY);

	//Ensure the response matches the expected response
	if (strncmp(response_data, VHF_SET_VOLUME_EXPECTED_RESPONSE, SET_VOLUME_RESPONSE_LENGTH) != 0)
		failed_config = true;

	// HAL_Delay(1000);
	tx_thread_sleep(tx_s_to_ticks(1));

	//Set the filter parameters
	//Invert all the bools passed in since the VHF module treats "0" as true
	sprintf(transmit_data, "AT+SETFILTER=%d,%d,%d\r\n", emphasis, hpf, lpf);

	HAL_UART_Transmit(&huart, (uint8_t*) transmit_data, SET_FILTER_TRANSMIT_LENGTH, HAL_MAX_DELAY);
	HAL_UART_Receive(&huart, (uint8_t*) response_data, SET_FILTER_RESPONSE_LENGTH, HAL_MAX_DELAY);

	//Ensure the response matches the expected response
	if (strncmp(response_data, VHF_SET_FILTER_EXPECTED_RESPONSE, SET_FILTER_RESPONSE_LENGTH) != 0)
		failed_config = true;

	// HAL_Delay(1000);
	tx_thread_sleep(tx_s_to_ticks(1));

	return failed_config;
}


void vhf_set_ptt(bool is_tx){

	//isTx determines if the GPIO is high or low, and thus if we are transmitting or not
	HAL_GPIO_WritePin(VHF_PTT_GPIO_Port, VHF_PTT_Pin, is_tx);

}

void vhf_set_power_level(VHFPowerLevel power_level){

	//isHigh determines if we should use high power (1W) or low (0.5W)
	HAL_GPIO_WritePin(APRS_H_L_GPIO_Port, APRS_H_L_Pin, (power_level == VHF_POWER_HIGH)? GPIO_PIN_SET : GPIO_PIN_RESET);

}

void vhf_sleep(){

	//Set the PD pin on the module to low to make the module sleep
	HAL_GPIO_WritePin(APRS_PD_GPIO_Port, APRS_PD_Pin, GPIO_PIN_RESET);
}

void vhf_wake(){

	//Set the PD pin on the module to high to wake the module
	HAL_GPIO_WritePin(APRS_PD_GPIO_Port, APRS_PD_Pin, GPIO_PIN_SET);
	tx_thread_sleep(tx_ms_to_ticks(500));
	// HAL_Delay(500); //wait for module to wake

}
