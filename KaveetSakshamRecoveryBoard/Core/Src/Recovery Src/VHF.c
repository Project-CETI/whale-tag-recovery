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


static inline HAL_StatusTypeDef __vhf_handshake(VHF_HandleTypdeDef *vhf){
	uint8_t response_data[HANDSHAKE_RESPONSE_LENGTH];
	HAL_StatusTypeDef status = HAL_OK;

	status |= HAL_UART_Transmit(vhf->huart, (uint8_t *)"AT+DMOCONNECT\r\n", HANDSHAKE_TRANSMIT_LENGTH, HAL_MAX_DELAY);
	status |= HAL_UART_Receive(vhf->huart, response_data, HANDSHAKE_RESPONSE_LENGTH, 500);
	status |= memcmp("+DMOCONNECT:0\r\n", response_data, HANDSHAKE_RESPONSE_LENGTH);
	
	return status;
}

static inline HAL_StatusTypeDef __vhf_set_freq(VHF_HandleTypdeDef *vhf){
	HAL_StatusTypeDef status = HAL_OK;
	uint8_t transmit_data[SET_PARAMETERS_TRANSMIT_LENGTH + 1];
	uint8_t response_data[SET_PARAMETERS_RESPONSE_LENGTH];

	sprintf((char *)transmit_data, "AT+DMOSETGROUP=0,%8.4f,%8.4f,0000,0,0000\r\n", vhf->config.tx_freq_MHz, vhf->config.rx_freq_MHz);

	status |= HAL_UART_Transmit(vhf->huart, transmit_data, SET_PARAMETERS_TRANSMIT_LENGTH, HAL_MAX_DELAY);
	status |= HAL_UART_Receive(vhf->huart,  response_data, SET_PARAMETERS_RESPONSE_LENGTH, 500);
	status |= memcmp("+DMOSETGROUP:0\r\n",  response_data, SET_PARAMETERS_RESPONSE_LENGTH);
	return status;
}

static inline HAL_StatusTypeDef __vhf_set_volume(VHF_HandleTypdeDef *vhf){
	HAL_StatusTypeDef status = HAL_OK;
	uint8_t transmit_data[SET_VOLUME_TRANSMIT_LENGTH + 1];
	uint8_t response_data[SET_VOLUME_RESPONSE_LENGTH];

	//Set the volume of the transmissions
	sprintf((char *)transmit_data, "AT+DMOSETVOLUME=%1d\r\n", vhf->config.volume);

	status |= HAL_UART_Transmit(vhf->huart, transmit_data, SET_VOLUME_TRANSMIT_LENGTH, HAL_MAX_DELAY);
	status |= HAL_UART_Receive(vhf->huart, response_data, SET_VOLUME_RESPONSE_LENGTH, 500);
	status |= memcmp("+DMOSETVOLUME:0\r\n",  response_data, SET_VOLUME_RESPONSE_LENGTH);
	return status;
}

static inline HAL_StatusTypeDef __vhf_set_filter(VHF_HandleTypdeDef *vhf){
	HAL_StatusTypeDef status = HAL_OK;
	uint8_t transmit_data[SET_FILTER_TRANSMIT_LENGTH + 1];
	uint8_t response_data[SET_FILTER_RESPONSE_LENGTH];

	//Invert all the bools passed in since the VHF module treats "0" as true
	sprintf((char *)transmit_data, "AT+SETFILTER=%d,%d,%d\r\n", !vhf->config.emphasis, !vhf->config.hpf, !vhf->config.lpf);

	status |= HAL_UART_Transmit(vhf->huart, transmit_data, SET_FILTER_TRANSMIT_LENGTH, HAL_MAX_DELAY);
	status |= HAL_UART_Receive(vhf->huart, response_data, SET_FILTER_RESPONSE_LENGTH, 500);
	status |= memcmp("+DMOSETFILTER:0\r\n",  response_data, SET_FILTER_RESPONSE_LENGTH);
	return status;
}


HAL_StatusTypeDef vhf_set_freq(VHF_HandleTypdeDef *vhf, float freq_MHz){
	if( (freq_MHz < 134.0f) || (freq_MHz >= 174.0f)){
		return HAL_ERROR; //freq out of range
	}
	vhf->config.rx_freq_MHz = freq_MHz;
	vhf->config.tx_freq_MHz = freq_MHz;

	//if vhf is asleep, config will take place on wakeup. 
	if(vhf->state != VHF_STATE_SLEEP){
		__vhf_set_freq(vhf);
	}

	return HAL_OK; 
}

HAL_StatusTypeDef vhf_init(VHF_HandleTypdeDef *vhf){

	//Set the modes of the GPIO pins attached to the vhf module.
	//Leave PTT floating, set appropriate power level and wake chip
	// HAL_Delay(1000);
	tx_thread_sleep(tx_s_to_ticks(1));
	vhf_set_power_level(vhf, vhf->power_level);
	vhf_wake(vhf);
	// HAL_Delay(1000);
	tx_thread_sleep(tx_s_to_ticks(1));


	return configure_dra818v(vhf);
}

HAL_StatusTypeDef configure_dra818v(VHF_HandleTypdeDef *vhf){
	//Note: variable tracks failure so that false (0) maps to HAL_OK (also 0)
	HAL_StatusTypeDef failed_config = HAL_OK;

	//Now, set the parameters of the module
	failed_config =__vhf_set_freq(vhf);
	if(failed_config != HAL_OK)
		return failed_config;

	//Set volume
	failed_config =__vhf_set_volume(vhf);
	if(failed_config != HAL_OK)
		return failed_config;

	//Set the filter parameters
	failed_config = __vhf_set_filter(vhf);
	
	return failed_config;
}

void vhf_set_power_level(VHF_HandleTypdeDef *vhf, VHFPowerLevel power_level){

	//isHigh determines if we should use high power (1W) or low (0.5W)
	vhf->power_level = power_level;
	HAL_GPIO_WritePin(vhf->h_l.port, vhf->h_l.pin, (power_level == VHF_POWER_HIGH)? GPIO_PIN_SET : GPIO_PIN_RESET);

}

void vhf_sleep(VHF_HandleTypdeDef *vhf){

	//set VHF_TX_GPIO as output
	uint32_t reg = VHF_TX_GPIO_Port->MODER;
	reg &= ~0b11; //mask off pin 0: VHF_TX_Pin
	reg |= (0b01); //set as output
	VHF_TX_GPIO_Port->MODER = reg;
	HAL_GPIO_WritePin(VHF_TX_GPIO_Port, VHF_TX_Pin, GPIO_PIN_RESET);


	//Set the PD pin on the module to low to make the module sleep
	HAL_GPIO_WritePin(vhf->pd.port, vhf->pd.pin, GPIO_PIN_RESET);
	vhf->state = VHF_STATE_SLEEP;
}

HAL_StatusTypeDef vhf_tx(VHF_HandleTypdeDef *vhf){
	if(vhf->state == VHF_STATE_SLEEP){
		//try to wake module
		if(vhf_wake(vhf) != HAL_OK) 
			return HAL_ERROR;
	}
	HAL_GPIO_WritePin(vhf->ptt.port, vhf->ptt.pin, GPIO_PIN_SET);
	vhf->state = VHF_STATE_TX;
	return HAL_OK;
}

HAL_StatusTypeDef vhf_rx(VHF_HandleTypdeDef *vhf){
	if(vhf->state == VHF_STATE_SLEEP){
		//try to wake module
		if(vhf_wake(vhf) != HAL_OK) 
			return HAL_ERROR;
	}
	HAL_GPIO_WritePin(vhf->ptt.port, vhf->ptt.pin, GPIO_PIN_RESET);
	vhf->state = VHF_STATE_RX;
	return HAL_OK;
}

HAL_StatusTypeDef vhf_wake(VHF_HandleTypdeDef *vhf){

	//set VHF_TX as usart
//	HAL_GPIO_WritePin(VHF_TX_GPIO_Port, VHF_TX_Pin, GPIO_PIN_SET);
	uint32_t reg = VHF_TX_GPIO_Port->MODER;
	reg &= ~0b11; //mask off pin 0: VHF_TX_Pin
	reg |= (0b10); //set as alternative
	VHF_TX_GPIO_Port->MODER = reg;

	//Set the PD pin on the module to high to wake the module
	HAL_GPIO_WritePin(vhf->ptt.port, vhf->ptt.pin, GPIO_PIN_RESET);
//	HAL_GPIO_WritePin(vhf->pd.port, vhf->pd.pin, GPIO_PIN_SET);
//	tx_thread_sleep(tx_ms_to_ticks(500)); //wait for module to wake

	//Start with the VHF handshake to confirm module is setup correctly
	/* 
	 * If the host doesn’t receive any response from module after three times
	 * of continuously sending this command, it will restart the module.
	 *                                                       -DRA818V datasheet
	 */
	uint_fast8_t failed_attempts = 0;
	while(__vhf_handshake(vhf) != HAL_OK){ //I'm not sure why this needs to be called prior to restart... but it does?
		failed_attempts++;
		vhf_sleep(vhf);
		uint32_t reg = VHF_TX_GPIO_Port->MODER;
		reg &= ~0b11; //mask off pin 0: VHF_TX_Pin
		reg |= (0b10); //set as alternative
		VHF_TX_GPIO_Port->MODER = reg;
		HAL_GPIO_WritePin(vhf->pd.port, vhf->pd.pin, GPIO_PIN_SET);
		if(failed_attempts == 20){//failed for 10 seconds
			return HAL_ERROR;
		}
	}


	/*
	* The parameters will be lost after the module is powered off. Therefore,
	* the module must be configured with the parameters at each power-on in 
	* order to work normally. 
	*                                                        -DRA818V datasheet
	*/
	return configure_dra818v(vhf);
}
