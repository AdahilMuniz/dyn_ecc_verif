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
#include "NASCERR_Config.h"

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
typedef enum
{
	ACK_IDLE = 0x0,
	ACK_WRITE_RECEIVED,
	ACK_CMD_PROCESSING,
	ACK_SUCESS,
	ACK_FAIL
}ACK_PHASE;

/* Operating Logs Exp */
typedef enum
{
	OP_EXP_START = 0xA0,
	OP_EXP_IDLE,
	OP_EXP_RUNNING,
	OP_EXP_BUSY,
	OP_EXP_FAIL,
	OP_EXP_MAX
} OP_EXPERIMENT;

/* Operating Logs HW */
typedef enum
{
	OP_HW_START = 0XB0,
	OP_HW_OK,
	OP_HW_SLEEP,
	OP_HW_BUSY,
	OP_HW_FAIL,
	OP_HW_MAX
} OP_HARDWARE;

/* Operating Logs Structure */
typedef struct
{
	OP_EXPERIMENT 	EXP;
	OP_HARDWARE		HW;
} OP;

/* Request Logs Exp */
typedef enum
{
	REQ_EXP_START = 0XA0,
	REQ_EXP_NO_REQUEST,
	REQ_EXP_CMD_RECEIVED,
	REQ_EXP_READ_PROCESSING,
	REQ_EXP_TX_RESULT,
	REQ_EXP_FAIL,
	REQ_EXP_MAX
} REQ_EXPERIMENT;

/* Request Logs HW */
typedef enum
{
	REQ_HW_START = 0XB0,
	REQ_HW_NO_REQUEST,
	REQ_HW_TX_TEMP,
	REQ_HW_TX_CURRENTS,
	REQ_HW_TX_POWERS,
	REQ_HW_TX_TOT_PWR,
	REQ_HW_MAX
} REQ_HARDWARE;

/* Request Logs Structure */
typedef struct
{
	REQ_EXPERIMENT 	EXP;
	REQ_HARDWARE 	HW;
} REQ;

/* Status Structure */
typedef struct __attribute__((packed))
{
	OP	OP;
	REQ REQ;
	uint8_t FAIL;
} NASCERR_STATUS;

typedef struct __attribute__((packed)) {
    uint32_t firmware_version;          		/*< versão do firmware ativo				*/
    uint32_t other_firmware_version;    		/*< versão do firmware no outro bank 		*/
    uint8_t  flags;                    			/*< BootFlags 								*/
    uint8_t  last_error;                		/*< código de erro 							*/
    uint16_t last_packet_id;            		/*< último pacote 							*/
    uint8_t  current_region;            		/*< 0xAA=app1 | 0xBB=app2					*/
    uint8_t  reserved;                  		/*< alinhamento/futuro 						*/
} NASCERR_BOOT_STATUS;

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
