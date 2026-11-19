/**
 * ******************************************************************************
 *  @file    : NASCERR_Protocol.c
 *  @brief   : NASCERR Registers
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */
#include "NASCERR_Registers.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

/* Allocation of memory for register values */
uint8_t STAT_VALUE[REG_STAT_SIZE] = {OP_OK, REQ_NOREQUEST};
uint8_t IDX_VALUE[REG_IDX_SIZE] = {0};
uint8_t TXB_VALUE[REG_TXB_SIZE] = {0};
uint8_t CMD_VALUE[REG_CMD_SIZE] = {0}; //verificar se está recebendo os dados
uint8_t MODO_VALUE[REG_MODO_SIZE] = {UNDEFINED_MOD};
uint8_t SLEEP_VALUE[REG_SLEEP_SIZE] = {0};/* TODO: IMPLEMENTAR */
uint8_t WAKEUP_VALUE[REG_WAKEUP_SIZE] = {0};/* TODO: IMPLEMENTAR */
uint8_t GPS_VALUE[REG_GPS_SIZE] = {0};/* TODO: IMPLEMENTAR */
uint8_t TIME_VALUE[REG_TIME_SIZE] = {0};/* TODO: IMPLEMENTAR */
uint8_t TEMP_VALUE[REG_TEMP_SIZE] = {0};/* TODO: IMPLEMENTAR */
uint8_t ERROR_OP_VALUE[REG_ERROR_OP_SIZE] = {0};/* TODO: IMPLEMENTAR */
uint8_t ERROR_EPS_VALUE[REG_ERROR_EPS_SIZE] = {0};/* TODO: IMPLEMENTAR */

/* Register values */
volatile RegStructTypeDef g_register_data[] =
{
    [STAT]      = { READ_ONLY,    REG_STAT_ADDR,  	  REG_STAT_SIZE,        STAT_VALUE },
    [IDX]       = { READ_ONLY,    REG_IDX_ADDR,    	  REG_IDX_SIZE,         IDX_VALUE },
    [TXB]       = { READ_ONLY,    REG_TXB_ADDR,    	  REG_TXB_SIZE,         TXB_VALUE },
    [CMD]       = { WRITE_ONLY,   REG_CMD_ADDR,    	  REG_CMD_SIZE,         CMD_VALUE },
    [MODO]      = { READ_ONLY,    REG_MODO_ADDR,   	  REG_MODO_SIZE,        MODO_VALUE },
    [SLEEP]     = { WRITE_ONLY,   REG_SLEEP_ADDR,  	  REG_SLEEP_SIZE,       SLEEP_VALUE },
    [WAKEUP]    = { WRITE_ONLY,   REG_WAKEUP_ADDR, 	  REG_WAKEUP_SIZE,      WAKEUP_VALUE },
    [GPS]       = { WRITE_ONLY,   REG_GPS_ADDR,    	  REG_GPS_SIZE,         GPS_VALUE },
    [TIME]      = { WRITE_ONLY,   REG_TIME_ADDR,   	  REG_TIME_SIZE,        TIME_VALUE },
    [TEMP]      = { WRITE_ONLY,   REG_TEMP_ADDR,   	  REG_TEMP_SIZE,        TEMP_VALUE },
    [ERROR_OP]  = { READ_ONLY,    REG_ERROR_OP_ADDR,  REG_ERROR_OP_SIZE,    ERROR_OP_VALUE },
    [ERROR_EPS] = { READ_ONLY,    REG_ERROR_EPS_ADDR, REG_ERROR_EPS_SIZE,   ERROR_EPS_VALUE },
};
/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name : nascerr_registers_changeStatus
 *  @brief        : Changes the status of the NASCERR registers.
 *  @param        : OP_LOGS new_operation - The new operation status to be set.
 *                  REQ_LOGS new_request - The new request status to be set.
 *  @retval       : NASCERR_STATUS - The updated status of the registers.
 * ******************************************************************************
 */
NASCERR_STATUS nascerr_registers_changeStatus(OP_LOGS new_operation, REQ_LOGS new_request)
{
	NASCERR_STATUS status;

	STAT_VALUE[0] = new_operation;
	STAT_VALUE[1] = new_request;

	status.OP_LOGS = STAT_VALUE[0];
	status.REQ_LOGS = STAT_VALUE[1];

	return status;
}

