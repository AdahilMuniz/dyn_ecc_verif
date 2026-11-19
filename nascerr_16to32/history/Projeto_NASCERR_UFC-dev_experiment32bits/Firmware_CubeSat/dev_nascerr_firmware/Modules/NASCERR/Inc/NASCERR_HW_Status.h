/*
 * NASCERR_HW_Status.h
 *
 *  Created on: Dec 17, 2024
 *      Author: dan-alencar
 */

#ifndef NASCERR_INC_NASCERR_HW_STATUS_H_
#define NASCERR_INC_NASCERR_HW_STATUS_H_

#include "main.h"
#include "adc.h"
#include "tim.h"
#include "gpio.h"

#define BUFFER_SIZE 6

void NASCERR_ADC_Start(ADC_HandleTypeDef hadc, uint32_t adc_buf[BUFFER_SIZE]);
void NASCERR_ADC_Stop(ADC_HandleTypeDef hadc);
void NASCERR_TIM_Start(TIM_HandleTypeDef htim);
void NASCERR_TIM_Stop(TIM_HandleTypeDef htim);
//void NASCERR_HWRead_Start(ADC_HandleTypeDef hadc, TIM_HandleTypeDef htim);
void NASCERR_HWRead_Start(ADC_HandleTypeDef hadc, TIM_HandleTypeDef htim, uint32_t adc_buf[BUFFER_SIZE]);
void NASCERR_HWRead_Stop(ADC_HandleTypeDef hadc, TIM_HandleTypeDef htim);

#endif /* NASCERR_INC_NASCERR_HW_STATUS_H_ */
