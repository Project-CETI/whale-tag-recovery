/*
 * BatteryMonitoring.c
 *
 *  Created on: Aug 21, 2023
 *      Author: Kaveet
 */

#include "Sensor Inc/BatteryMonitoring.h"
#include "Lib Inc/state_machine.h"
#include "main.h"
#include "stm32u5xx_hal_adc.h"

//External variables
extern ADC_HandleTypeDef hadc4;
extern TX_EVENT_FLAGS_GROUP state_machine_event_flags_group;

float voltage_mon = 0;

//Main thread entry for battery monitoring function
void battery_monitor_thread_entry(ULONG thread_input){


	while (1){

		//Get our battery voltage value
		voltage_mon = battery_monitor_get_true_voltage();

		//If our voltage is too low (low battery detected)
		if (voltage_mon < BATT_MON_LOW_VOLTAGE_THRESHOLD){
			tx_event_flags_set(&state_machine_event_flags_group, STATE_CRITICAL_LOW_BATTERY_FLAG, TX_OR);
		}

		//Sleep and repeat the process once woken up
		tx_thread_sleep(BATT_MON_SLEEP_TIME_TICKS);
	}
}

uint32_t battery_monitor_get_raw_adc_data(){

	volatile HAL_StatusTypeDef ret = HAL_ERROR;

	while (ret != HAL_OK){
		ret = HAL_ADC_Start(&hadc4);
	}
	//Wait for completion
	ret = HAL_ADC_PollForConversion(&hadc4, HAL_MAX_DELAY);

	//Read the value
	uint32_t raw_reading = HAL_ADC_GetValue(&hadc4);

	//Stop converting
	HAL_ADC_Stop(&hadc4);

	return raw_reading;
}

//Function to call to get the true (fully scaled) battery voltage, form 0-7.5V.
float battery_monitor_get_true_voltage(){

	//Call to get our raw data first
	uint32_t raw = battery_monitor_get_raw_adc_data();

	//Use our macros to scale the voltage appropriately
	return batt_true_voltage(batt_to_analog(raw));
}
