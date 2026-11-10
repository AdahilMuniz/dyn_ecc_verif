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
uint8_t ACK_VALUE[REG_ACK_SIZE] = {0};


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
	[ACK]		= {	WRITE_ONLY,   REG_ACK_ADDR,		  REG_ACK_SIZE,			ACK_VALUE },
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


/*
 * ******************************************************************************
 *  Function Name : init_error_cbuffer
 *  @brief        : Initializes the circular buffer for error codes.
 *  @param        : RegErrorsCBTypedef *cb - Pointer to the circular buffer structure to initialize.
 *  @retval       : None
 * ******************************************************************************
 */
void nascerr_registers_init_error_cb(RegErrorsCBTypedef *cb){
	cb->head = 0;
	cb->tail = 0;
	cb->is_full = 0;
}


/*
 * ******************************************************************************
 *  Function Name : write_error_cbuffer
 *  @brief        : Writes a new error code to the circular buffer.
 *  @param        : RegErrorsCBTypedef *cb - Pointer to the circular buffer structure.
 *                  uint8_t error_code - The error code to write into the buffer.
 *  @retval       : None
 * ******************************************************************************
 */
void nascerr_registers_write_error_cb(RegErrorsCBTypedef *cb, uint8_t error_code){
	cb->buffer[cb->head] = error_code;

	if(cb->is_full == 1){
		cb->tail = (cb->tail + 1) % REG_ERRORS_BUFF_SIZE; // Overwrite oldest data.
	}

	cb->head = (cb->head + 1) % REG_ERRORS_BUFF_SIZE;
	cb->is_full = (cb->head == cb->tail) ? 1 : 0;
}


/*
 * ******************************************************************************
 *  Function Name : read_error_cbuffer
 *  @brief        : Reads the oldest error code from the circular buffer.
 *  @param        : RegErrorsCBTypedef *cb - Pointer to the circular buffer structure.
 *                  uint8_t *error_code - Pointer to store the retrieved error code.
 *  @retval       : int - Returns 1 if read is successful, 0 if the buffer is empty.
 * ******************************************************************************
 */
void nascerr_registers_read_error_cb(RegErrorsCBTypedef *cb, uint8_t *error_code){
	if (cb->head == cb->tail && cb->is_full == 0){
		return 0; // Buffer is empty
	}

	*error_code = cb->buffer[cb->tail];
	cb->tail = (cb->tail + 1) % REG_ERRORS_BUFF_SIZE;
	cb->is_full = 0;

	return 1; // Success
}




































