/**
 * ******************************************************************************
 *  @file    : NASCERR_Protocol.h
 *  @brief   : Header for NASCERR_Protocol.c file.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

#ifndef INC_NASCERR_PROTOCOL_H_
#define INC_NASCERR_PROTOCOL_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */
#include "NASCERR_System.h"
#include "NASCERR_Registers.h"
#include "NASCERR_SysTime.h"
#include "NASCERR_Control.h"
#include "i2c_slave.h"
#include "util.h"
#include "main.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

#define COMM_BUFFER_SIZE   						1024		/* > Communication buffer size  */
#define COMM_TIMEOUT   				    		TIMER_10ms  /* > Communication timeout [ms] */

 /* Define function send/receive data */
#define func_send_channel(pData, Size)			i2c_slave_send(pData, Size)
#define func_receive_channel(pData, Size)		i2c_slave_receive(pData, Size)
/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/* Definition for bus state */
typedef enum
{
	RECEIVING_DATA = 0,     /* > Receiving data                */
	TRANSMITTING_DATA,    	/* > Transmitting data             */
	LISTENING_MODE,   	    /* > Listening for address or data */
} BusTypeDef;


/* Status Communications */
typedef enum
{
	COMMUNICATION_IDLE = 0,
	COMMUNICATION_INIT,
	COMMUNICATION_SUCCESS,
	COMMUNICATION_INPROCESS,
	COMMUNICATION_FAILURE,
	PROCESSING_DATA,
	DATA_CORRUPTION,
} StatusCommTypeDef;

/* Protocol states */
typedef enum
{
	PROTOCOL_IDLE = 0,
	PROTOCOL_REGISTER,
	PROTOCOL_LENGTH_DATA_HI,
	PROTOCOL_LENGTH_DATA_LO,
	PROTOCOL_RECEIVE_DATA,
	PROTOCOL_SEND_DATA,
	PROTOCOL_PROCESS_DATA,
	PROTOCOL_FAIL,
} ProtocolStateTypeDef;

/* Communication channel structure */
typedef struct
{
	BusTypeDef     		 status_bus;
	RegistersTypeDef     command_reg;
	StatusCommTypeDef    status_comm;
	ProtocolStateTypeDef state;
	uint8_t              addr;
	uint16_t             data_length;
	uint16_t             current_idx;
	uint8_t              *data_rxbuff;
	uint8_t              *data_txbuff;
} ProtocolTypeDef;


/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

void nascerr_protocol_init(ProtocolTypeDef *comm_config);

void nascerr_protocol_commInit(ProtocolTypeDef *comm_config);

StatusCommTypeDef nascerr_protocol_poll(ProtocolTypeDef *comm_config, BusTypeDef status_bus); /* TODO: FALTA IMPLEMENTAR ALGUNS COMANDOS */

StatusCommTypeDef nascerr_protocol_send(uint8_t *pData, uint16_t Size);

StatusCommTypeDef nascerr_protocol_receive(uint8_t *pData, uint16_t Size);

StatusCommTypeDef nascerr_protocol_processData(RegistersTypeDef new_register); /* TODO: FALTA IMPLEMENTAR ALGUNS COMANDOS */

NASCERR_STATUS nascerr_protocol_getStatusSystem(void);

NASCERR_MODE nascerr_protocol_getModeSystem(void);

/* **************************************************************************** */
/* 						FUNCTIONS FOR UPPER LAYERS								*/
/* **************************************************************************** */

uint8_t nascerr_protocol_requestTxResult(NASCERR_RESPONSE response);

uint8_t nascerr_protocol_receivedNewCommand(void);

NASCERR_COMMANDS nascerr_protocol_processCommand(void); /* TODO: FALTA IMPLEMENTAR ALGUNS COMANDOS */

uint8_t nascerr_protocol_getDataCommand(uint8_t *pData);

/* **************************************************************************** */
#endif /* INC_NASCERR_PROTOCOL_H_ */
