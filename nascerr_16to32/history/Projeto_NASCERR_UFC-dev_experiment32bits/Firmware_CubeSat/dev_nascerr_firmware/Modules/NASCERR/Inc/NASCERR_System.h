/**
 * ******************************************************************************
 *  @file    : nascerr_system.h
 *  @brief   : Header for system settings.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

#ifndef INC_NASCERR_SYSTEM_H_
#define INC_NASCERR_SYSTEM_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include <stdint.h>


/* Define DEBUG_MODE to enable debug messages */
#define 	MOCK_COMMAND		1
#define 	MOCK_EXPERIMENT		0
#define 	ENABLE_LOGS 		1
#define   	SOFTWARE_DEBUG   	1
#define   	ERROR_BY_PACKAGE  	16 // bytes por pacote: 128 + 24 = 152 bytes
/*
 * ******************************************************************************
 *  SYSTEM Params
 * ******************************************************************************
 */

/* Parameter Standard */
typedef enum
{
    Clear = 0,
    Set = 1,
	Open = 0,
    Close = 1,
	Min = 0,
	Max = 1,
    Turn_off = 0,
    Turn_on = 1,
	Action_Failed = 0,
	Action_Succeeded = 1,
} PARAM_STANDARD;

/* System Status */
typedef enum
{
	ERR_UNKNOWN = 0,
	STATUS_OK,
	STATUS_SUCCESS,
	ERR_INIT,
	ERR_BUSY,
	ERR_COMM,
	ERR_IO,
	ERR_TIMEOUT,
	ERR_FLASH,
	ERR_FLASH_ERASE
} SYSTEM_STATUS;

/*
 * ******************************************************************************
 *  NASCERR Params
 * ******************************************************************************
 */

/* Operating Logs */
typedef enum
{
	OP_OK = 0xAA, //TODO: MUDAR PARA 0 (ONLY DEBUG)
	OP_EXPERIMENT,
	OP_RECEIVE_COMMAND,
	OP_TRANSMIT_RESPONSE,
	OP_TRANSMIT_FINISH,
	OP_BUSY,
	OP_FINISH,
	OP_FAIL,
	OP_MAX,
} OP_LOGS;


typedef enum
{
	WARN_MEMORY_FAULT = 0xAA,
	ERR_EXP_PARAM_DLEN,
	ERR_EXP_PARAM_MEM,
	ERR_EXP_PARAM_REP,
	ERR_EXP_PARAM_DELAY,
}ERROR_CODES;


/* Request Logs */
typedef enum
{
	REQ_NOREQUEST = 0xAA, //TODO: MUDAR PARA 0 (ONLY DEBUG)
	REQ_TXRESULT,
	REQ_GPS,
	REQ_TIME,
	REQ_TEMP,
	REQ_MAX,
} REQ_LOGS;

/* NASCERR Status Logs */
typedef struct
{
	OP_LOGS  OP_LOGS;
	REQ_LOGS REQ_LOGS;
} NASCERR_STATUS;


typedef enum
{
	UNDEFINED_MOD = 0,
	BYPASS_1,
	BYPASS_2,
	TBEC,
	TBEC_1_TO_3,
	TBEC_1_TO_7,
	BYPASS_1_TO_7,
	MRSC,
} NASCERR_MODE;

/* NASCERR COMMANDS */
typedef enum
{
	NO_COMMANDS = 0,
	SEND_STATUS,
	DATA_RECEIVED,
	SEND_MODE,
	COMMAND_TO_SLEEP,
	COMMAND_TO_WAKEUP,
	GPS_DATA_RECEIVED,
	TIME_DATA_RECEIVED,
	TEMP_DATA_RECEIVED,
	ACK_RECEIVED,
} NASCERR_COMMANDS;

/* BOOL definitions */
#define false       (0)
#define true        (1)
#define undefined   (2)
#ifndef NULL
#define NULL        (0)
#endif /* Define of NULL */

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

#endif /* INC_NASCERR_SYSTEM_H_ */
