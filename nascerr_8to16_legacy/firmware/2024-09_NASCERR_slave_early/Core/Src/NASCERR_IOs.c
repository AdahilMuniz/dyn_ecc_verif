/*
 * NASCERR_IOs.c
 *
 *  Created on: Sep 5, 2024
 *      Author: Clara
 */

#include "stm32l4xx_hal.h"


void nascerr_io_pin_set(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin){
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, 1);
}


void nascerr_io_pin_reset(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin){
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, 0);
}

void nascerr_io_delay(uint32_t delay){
	HAL_Delay(delay);
}
