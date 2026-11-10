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
extern NASCERR_STATUS system_status;

/* Allocation of memory for register values */
uint8_t STAT_VALUE[REG_STAT_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, OP_EXP_OK, REQ_EXP_NOREQUEST, OP_HW_OK, REQ_HW_NOREQUEST};
uint8_t IDX_VALUE[REG_IDX_SIZE] = {4, 0, 0, 0};
uint8_t TXB_VALUE[REG_TXB_SIZE] = {0};
uint8_t CMD_VALUE[REG_CMD_SIZE] = {0};
uint8_t SLEEP_VALUE[REG_SLEEP_SIZE] = {0, 0, 0};
uint8_t WAKEUP_VALUE[REG_WAKEUP_SIZE] = {0, 0, 0};
uint8_t GPS_VALUE[REG_GPS_SIZE] = {0};
uint8_t LOG_VALUE[REG_LOG_SIZE] = {0};
uint8_t ACK_VALUE[REG_ACK_SIZE] = {0};
uint8_t HW_STAT_VALUE[REG_HW_STAT_SIZE] = {0};
uint8_t HW_TXB_VALUE[REG_HW_TXB_SIZE] = {0};
uint8_t BOOT_DATA_VALUE[REG_DATA_BOOT_SIZE] = {0};
uint8_t BOOT_STAT_VALUE[REG_STAT_BOOT_SIZE] = {0, 0, 0, 0, 0};
uint8_t EXP_STAT_VALUE[REG_EXP_STAT_SIZE]     = { 0 };
uint8_t EXP_SUM_IDX_VALUE[REG_EXP_SUM_IDX_SIZE] = { 0 };
uint8_t EXP_SUM_DATA_VALUE[REG_EXP_SUM_DATA_SIZE] = { 0 };
uint8_t DB_CLEAR_VALUE[REG_DB_CLEAR_SIZE] = { 0 };



/* Register values */
volatile RegStructTypeDef g_register_data[] =
{
    [STAT]        = { READ_ONLY,    REG_STAT_ADDR,  	  	REG_STAT_SIZE,        STAT_VALUE },
    [IDX]         = { READ_ONLY,    REG_IDX_ADDR,    	  	REG_IDX_SIZE,         IDX_VALUE },
    [TXB]         = { READ_ONLY,    REG_TXB_ADDR,    	  	REG_TXB_SIZE,         TXB_VALUE },
    [CMD]         = { WRITE_ONLY,   REG_CMD_ADDR,    	  	REG_CMD_SIZE,         CMD_VALUE },
    [SLEEP]       = { WRITE_ONLY,   REG_SLEEP_ADDR,  	  	REG_SLEEP_SIZE,       SLEEP_VALUE },
    [WAKEUP]      = { WRITE_ONLY,   REG_WAKEUP_ADDR, 	  	REG_WAKEUP_SIZE,      WAKEUP_VALUE },
    [GPS]         = { WRITE_ONLY,   REG_GPS_ADDR,    	  	REG_GPS_SIZE,         GPS_VALUE },
    [LOG]  		  = { READ_ONLY,    REG_LOG_ADDR,  	  		REG_LOG_SIZE,    	  LOG_VALUE },
	[ACK]		  = { WRITE_ONLY,   REG_ACK_ADDR,		  	REG_ACK_SIZE,		  ACK_VALUE },
	[HW_STAT]     = { WRITE_ONLY,	REG_HW_STAT_ADDR,   	REG_HW_STAT_SIZE,     HW_STAT_VALUE},
	[HW_TXB] 	  = { READ_ONLY,	REG_HW_TXB_ADDR,    	REG_HW_STAT_SIZE,     HW_TXB_VALUE},
	[BOOT_DATA]	  = { WRITE_ONLY,	REG_DATA_BOOT_ADDR, 	REG_DATA_BOOT_SIZE,   BOOT_DATA_VALUE},
	[BOOT_STAT]   = { READ_ONLY,	REG_STAT_BOOT_ADDR, 	REG_STAT_BOOT_SIZE,   BOOT_STAT_VALUE},
	[EXP_STAT]    = { READ_ONLY,  	REG_EXP_STAT_ADDR,    	REG_EXP_STAT_SIZE,    EXP_STAT_VALUE },
	[EXP_SUM_IDX] = { WRITE_ONLY, 	REG_EXP_SUM_IDX_ADDR, 	REG_EXP_SUM_IDX_SIZE, EXP_SUM_IDX_VALUE },
	[EXP_SUM_DATA]= { READ_ONLY,  	REG_EXP_SUM_DATA_ADDR,	REG_EXP_SUM_DATA_SIZE,EXP_SUM_DATA_VALUE },
	[DB_CLEAR]    = { WRITE_ONLY, 	REG_DB_CLEAR_ADDR,    	REG_DB_CLEAR_SIZE,    DB_CLEAR_VALUE },
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

//NASCERR_STATUS nascerr_registers_changeExpExecutingStatus(uint32_t current_id, uint8_t current_block)
//{
//	memcpy(g_register_data[STAT].value, &current_id, sizeof(uint32_t));
//	size_t offset = sizeof(uint32_t);
//
//	memcpy(g_register_data[STAT].value + offset, &current_block, sizeof(uint8_t));
//
//	NASCERR_STATUS status;
//	memcpy(&status, g_register_data[STAT].value, sizeof(NASCERR_STATUS));
//	return status;
//}

//NASCERR_STATUS nascerr_registers_changeExpStatus(OP_EXPERIMENT op, REQ_EXPERIMENT req)
//{
//	uint8_t valid_op = (op > OP_EXP_START && op < OP_EXP_MAX);
//	uint8_t valid_req = (req > REQ_EXP_START && req < REQ_EXP_MAX);
//
//	if(valid_op && valid_req)
//	{
//		size_t offset = sizeof(uint32_t) + sizeof(uint8_t);
//		memcpy(g_register_data[STAT].value + offset, &op, sizeof(uint8_t));
//
//		offset += sizeof(uint8_t);
//		memcpy(g_register_data[STAT].value + offset, &req, sizeof(uint8_t));
//	}
//	else
//	{
//		//TODO: erro ocorreu
//
//	}
//
//	NASCERR_STATUS status;
//	memcpy(&status, g_register_data[STAT].value, sizeof(NASCERR_STATUS));
//	return status;
//}
//
//NASCERR_STATUS nascerr_registers_changeHwStatus(OP_HARDWARE op, REQ_HARDWARE req)
//{
//
//	uint8_t valid_op = (op > OP_HW_START && op < OP_HW_MAX);
//	uint8_t valid_req = (req > REQ_HW_START && req < REQ_HW_MAX);
//
//	if(valid_op && valid_req)
//	{
//		size_t offset =  sizeof(uint32_t) + 3 * sizeof(uint8_t);
//		memcpy(g_register_data[STAT].value + offset, &op, sizeof(uint8_t));
//
//		offset += sizeof(uint8_t);
//		memcpy(g_register_data[STAT].value + offset, &req, sizeof(uint8_t));
//	}
//	else
//	{
//		//TODO: erro ocorreu
//
//	}
//
//	NASCERR_STATUS status;
//	memcpy(&status, g_register_data[STAT].value, sizeof(NASCERR_STATUS));
//	return status;
//}
