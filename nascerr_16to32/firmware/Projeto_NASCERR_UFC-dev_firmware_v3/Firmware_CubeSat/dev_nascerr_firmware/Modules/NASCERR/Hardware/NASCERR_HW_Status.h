/*
 * ******************************************************************************
 *  @file    : NASCERR_HW_Status.c
 *  @brief   : Header for NASCERR_HW_Status.c file.
 *  @author  : Cicero Oliveira
 * ******************************************************************************
 */

#ifndef NASCERR_INC_NASCERR_HW_STATUS_H_
#define NASCERR_INC_NASCERR_HW_STATUS_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "NASCERR_Registers.h"
#include "main.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

#define BUFFER_SIZE 		6
#define WINDOW_SIZE 		8
#define LUT_SIZE        	256
#define ADC_MAX_VALUE   	4095
#define SHIFT_FOR_INDEX 	4


#define shift_size 6
#define buf_size   64 // 2^6


/* Fail FLAGS */
#define HW_FAIL_1V2         (1U << 0)
#define HW_FAIL_2V5         (1U << 1)
#define HW_FAIL_2V5vpp      (1U << 2)
#define HW_FAIL_3V3         (1U << 3)
#define HW_FAIL_3V3_FPGA    (1U << 4)
#define HW_FAIL_TEMPERATURE (1U << 5)


/* Corrente 1V2 */
#define BETA_1V2                     		3		//0.2227f é multiplicado por um valor escalado em 100
#define INTERCEPT_1V2                		53440	//5.3441f
#define STDDEV_1V2                  		4621	//4.6216f
#define MARGIN_INTERCEPT_1V2         		(INTERCEPT_1V2 + 4 * STDDEV_1V2)
#define MARGIN_25_1V2 						BETA_1V2 * 2500  + MARGIN_INTERCEPT_1V2
#define MARGIN_85_1V2 						BETA_1V2 * 8500  + MARGIN_INTERCEPT_1V2

/* Corrente 2V5_VPP */
#define INTERCEPT_2V5_VPP           		 15300	//0.3f

/* Corrente 2V5 efetiva (cenario real) */
#define BETA_2V5_EFFECTIVE          		3     //0.279973f é multiplicado por um valor escalado em 100
#define INTERCEPT_2V5_EFFECTIVE      		8877	//8.877914f
#define STDDEV_2V5_EFFECTIVE         		770		//0.770644f
#define MARGIN_INTERCEPT_2V5_EFFECTIVE		(INTERCEPT_2V5_EFFECTIVE + 4 * STDDEV_2V5_EFFECTIVE)
#define MARGIN_25_2V5_EFFECTIVE      		(BETA_2V5_EFFECTIVE * 2500 + MARGIN_INTERCEPT_2V5_EFFECTIVE)
#define MARGIN_85_2V5_EFFECTIVE      		(BETA_2V5_EFFECTIVE * 8500 + MARGIN_INTERCEPT_2V5_EFFECTIVE)

/* Corrente 2V5 para analise de consumo */
#define BETA_2V5_BYPASS                		296		//0.296524f
#define INTERCEPT_2V5_BYPASS           		3846	//3.846582f

/* Corrente 2V5 para analise de consumo */
#define BETA_2V5_BOTH_ON               		280     //0.279973f
#define INTERCEPT_2V5_BOTH_ON          		8877	//8.877914f

/* Corrente 3V3_FPGA */
#define INTERCEPT_3V3_FPGA             		60000 //60.0f

/* Corrente 3V3 microcontrolador */
#define INTERCEPT_3V3                  		65000 //50.0f

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

typedef struct
{
	float	temp;
	float   currents[5];
	float 	powers[5];
	float	total_power;
} NASCERR_HW_RESPONSE;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

void NASCERR_HWStart(ADC_HandleTypeDef hadc, TIM_HandleTypeDef htim);
void NASCERR_HWStop(ADC_HandleTypeDef hadc, TIM_HandleTypeDef htim);
void NASCERR_HWScan(void);
uint32_t* nascerr_hw_adc_convert_to_currents(void);
int nascerr_hw_adc_convert_to_temperature(void);
NASCERR_HW_RESPONSE NASCERR_HWRead(void);

uint8_t nascerr_hw_processHWStatusReq(void);
uint8_t nascerr_hw_check_limits();
uint8_t nascerr_hw_check_sleep(void);
uint8_t nascerr_hw_check_wakeup(void);
//about economy of energy
void nascerr_hw_shutdown(void);
void nascerr_hw_waking_up(void);
//check error
uint8_t nascerr_hw_get_fail_flag(void);
//Control of when convertions shall happen
void nascerr_hw_adc_covertion_pause(void);
void nascerr_hw_adc_covertion_unpause(void);



#endif /* NASCERR_INC_NASCERR_HW_STATUS_H_ */
