/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "NASCERR_SysTime.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "NASCERR_Util.h"
#include "NASCERR_HW_Status.h"
//#include "NASCERR_Tasks.h"
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

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DEBUG_LD2_Pin GPIO_PIN_7
#define DEBUG_LD2_GPIO_Port GPIOB
#define DEBUG_LD1_Pin GPIO_PIN_6
#define DEBUG_LD1_GPIO_Port GPIOB
#define MCU_SPI3_CS_Pin GPIO_PIN_12
#define MCU_SPI3_CS_GPIO_Port GPIOG
#define CURRENT_2V5_Pin GPIO_PIN_2
#define CURRENT_2V5_GPIO_Port GPIOC
#define CURRENT_3V3_Pin GPIO_PIN_3
#define CURRENT_3V3_GPIO_Port GPIOC
#define CURRENT_3V3_FPGA_Pin GPIO_PIN_4
#define CURRENT_3V3_FPGA_GPIO_Port GPIOC
#define CURRENT_1V2_Pin GPIO_PIN_0
#define CURRENT_1V2_GPIO_Port GPIOA
#define TEMP_SRAM_Pin GPIO_PIN_3
#define TEMP_SRAM_GPIO_Port GPIOA
#define CURRENT_2V5_VPP_Pin GPIO_PIN_5
#define CURRENT_2V5_VPP_GPIO_Port GPIOC

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
