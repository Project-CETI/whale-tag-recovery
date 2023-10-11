/*
 * FishTracker.c
 *
 *  Created on: Aug 16, 2023
 *      Author: Kaveet
 */

#include "Recovery Inc/FishTracker.h"
#include "Recovery Inc/VHF.h"
#include <stdint.h>
#include "constants.h"
#include "main.h"
#include "config.h"
#include <stdbool.h>

//For our DAC values
static void calcFishtrackerDacValues();
uint32_t fishtracker_dac_input[FISHTRACKER_NUM_DAC_SAMPLES] = {0};

//Extern variables
extern DAC_HandleTypeDef hdac1;
extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart4;
extern VHF_HandleTypdeDef vhf;

void fishtracker_thread_entry(ULONG thread_input){

	//Initialization - find out our DAC values and the timer period
	calcFishtrackerDacValues();

	//Initialize VHF module for transmission. Turn transmission off so we don't hog the frequency
	vhf_set_freq(&vhf, FISHTRACKER_CARRIER_FREQ_MHZ);
	vhf_set_power_level(&vhf, FISHTRACKER_POWER);

	//We must use DMA since our DAC is configured that way in the IOC (See calcFishtrackerDacValues for the override)
	HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, fishtracker_dac_input, FISHTRACKER_NUM_DAC_SAMPLES, DAC_ALIGN_8B_R);
	HAL_TIM_Base_Start(&htim2);

	while (1) {

		//We have a constant sine wave being outputted the VHF module. Since its FM modulation, there is no way to make the output wave truly "0".
		//
		//Thus, we mimic this by turning on and off the PTT, to simulate an ON/OFF wave.
		//
		//Set PTT true to get the ON cycle.
		vhf_tx(&vhf);

		//Go to sleep for the ON period
		HAL_Delay(FISHTRACKER_ON_TIME_MS);
//		tx_thread_sleep(FISHTRACKER_ON_TIME_TICKS);

		//Start the OFF cycle
		#if FISHTRACKER_OFF_TIME_MS > VHF_MAX_WAKE_TIME_MS
			vhf_sleep(&vhf);
			//Go to sleep again for the off period.
			tx_thread_sleep(FISHTRACKER_OFF_TIME_TICKS - VHF_MAX_WAKE_TIME_MS);
		#else
			vhf_rx(&vhf);
			//Go to sleep again for the off period.
			tx_thread_sleep(FISHTRACKER_OFF_TIME_TICKS);
		#endif
	}
}


static void calcFishtrackerDacValues(){

	for (uint8_t i = 0; i < FISHTRACKER_NUM_DAC_SAMPLES; i++){

		//Based off the IOC, our DAC is configurd to go through DMA.
		//We just want a one value output, so just make the entire array just one value
		fishtracker_dac_input[i] = 255;
	}

}
