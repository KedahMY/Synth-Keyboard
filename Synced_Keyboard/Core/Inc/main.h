/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "keydef.h"
#include "audio.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Row0_Pin GPIO_PIN_3
#define Row0_GPIO_Port GPIOC
#define Row2_Pin GPIO_PIN_2
#define Row2_GPIO_Port GPIOA
#define Row3_Pin GPIO_PIN_3
#define Row3_GPIO_Port GPIOA
#define Row1_Pin GPIO_PIN_5
#define Row1_GPIO_Port GPIOC
#define Col2_Pin GPIO_PIN_10
#define Col2_GPIO_Port GPIOB
#define Col3_Pin GPIO_PIN_11
#define Col3_GPIO_Port GPIOB
#define Col4_Pin GPIO_PIN_12
#define Col4_GPIO_Port GPIOB
#define Col5_Pin GPIO_PIN_13
#define Col5_GPIO_Port GPIOB
#define Col6_Pin GPIO_PIN_14
#define Col6_GPIO_Port GPIOB
#define Col7_Pin GPIO_PIN_15
#define Col7_GPIO_Port GPIOB
#define Col0_Pin GPIO_PIN_8
#define Col0_GPIO_Port GPIOB
#define Col1_Pin GPIO_PIN_9
#define Col1_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
