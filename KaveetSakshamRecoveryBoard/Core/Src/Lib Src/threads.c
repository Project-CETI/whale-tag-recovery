/*
 * tasks.c
 *
 *  Created on: Jul 6, 2023
 *      Author: Kaveet
 */

#ifndef SRC_LIB_SRC_THREADS_C_
#define SRC_LIB_SRC_THREADS_C_

#include <Lib Inc/threads.h>
#include <stdint.h>
#include "main.h"

Thread_HandleTypeDef threads[NUM_THREADS];
TX_TIMER heartbeat_timer;
TX_TIMER bootloader_check_timer;

static void priv__heartbeat(ULONG heartbeat_input);

#if USB_BOOTLOADER_ENABLED
static void priv__check_usb_boot(ULONG timer_input){
	static uint16_t consecutive = 0;
	if(HAL_GPIO_ReadPin(USB_BOOT_EN_GPIO_Port, USB_BOOT_EN_Pin) == GPIO_PIN_RESET){
		consecutive++;
	}else{
		consecutive = 0;
	}

	/*reset system*/
	if(consecutive == USB_BOOTLOADER_HOLD_TIME_SECONDS){
		HAL_NVIC_SystemReset(); //reset system;
	}
}
#endif

static void priv__heartbeat(ULONG heartbeat_input){
	for(int i = 0; i < 15; i++){
				HAL_GPIO_TogglePin(PWR_LED_NEN_GPIO_Port, PWR_LED_NEN_Pin);
				HAL_Delay(33);//flash at ~15 Hz for 1 second
	}
	 HAL_GPIO_WritePin(PWR_LED_NEN_GPIO_Port, PWR_LED_NEN_Pin, GPIO_PIN_SET);//ensure light is off after strobe
}



void threadListInit(void){
	//create bootloader timer
#if USB_BOOTLOADER_ENABLED
	tx_timer_create(
			&bootloader_check_timer,
			"USB Bootloader Check",
			priv__check_usb_boot, 0,
			1, tx_s_to_ticks(1),
			TX_AUTO_ACTIVATE
	);
#endif

	//create heartbeat timer
#if HEARTBEAT_ENABLED
	tx_timer_create(
			&heartbeat_timer,
			"Heartbeat LED Timer",
			priv__heartbeat, 0,
			tx_s_to_ticks(30), tx_s_to_ticks(HEARTRATE_SECONDS),
			TX_AUTO_ACTIVATE
	); 
#endif


	//create threads
	for (Thread index = 0; index < NUM_THREADS; index++){

		//Attach our config info (the ones statically defined in the header file)
		threads[index].config = threadConfigList[index];

		//Allocate thread stack space
		threads[index].thread_stack_start = malloc(threads[index].config.thread_stack_size);
	}
}
#endif /* SRC_LIB_SRC_TASKS_C_ */
