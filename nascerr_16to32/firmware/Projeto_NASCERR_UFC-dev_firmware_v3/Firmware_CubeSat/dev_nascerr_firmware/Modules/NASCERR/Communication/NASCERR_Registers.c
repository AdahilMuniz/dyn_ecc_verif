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

//todo: CRIAR REGISTRO DE IMAGEM_CARREGADA

/* Allocation of memory for register values */
uint8_t STAT_VALUE[REG_STAT_SIZE] = {OP_EXP_IDLE, OP_HW_OK, REQ_EXP_NO_REQUEST, REQ_HW_NO_REQUEST, 0, 0, 0};
uint8_t IDX_VALUE[REG_IDX_SIZE] = {4, 0, 0, 0};
uint8_t TXB_VALUE[REG_TXB_SIZE] = {0};
uint8_t CMD_VALUE[REG_CMD_SIZE] = {0};
uint8_t SLEEP_VALUE[REG_SLEEP_SIZE] = {0, 0, 0};
uint8_t WAKEUP_VALUE[REG_WAKEUP_SIZE] = {0, 0, 0};
uint8_t GPS_VALUE[REG_GPS_SIZE] = {0};
uint8_t ACK_VALUE[REG_ACK_SIZE] = {0, 0, 0};
uint8_t HW_STAT_VALUE[REG_HW_STAT_SIZE] = {0};
uint8_t HW_TXB_VALUE[REG_HW_TXB_SIZE] = {0};
uint8_t BOOT_DATA_VALUE[REG_DATA_BOOT_SIZE] = {0};
uint8_t BOOT_STAT_VALUE[REG_STAT_BOOT_SIZE] = {0, 0, 0, 0, 0};
uint8_t EXP_STAT_VALUE[REG_EXP_STAT_SIZE]     = { 0 };
uint8_t EXP_SUM_IDX_VALUE[REG_EXP_SUM_IDX_SIZE] = { 0 };
uint8_t EXP_SUM_DATA_VALUE[REG_EXP_SUM_DATA_SIZE] = { 0 };
uint8_t DB_CLEAR_VALUE[REG_DB_CLEAR_SIZE] = { 0 };
uint8_t LOG_SUM_VALUE[REG_LOG_SUM_SIZE] = {0};
uint8_t LOG_SET_PAGE_VALUE[REG_LOG_SET_PAGE_SIZE] = {0};
uint8_t LOG_READ_PAGE_VALUE[REG_LOG_READ_PAGE_SIZE] = {0};
uint8_t ROBUST_MEM_HEALTH_VALUE[REG_ROBUST_MEM_HEALTH_SIZE] = {0};
uint8_t IMAGE_STAT_VALUE[REG_IMAGE_STAT_SIZE] = {0};

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
	[ACK]		  = { READ_ONLY,   	REG_ACK_ADDR,		  	REG_ACK_SIZE,		  ACK_VALUE },
	[HW_STAT]     = { WRITE_ONLY,	REG_HW_STAT_ADDR,   	REG_HW_STAT_SIZE,     HW_STAT_VALUE},
	[HW_TXB] 	  = { READ_ONLY,	REG_HW_TXB_ADDR,    	REG_HW_STAT_SIZE,     HW_TXB_VALUE},
	[BOOT_DATA]	  = { WRITE_ONLY,	REG_DATA_BOOT_ADDR, 	REG_DATA_BOOT_SIZE,   BOOT_DATA_VALUE},
	[BOOT_STAT]   = { READ_ONLY,	REG_STAT_BOOT_ADDR, 	REG_STAT_BOOT_SIZE,   BOOT_STAT_VALUE},
	[EXP_STAT]    = { READ_ONLY,  	REG_EXP_STAT_ADDR,    	REG_EXP_STAT_SIZE,    EXP_STAT_VALUE },
	[EXP_SUM_IDX] = { WRITE_ONLY, 	REG_EXP_SUM_IDX_ADDR, 	REG_EXP_SUM_IDX_SIZE, EXP_SUM_IDX_VALUE },
	[EXP_SUM_DATA]= { READ_ONLY,  	REG_EXP_SUM_DATA_ADDR,	REG_EXP_SUM_DATA_SIZE,EXP_SUM_DATA_VALUE },
	[DB_CLEAR]    = { WRITE_ONLY, 	REG_DB_CLEAR_ADDR,    	REG_DB_CLEAR_SIZE,    DB_CLEAR_VALUE },
	[LOG_SUM]       = { READ_ONLY, 		REG_LOG_SUM_ADDR,    	REG_LOG_SUM_SIZE,     	LOG_SUM_VALUE },
	[LOG_SET_PAGE]  = { WRITE_ONLY, 	REG_LOG_SET_PAGE_ADDR,  REG_LOG_SET_PAGE_SIZE,	LOG_SET_PAGE_VALUE },
	[LOG_READ_PAGE] = { READ_ONLY, 		REG_LOG_READ_PAGE_ADDR, REG_LOG_READ_PAGE_SIZE, LOG_READ_PAGE_VALUE },
	[ROBUST_MEM_HEALTH]   = { READ_ONLY, 		REG_ROBUST_MEM_HEALTH_ADDR, REG_ROBUST_MEM_HEALTH_SIZE, ROBUST_MEM_HEALTH_VALUE },
	[IMAGE_STAT]  = { READ_ONLY, 	REG_IMAGE_STAT_ADDR, 	REG_IMAGE_STAT_SIZE,  IMAGE_STAT_VALUE},
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
 *  Function Name: nascerr_registers_getStatusSystem
 *  @brief     : Get the status of the system
 *  @param     : none
 *  @retval    : Status of the system (NASCERR_STATUS)
 * ******************************************************************************
 */
NASCERR_STATUS nascerr_registers_getStatusSystem(void)
{
	NASCERR_STATUS status;
	memcpy(&status, g_register_data[STAT].value, sizeof(NASCERR_STATUS));
	return status;
}

/*
 * ******************************************************************************
 *  Function Name : nascerr_registers_changeExpStatus
 *  @brief        : Changes the experiment operation and request status fields
 *                  in the NASCERR registers.
 *  @param        : OP_EXPERIMENT op  - The new experiment operation status to be set.
 *                  REQ_EXPERIMENT req - The new experiment request status to be set.
 *  @retval       : NASCERR_STATUS - The updated status of the registers.
 * ******************************************************************************
 */
NASCERR_STATUS nascerr_registers_changeExpStatus(OP_EXPERIMENT op, REQ_EXPERIMENT req)
{
	uint8_t valid_op = (op > OP_EXP_START && op < OP_EXP_MAX);
	uint8_t valid_req = (req > REQ_EXP_START && req < REQ_EXP_MAX);

	if(valid_op && valid_req)
	{
		g_register_data[STAT].value[0] = op;
		g_register_data[STAT].value[2] = req;
	}

	NASCERR_STATUS status;
	memcpy(&status, g_register_data[STAT].value, sizeof(NASCERR_STATUS));
	return status;
}

/*
 * ******************************************************************************
 *  Function Name : nascerr_registers_changeHwStatus
 *  @brief        : Changes the hardware operation and request status fields
 *                  in the NASCERR registers.
 *  @param        : OP_HARDWARE op  - The new hardware operation status to be set.
 *                  REQ_HARDWARE req - The new hardware request status to be set.
 *  @retval       : NASCERR_STATUS - The updated status of the registers.
 * ******************************************************************************
 */
NASCERR_STATUS nascerr_registers_changeHwStatus(OP_HARDWARE op, REQ_HARDWARE req)
{

	uint8_t valid_op = (op > OP_HW_START && op < OP_HW_MAX);
	uint8_t valid_req = (req > REQ_HW_START && req < REQ_HW_MAX);

	if(valid_op && valid_req)
	{
		g_register_data[STAT].value[1] = op;
		g_register_data[STAT].value[3] = req;
	}

	NASCERR_STATUS status;
	memcpy(&status, g_register_data[STAT].value, sizeof(NASCERR_STATUS));
	return status;
}

/*
 * ******************************************************************************
 *  Function Name : nascerr_registers_changeHwFailStatus
 *  @brief        : Changes the global fail status flag in the NASCERR registers
 *  to show which hardware failure occured.
 *  @param        : uint8_t fail - Fail status flag to be set.
 *  @retval       : NASCERR_STATUS - The updated status of the registers.
 * ******************************************************************************
 */
NASCERR_STATUS nascerr_registers_changeHwFailStatus(uint8_t fail)
{
    g_register_data[STAT].value[4] =
        (g_register_data[STAT].value[4] & 0b11000000) | (fail);

    NASCERR_STATUS status;
    memcpy(&status, g_register_data[STAT].value, sizeof(NASCERR_STATUS));
    return status;
}

/*
 * ******************************************************************************
 *  Function Name : nascerr_register_changeQueueStatus
 *  @brief        : Changes the global fail status flag in the NASCERR registers
 *  to show how filled the queue is.
 *  @param        : uint8_t queue_count - Indicates how many elements on the
 *  queue.
 *  @retval       : NASCERR_STATUS - The updated status of the registers.
 * ******************************************************************************
 */
NASCERR_STATUS nascerr_register_changeQueueStatus(uint8_t queue_count)
{
	uint8_t queue_bits = 0;
	if (queue_count < 32)
	{
		queue_bits = 0b00000000;
	}
	else if(queue_count < 48)
	{
		queue_bits = 0b01000000;
	}
	else if (queue_count < 64)
	{
		queue_bits = 0b10000000;
	}
	else
	{
		queue_bits = 0b11000000;
	}
	g_register_data[STAT].value[4] =
	        (g_register_data[STAT].value[4] & 0b00111111) | (queue_bits);

	NASCERR_STATUS status;
	memcpy(&status, g_register_data[STAT].value, sizeof(NASCERR_STATUS));
	return status;
}

/*
 * ******************************************************************************
 *  Function Name : nascerr_register_changeAckPhase
 *  @brief        : Changes the acknowledgement phase of the command.
 *  @param        : ACK_PHASE phase  - The new phase status to be set.
 *  @retval       : None
 * ******************************************************************************
 */
void nascerr_register_changeAckPhase(ACK_PHASE phase)
{
	g_register_data[ACK].value[0] = phase;
}
