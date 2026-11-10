/**
 * ******************************************************************************
 *  @file    : NASCERR_SysTime.h
 *  @brief   : Header for NASCERR_SysTime.c file.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

#ifndef INC_NASCERR_SYSTIME_H_
#define INC_NASCERR_SYSTIME_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */
#include "NASCERR_System.h"
#include "main.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

#define TIMER_COUNTER_LIMIT	2000000000 /* MAX VALUE TIMER COUNT */

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/* Time period default parameter */
typedef enum
{
	TIMER_1ms   = 1,
	TIMER_5ms   = 5,
    TIMER_10ms  = 10,
	TIMER_50ms  = 50,
	TIMER_100ms = 100,
	TIMER_250ms = 250,
	TIMER_500ms = 500,
	TIMER_1s    = 1000,
	TIMER_5s    = 5000,
	TIMER_10s   = 10000,
} timer_value;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

PARAM_STANDARD nascerr_systimer(timer_value timerPeriod, uint32_t *position);
void nascerr_systimer_reset(uint32_t *position);
void nascerr_systimer_refresh(void);

#endif /* INC_NASCERR_SYSTIME_H_ */
