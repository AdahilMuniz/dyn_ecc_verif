/**
 * ******************************************************************************
 *  @file    : NASCERR_Tasks.h
 *  @brief   : Header for NASCERR_Tasks.c file.
 *  @author  : Marcus Anderson; Joao Manoel
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "main.h"
#include "NASCERR_System.h"
#include "NASCERR_Protocol.h"
#include "NASCERR_Registers.h"
#include "NASCERR_Control.h"
#include "NASCERR_Experiment.h"
#include "NASCERR_ImageProcessing.h"
#include "i2c_slave.h"
/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

#define MAX_COMMAND_BUFFER      3U /* Numero maximo de comando que pode receber */

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

void receive_dataResp(void *pvParameters);

void send_dataResp(void *pvParameters);

void nascerr_experiment(void *pvParameters);
