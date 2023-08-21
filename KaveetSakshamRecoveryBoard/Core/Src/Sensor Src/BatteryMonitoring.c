/*
 * BatteryMonitoring.c
 *
 *  Created on: Aug 21, 2023
 *      Author: Kaveet
 */

#include "Sensor Inc/BatteryMonitoring.h"
#include "main.h"
#include "stm32u5xx_hal_adc.h"
//External variables
extern ADC_HandleTypeDef hadc1;

//Main thread entry for battery monitoring function
void battery_monitor_thread_entry(ULONG thread_input){


	uint32_t data[10] = {0};

	//HAL_ADC_Start_IT(&hadc1, data, 10);

	while (1){
		battery_monitor_get_raw_adc_data();
	}
}

uint32_t battery_monitor_get_raw_adc_data(){

	//Start the conversion
	//HAL_Delay(100);

	volatile HAL_StatusTypeDef ret = HAL_ERROR;

	while (ret != HAL_OK){
		ret = HAL_ADC_Start(&hadc1);
	}
	//Wait for completion
	ret = HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);

	//Read the value
	uint32_t raw_reading = HAL_ADC_GetValue(&hadc1);

	//Stop converting
	HAL_ADC_Stop(&hadc1);

	HAL_Delay(500);

	return raw_reading;
}

//Function to call to get the true (fully scaled) battery voltage, form 0-7.5V.
float battery_monitor_get_true_voltage(){

}
