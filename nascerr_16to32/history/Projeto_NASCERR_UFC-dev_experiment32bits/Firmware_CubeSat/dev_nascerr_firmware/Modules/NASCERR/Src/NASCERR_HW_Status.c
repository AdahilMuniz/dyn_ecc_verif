/*
 * NASCERR_HW_Status.c
 *
 *  Created on: Dec 17, 2024
 *      Author: dan-alencar
 */

#include "NASCERR_HW_Status.h"

//uint32_t adc_buf[BUFFER_SIZE], adc_val = 0;

void NASCERR_ADC_Start(ADC_HandleTypeDef hadc, uint32_t adc_buf[BUFFER_SIZE]){
	if (HAL_ADCEx_Calibration_Start(&hadc, ADC_SINGLE_ENDED) != HAL_OK)
		Error_Handler();

	if (HAL_ADC_Start_DMA(&hadc, adc_buf, BUFFER_SIZE) != HAL_OK)
		Error_Handler();
}

void NASCERR_ADC_Stop(ADC_HandleTypeDef hadc){
	if (HAL_ADC_Stop_DMA(&hadc) != HAL_OK)
			Error_Handler();
}

void NASCERR_TIM_Start(TIM_HandleTypeDef htim){
	if (HAL_TIM_Base_Start(&htim) != HAL_OK)
			Error_Handler();
}
void NASCERR_TIM_Stop(TIM_HandleTypeDef htim){
	if (HAL_TIM_Base_Stop(&htim) != HAL_OK)
				Error_Handler();
}

void NASCERR_HWRead_Start(ADC_HandleTypeDef hadc, TIM_HandleTypeDef htim, uint32_t adc_buf[BUFFER_SIZE]){
	NASCERR_TIM_Start(htim);
	NASCERR_ADC_Start(hadc, adc_buf);
	HAL_GPIO_WritePin(GPIOB, DEBUG_LD1_Pin, GPIO_PIN_SET);
}

void NASCERR_HWRead_Stop(ADC_HandleTypeDef hadc, TIM_HandleTypeDef htim){
	NASCERR_TIM_Stop(htim);
	NASCERR_ADC_Stop(hadc);
	HAL_GPIO_WritePin(GPIOB, DEBUG_LD1_Pin, GPIO_PIN_RESET);
}

