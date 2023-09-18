/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void MX_TIM2_Fake_Init(uint8_t newPeriod);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define GPS_NEN_Pin GPIO_PIN_1
#define GPS_NEN_GPIO_Port GPIOC
#define VHF_TX_Pin GPIO_PIN_0
#define VHF_TX_GPIO_Port GPIOA
#define VHF_RX_Pin GPIO_PIN_1
#define VHF_RX_GPIO_Port GPIOA
#define VHF_PTT_Pin GPIO_PIN_2
#define VHF_PTT_GPIO_Port GPIOA
#define EXT_RX_Pin GPIO_PIN_3
#define EXT_RX_GPIO_Port GPIOA
#define APRS_PD_Pin GPIO_PIN_5
#define APRS_PD_GPIO_Port GPIOA
#define APRS_H_L_Pin GPIO_PIN_6
#define APRS_H_L_GPIO_Port GPIOA
#define GPS_TX_Pin GPIO_PIN_4
#define GPS_TX_GPIO_Port GPIOC
#define GPS_RX_Pin GPIO_PIN_5
#define GPS_RX_GPIO_Port GPIOC
#define GPS_EXTINT_Pin GPIO_PIN_0
#define GPS_EXTINT_GPIO_Port GPIOB
#define PWR_LED_NEN_Pin GPIO_PIN_2
#define PWR_LED_NEN_GPIO_Port GPIOB
#define VSYS_SENSE_Pin GPIO_PIN_11
#define VSYS_SENSE_GPIO_Port GPIOD
#define USB_BOOT_EN_Pin GPIO_PIN_10
#define USB_BOOT_EN_GPIO_Port GPIOA
#define EXT_TX_Pin GPIO_PIN_5
#define EXT_TX_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
