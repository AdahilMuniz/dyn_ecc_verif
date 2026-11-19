/**
 * ******************************************************************************
 *  @file    : NASCERR_Protocol.c
 *  @brief   : NASCERR communication protocol.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include "NASCERR_Protocol.h"
#include "NASCERR_System.h"
//#include "NASCERR_Log.h"
#include "NASCERR_Bootloader.h"
//#include "NASCERR_SchedulerDB.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

ProtocolTypeDef g_comm_config;
NASCERR_COMMANDS g_new_command;
uint32_t g_protocol_Timer[3] = {0,0,0};
uint8_t g_receive_command;
uint8_t g_receive_hw_stat;
uint8_t g_master_receive_response;
uint8_t g_comm_byte;

/* LOG para Debug */
uint8_t log_state[256];
uint8_t log_count = 0;

NASCERR_MODE mode_system;

uint16_t g_header_size = 0;
uint16_t g_dresult_size = 0;
uint16_t g_response_size = 0;
uint16_t g_response_crc = 0;

uint16_t ack;

extern PARAM_STANDARD transmitted;
extern uint8_t boot_in_process;

NASCERR_BOOT_STATUS boot_status;
uint8_t g_receive_boot_data;

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

uint8_t protocol_checkAccess(ProtocolTypeDef *comm_config);

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_init
 *  @brief     : Initialize the communication protocol
 *  @param     : comm_config - Pointer to the communication configuration structure
 *  @retval    : none
 * ******************************************************************************
 */
void nascerr_protocol_init(ProtocolTypeDef *comm_config)
{
    comm_config->status_bus  = LISTENING_MODE;
    comm_config->command_reg = NONE;
    comm_config->status_comm = COMMUNICATION_SUCCESS;
    comm_config->state  = PROTOCOL_IDLE;
    comm_config->data_length = 0;
    comm_config->current_idx = 0;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_commInit
 *  @brief     : Initialize communication on the bus
 *  @param     : comm_config - Pointer to the communication configuration structure
 *  @retval    : none
 * ******************************************************************************
 */
void nascerr_protocol_commInit(ProtocolTypeDef *comm_config)
{
    comm_config->status_bus = LISTENING_MODE;
    comm_config->status_comm = COMMUNICATION_IDLE;
    comm_config->state = PROTOCOL_IDLE;
    comm_config->command_reg = NONE;
    nascerr_systimer_reset(g_protocol_Timer);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_poll
 *  @brief     : Poll the communication protocol
 *  @param     : comm_config - Pointer to the communication configuration structure
 *               status_bus - Current status of the bus
 *  @retval    : Status of the communication (StatusCommTypeDef)
 * ******************************************************************************
 */
StatusCommTypeDef nascerr_protocol_poll(ProtocolTypeDef *comm_config, BusTypeDef status_bus)
{

    uint8_t *bytePointer;

    comm_config->status_bus = status_bus;

    if (nascerr_systimer(COMM_TIMEOUT, g_protocol_Timer) && (comm_config->state > PROTOCOL_REGISTER))
    {
    	nascerr_systimer(COMM_TIMEOUT, g_protocol_Timer);
        comm_config->state = PROTOCOL_FAIL;
        comm_config->data_length = 0;
        comm_config->current_idx = 0;
    }

    switch (comm_config->state) {
        case PROTOCOL_IDLE:
        	/* ECHO ADDR -> FOR I2C DETECT */
        	if (comm_config->status_bus == TRANSMITTING_DATA)
        	{
        		bytePointer = (uint8_t *)&(comm_config->addr);
        		nascerr_protocol_send(bytePointer, 1);
        		break;
        	}
            comm_config->status_comm = COMMUNICATION_INIT;
            comm_config->state = PROTOCOL_REGISTER;
            break;

        case PROTOCOL_REGISTER:
            comm_config->command_reg = g_comm_byte;
            comm_config->status_comm = COMMUNICATION_INPROCESS;
            switch (comm_config->command_reg) {

                case BOOT_STAT:
                {
                	uint8_t *boot_stat;
                	boot_stat = nascerr_bootloader_getBootStatus();

                    uint16_t crc_boot_stat = calculate_crc16(boot_stat, g_register_data[comm_config->command_reg].size-2);

                    comm_config->data_txbuff = boot_stat;
                    comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-1] = (uint8_t)(crc_boot_stat & 0xFF);
					comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-2] = (uint8_t)((crc_boot_stat >> 8) & 0xFF);

					comm_config->data_length = g_register_data[comm_config->command_reg].size;
                    comm_config->state = PROTOCOL_SEND_DATA;
					break;
                }

                default:
                    comm_config->status_comm = COMMUNICATION_FAILURE;
                    break;
            }

            break;

        case PROTOCOL_SEND_DATA:
            if ((!protocol_checkAccess(comm_config)) || (comm_config->status_bus == RECEIVING_DATA)) {
                comm_config->status_comm = COMMUNICATION_FAILURE;
                break;
            }

            if ((comm_config->status_bus == TRANSMITTING_DATA) && (comm_config->data_length != 0)) {
            	nascerr_protocol_send(comm_config->data_txbuff, comm_config->data_length);
            }

            comm_config->data_length = 0;
            comm_config->current_idx = 0;

            nascerr_protocol_commInit(comm_config);

            break;

        default:
            comm_config->status_comm = COMMUNICATION_FAILURE;
            break;
    }

    if (comm_config->status_bus == RECEIVING_DATA)
    {
        g_comm_byte = 0;
        nascerr_protocol_receive(&g_comm_byte, 1);
    }

    return comm_config->status_comm;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_send
 *  @brief     : Send data over the communication channel
 *  @param     : pData - Pointer to the data to be sent
 *               Size - Size of the data to be sent
 *  @retval    : Status of the communication (StatusCommTypeDef)
 * ******************************************************************************
 */
StatusCommTypeDef nascerr_protocol_send(uint8_t *pData, uint16_t Size)
{
	if (func_send_channel(pData, Size) != STATUS_OK) {
		return COMMUNICATION_FAILURE;
	}

    return COMMUNICATION_SUCCESS;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_receive
 *  @brief     : Receive data from the communication channel
 *  @param     : pData - Pointer to the buffer to store received data
 *               Size - Size of the buffer
 *  @retval    : Status of the communication (StatusCommTypeDef)
 * ******************************************************************************
 */
StatusCommTypeDef nascerr_protocol_receive(uint8_t *pData, uint16_t Size)
{
    if (func_receive_channel(pData, Size) != STATUS_OK) {
        return COMMUNICATION_FAILURE;
    }

    return COMMUNICATION_SUCCESS;
}

/*
 * ******************************************************************************
 *  Function Name: protocol_checkAccess (LOCAL FUNCTION)
 *  @brief     : Check access permission for the register
 *  @param     : comm_config - Pointer to the communication configuration structure
 *  @retval    : Access permission (true/false)
 * ******************************************************************************
 */
uint8_t protocol_checkAccess(ProtocolTypeDef *comm_config)
{
    uint8_t access = false;

    if ((g_register_data[comm_config->command_reg].access != FULL_ACCESS) &&
        (g_register_data[comm_config->command_reg].access != (RegModeTypeDef) comm_config->status_bus))
    {
        return access;
    }

    access = true;

    return access;
}

