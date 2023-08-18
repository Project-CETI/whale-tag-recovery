/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_threadx.c
  * @author  MCD Application Team
  * @brief   ThreadX applicative file
  ******************************************************************************
    * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_threadx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Lib Inc/threads.h"
#include "Recovery Inc/Aprs.h"
#include "Recovery Inc/FishTracker.h"
#include <stdint.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
extern Thread_HandleTypeDef threads[NUM_THREADS];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  /* USER CODE BEGIN App_ThreadX_MEM_POOL */

	//Initialize thread list so we can create threads
	threadListInit();

	//loop through each thread in the list, allocate the memory and create the stack
	for (uint8_t index = 0; index < NUM_THREADS; index++){

	  VOID * pointer = threads[index].thread_stack_start;
	// Allocate memory pool
	  TX_BYTE_POOL* byte_pool = (TX_BYTE_POOL*) memory_ptr;
	  ret = tx_byte_allocate(byte_pool, &pointer, threads[index].config.thread_stack_size, TX_NO_WAIT);

  /* USER CODE END App_ThreadX_MEM_POOL */
  /* USER CODE BEGIN App_ThreadX_Init */
	  tx_thread_create(
			  &threads[index].thread,
			  threads[index].config.thread_name,
			  threads[index].config.thread_entry_function,
			  threads[index].config.thread_input,
			  threads[index].thread_stack_start,
			  threads[index].config.thread_stack_size,
			  threads[index].config.priority,
			  threads[index].config.preempt_threshold,
			  threads[index].config.timeslice,
			  threads[index].config.start);
	}

	//TEMP: start the correct thread
	if (USE_FISHTRACKER){
		tx_thread_resume(&threads[FISHTRACKER_THREAD].thread);
	}
	else {
		tx_thread_resume(&threads[GPS_COLLECTION_THREAD].thread);
		tx_thread_resume(&threads[PI_COMMS_RX_THREAD].thread);
	}
  /* USER CODE END App_ThreadX_Init */

  return ret;
}

  /**
  * @brief  MX_ThreadX_Init
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN  Before_Kernel_Start */

  /* USER CODE END  Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN  Kernel_Start_Error */

  /* USER CODE END  Kernel_Start_Error */
}

/**
  * @brief  App_ThreadX_LowPower_Timer_Setup
  * @param  count : TX timer count
  * @retval None
  */
void App_ThreadX_LowPower_Timer_Setup(ULONG count)
{
  /* USER CODE BEGIN  App_ThreadX_LowPower_Timer_Setup */

  /* USER CODE END  App_ThreadX_LowPower_Timer_Setup */
}

/**
  * @brief  App_ThreadX_LowPower_Enter
  * @param  None
  * @retval None
  */
void App_ThreadX_LowPower_Enter(void)
{
  /* USER CODE BEGIN  App_ThreadX_LowPower_Enter */

  /* USER CODE END  App_ThreadX_LowPower_Enter */
}

/**
  * @brief  App_ThreadX_LowPower_Exit
  * @param  None
  * @retval None
  */
void App_ThreadX_LowPower_Exit(void)
{
  /* USER CODE BEGIN  App_ThreadX_LowPower_Exit */

  /* USER CODE END  App_ThreadX_LowPower_Exit */
}

/**
  * @brief  App_ThreadX_LowPower_Timer_Adjust
  * @param  None
  * @retval Amount of time (in ticks)
  */
ULONG App_ThreadX_LowPower_Timer_Adjust(void)
{
  /* USER CODE BEGIN  App_ThreadX_LowPower_Timer_Adjust */
  return 0;
  /* USER CODE END  App_ThreadX_LowPower_Timer_Adjust */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
