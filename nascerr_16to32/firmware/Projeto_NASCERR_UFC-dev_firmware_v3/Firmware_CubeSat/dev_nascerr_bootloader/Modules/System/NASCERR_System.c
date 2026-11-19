/**
 * ******************************************************************************
 *  @file    : NASCERR_Protocol.c
 *  @brief   : Manages power state transitions and communication readiness
 *  for the NASCERR system.
 *  @author  : Cicero
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */
#include "NASCERR_System.h"
#include "main.h"
#include "gpio.h"
#include "dma.h"
#include "usart.h"
#include "spi.h"
#include "iwdg.h"

/*
 * ******************************************************************************
 *  Global Variables
 * ******************************************************************************
 */

NASCERR_STATUS system_status = {
	    .OP = {
	    	.EXP.id = 0,
	    	.EXP.page = 0,
	    	.EXP.stat = OP_EXP_OK,
			.EXP.error_flag = 0,
	        .HW  = OP_HW_OK,
			.LOG = OP_LOG_OK
	    },
	    .REQ = {
			.EXP = REQ_EXP_NOREQUEST,
			.HW  = REQ_HW_NOREQUEST,
			.LOG = REQ_LOG_NOREQUEST
	    }
};

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

void nascerr_system_reinitialization();

/*
 * ******************************************************************************
 *  External Function (to be accessed through other files)
 * ******************************************************************************
 */

extern void SystemClock_Config(void);




