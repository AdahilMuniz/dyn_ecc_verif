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
#include "NASCERR_Protocol.h"
#include "NASCERR_Registers.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

ProtocolTypeDef g_comm_config;
NASCERR_COMMANDS g_new_command;
uint32_t g_protocol_Timer[3] = {0,0,0};
uint8_t g_receive_command;
uint8_t g_comm_byte;

/* LOG para Debug */
uint8_t log_state[256];
uint8_t log_count = 0;

NASCERR_STATUS status_system;
NASCERR_MODE mode_system;

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
                case STAT:
                    /* GET system status */
                    status_system = nascerr_protocol_getStatusSystem();
                    bytePointer = (uint8_t *)&status_system;

                    /* SEND status over the comm channel */
                    comm_config->data_txbuff = bytePointer;
					comm_config->data_length = g_register_data[comm_config->command_reg].size;
                    comm_config->state = PROTOCOL_SEND_DATA;
                    break;
                case IDX:
                    comm_config->data_length = g_register_data[comm_config->command_reg].size;
                    comm_config->data_txbuff = g_register_data[comm_config->command_reg].value;
                    comm_config->state = PROTOCOL_SEND_DATA;
                    break;
                case TXB:
                    comm_config->data_length = (*(g_register_data[IDX].value) << 8) | *(g_register_data[IDX].value + 1);
                    comm_config->data_txbuff = g_register_data[TXB].value;
                    comm_config->state = PROTOCOL_SEND_DATA;
                    break;
                case CMD:
                    comm_config->data_rxbuff = g_register_data[CMD].value;
                    comm_config->state = PROTOCOL_LENGTH_DATA_HI;
                    break;
                case MODO:
                	/* GET system mode */
					mode_system = nascerr_protocol_getModeSystem();
					bytePointer = (uint8_t *)&mode_system;

					/* SEND mode over the comm channel */
					comm_config->data_txbuff = bytePointer;
					comm_config->data_length = g_register_data[comm_config->command_reg].size;
					comm_config->state = PROTOCOL_SEND_DATA;
                    break;
                default:
                    comm_config->status_comm = COMMUNICATION_FAILURE;
                    break;
            }

            break;
        case PROTOCOL_LENGTH_DATA_HI:
            if (!protocol_checkAccess(comm_config))
            {
                comm_config->status_comm = COMMUNICATION_FAILURE;
                break;
            }

            comm_config->data_length = g_comm_byte << 8;
            comm_config->status_comm = COMMUNICATION_INPROCESS;
            comm_config->state = PROTOCOL_LENGTH_DATA_LO;

            break;
        case PROTOCOL_LENGTH_DATA_LO:
            if (!protocol_checkAccess(comm_config))
            {
                comm_config->status_comm = COMMUNICATION_FAILURE;
                break;
            }

            comm_config->data_length |= g_comm_byte;
            comm_config->current_idx = 0;
            comm_config->status_comm = COMMUNICATION_INPROCESS;
            comm_config->state = PROTOCOL_RECEIVE_DATA;

            break;
        case PROTOCOL_RECEIVE_DATA:
            if ((!protocol_checkAccess(comm_config)) || (comm_config->status_bus == TRANSMITTING_DATA))
            {
                comm_config->status_comm = COMMUNICATION_FAILURE;
                break;
            }

            *(comm_config->data_rxbuff + comm_config->current_idx) = g_comm_byte;

            if (++comm_config->current_idx >= comm_config->data_length)
            {
                comm_config->status_comm = PROCESSING_DATA;
                comm_config->state = PROTOCOL_PROCESS_DATA;
            }
            else {
                comm_config->status_comm = COMMUNICATION_INPROCESS;
            }

            /* Reset and restart the timer */
            nascerr_systimer_reset(g_protocol_Timer);
            nascerr_systimer(COMM_TIMEOUT, g_protocol_Timer);

            break;
        case PROTOCOL_SEND_DATA:
            if ((!protocol_checkAccess(comm_config)) || (comm_config->status_bus == RECEIVING_DATA))
            {
                comm_config->status_comm = COMMUNICATION_FAILURE;
                break;
            }

            if ((comm_config->status_bus == TRANSMITTING_DATA) && (comm_config->data_length != 0))
            {
                nascerr_protocol_send(comm_config->data_txbuff, comm_config->data_length);
            }

            comm_config->data_length = 0;
            comm_config->current_idx = 0;

            nascerr_protocol_commInit(comm_config);

            break;
        case PROTOCOL_PROCESS_DATA:
            /* Process received data */
            nascerr_protocol_processData(comm_config->command_reg);

            /* Log the last 256 commands received by the protocol */
            log_state[log_count] = comm_config->command_reg;
            log_count++;
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
    if (func_send_channel(pData, Size) != STATUS_OK)
    {
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
    if (func_receive_channel(pData, Size) != STATUS_OK)
    {
        return COMMUNICATION_FAILURE;
    }

    return COMMUNICATION_SUCCESS;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_getStatusSystem
 *  @brief     : Get the status of the system
 *  @param     : none
 *  @retval    : Status of the system (NASCERR_STATUS)
 * ******************************************************************************
 */
NASCERR_STATUS nascerr_protocol_getStatusSystem(void)
{
    NASCERR_STATUS status_system;

    status_system.OP_LOGS = (OP_LOGS) *(g_register_data[STAT].value);
    status_system.REQ_LOGS = (REQ_LOGS) *(g_register_data[STAT].value + 1);

    return status_system;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_getStatusSystem
 *  @brief     : Get the status of the system
 *  @param     : none
 *  @retval    : Status of the system (NASCERR_STATUS)
 * ******************************************************************************
 */
NASCERR_MODE nascerr_protocol_getModeSystem(void)
{
	NASCERR_MODE mode_system;

	mode_system = (NASCERR_MODE) *(g_register_data[MODO].value);

	return mode_system;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_newRequest
 *  @brief     : Create a new request
 *  @param     : request - Request type (REQ_LOGS)
 *  @retval    : Status of the request (true/false)
 * ******************************************************************************
 */
uint8_t nascerr_protocol_newRequest(REQ_LOGS request)
{
    if ((request < REQ_NOREQUEST) || (request >= REQ_MAX))
    {
        return false;
    }

    *(g_register_data[STAT].value + 1) = (uint8_t)request;

    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_requestTxResult
 *  @brief     : Request transmission result
 *  @param     : pData - Pointer to the data buffer
 *               Size - Size of the data buffer
 *  @retval    : Status of the request (true/false)
 * ******************************************************************************
 */
uint8_t nascerr_protocol_requestTxResult(uint8_t *pData, uint16_t Size)
{
    uint8_t *bytePointer;

    if ((pData == NULL) || (Size == 0U))
    {
        return false;
    }

    bytePointer = (uint8_t *)&Size;
    g_register_data[IDX].value[0] = bytePointer[0];
    g_register_data[IDX].value[1]  = bytePointer[1];

    for (uint16_t i = 0; i < Size; i++)
    {
        g_register_data[TXB].value[i] = pData[i];
    }

    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_processData
 *  @brief     : Process received data
 *  @param     : new_register - Register type (RegistersTypeDef)
 *  @retval    : Status of the communication (StatusCommTypeDef)
 * ******************************************************************************
 */
StatusCommTypeDef nascerr_protocol_processData(RegistersTypeDef new_register)
{
    StatusCommTypeDef status;

    switch (new_register) {
        case CMD:
            g_new_command = DATA_RECEIVED;
            status = COMMUNICATION_SUCCESS;
            break;
        case MODO:
            g_new_command = SEND_MODE;
            status = COMMUNICATION_SUCCESS;
            break;
        default:
            status = DATA_CORRUPTION;
            g_new_command = NO_COMMANDS;
            g_receive_command = false;
            return status;
            break;
    }

    g_receive_command = true;

    return status;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_receivedNewCommand
 *  @brief     : Check if a new command has been received
 *  @param     : none
 *  @retval    : Status of the new command (true/false)
 * ******************************************************************************
 */
uint8_t nascerr_protocol_receivedNewCommand(void)
{
    return g_receive_command;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_processCommand
 *  @brief     : Process the received command
 *  @param     : none
 *  @retval    : Command type (NASCERR_COMMANDS)
 * ******************************************************************************
 */
NASCERR_COMMANDS nascerr_protocol_processCommand(void)
{
    NASCERR_COMMANDS command = g_new_command;

    g_receive_command = false;
    g_new_command = NO_COMMANDS;

    return command;
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
