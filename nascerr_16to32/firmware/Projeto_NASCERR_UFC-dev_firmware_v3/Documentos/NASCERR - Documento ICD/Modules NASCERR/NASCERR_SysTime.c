/**
 * ******************************************************************************
 *  @file    : NASCERR_SysTime.c
 *  @brief   : NASCERR system time.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "NASCERR_SysTime.h"

/*
 * ******************************************************************************
 *  Global Variables
 * ******************************************************************************
 */

uint32_t systimer_counter;
int32_t  systimer_delta;

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

int32_t systimer_delta_value(uint32_t timerPeriod, uint32_t timer_LastValue);

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

/******************************************************************************
 * Function Name: timer_system
 * @brief     : Timer configurável
 * @param     : *position     - Endereco do Timer
 *              *(position+1) - Acumulador
 *              *(position+2) - Status (0: Turn_off | 1: Turn_on)
 * @retval    : PARAM_STANDARD Clear - Sem erro
 *                             Set   - Estouro de Contagem
 ******************************************************************************/
PARAM_STANDARD nascerr_systimer(timer_value timerPeriod, uint32_t *position)
{
	PARAM_STANDARD value_return = Clear;
	PARAM_STANDARD status = (PARAM_STANDARD)*(position+2);

	if(status) {

		if(*position != systimer_counter){
			systimer_delta = systimer_delta_value(timerPeriod, *position);
			*position = systimer_counter;
			*(position+1) = *(position+1) + (uint32_t)systimer_delta;
		}

		if(*(position+1) > (uint32_t)timerPeriod){
			value_return = Set;
			*(position+1) = 0;
			*(position+2) = 0;
		}
	}
	else {
		// Reiniciar contagem
		*position = systimer_counter;
		*(position+1) = 1;
		*(position+2) = 1;
	}
	return value_return;
}

/******************************************************************************
 * Function Name: timer_system_reset
 * @brief  : Atualizar contador do timer
 * @param  : *position - Endereco do Timer
 * @retval : None
 ******************************************************************************/
void nascerr_systimer_reset(uint32_t *position){
	if(*(position+2) != 0){
		*position     = systimer_counter;
		*(position+1) = 0;
		*(position+2) = 0;
	}
}

/******************************************************************************
 * Function Name: timer_system_refresh
 * @brief  : Atualizar contador do timer
 * 			(Inserir na callback da camada HAL do Timer)
 * @param  : None
 * @retval : None
 ******************************************************************************/
void nascerr_systimer_refresh(void)
{
	if(++systimer_counter > (uint32_t)TIMER_COUNTER_LIMIT) systimer_counter = 0;
}

/******************************************************************************
 * Function Name: systime_delta_value
 * @brief  : Intervalo de tempo decorrido entre as verficacoes
 * @param  : None
 * @retval : Intervalo de tempo
 ******************************************************************************/
int32_t systimer_delta_value(uint32_t timerPeriod, uint32_t timer_LastValue)
{
	int32_t delta;

	if(systimer_counter >= timer_LastValue) {
		delta = (int32_t)systimer_counter - (int32_t)timer_LastValue;
	}
	else {
		delta = ((int32_t)TIMER_COUNTER_LIMIT - (int32_t)timer_LastValue) + (int32_t)systimer_counter;
	}

	return delta;
}
