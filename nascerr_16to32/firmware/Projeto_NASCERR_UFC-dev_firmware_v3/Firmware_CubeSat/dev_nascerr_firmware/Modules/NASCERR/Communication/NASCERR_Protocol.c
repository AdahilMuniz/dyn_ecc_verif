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
#include "NASCERR_Log.h"
#include "NASCERR_Bootloader.h"
#include "NASCERR_SchedulerDB.h"

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

void protocol_process_ack();

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
    	DEBUG_PRINT(LOG_ERROR, "[I2C] Timeout na comunicacao I2C (estado=%d)\r\n", comm_config->state);
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

                    /* SEND status over the comm channel */
                    uint16_t crc_stat = calculate_crc16(g_register_data[comm_config->command_reg].value, g_register_data[comm_config->command_reg].size-2);
                    comm_config->data_txbuff = g_register_data[comm_config->command_reg].value;
					comm_config->data_length = g_register_data[comm_config->command_reg].size;
					comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-2] = (uint8_t)((crc_stat >> 8) & 0xFF);
					comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-1] = (uint8_t)(crc_stat & 0xFF);
                    comm_config->state = PROTOCOL_SEND_DATA;
                    break;
                case IDX:
                	uint16_t crc_idx = calculate_crc16(g_register_data[comm_config->command_reg].value, g_register_data[comm_config->command_reg].size-2);

                    comm_config->data_length = g_register_data[comm_config->command_reg].size;
                    comm_config->data_txbuff = g_register_data[comm_config->command_reg].value;
                    comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-2] = (uint8_t)((crc_idx >> 8) & 0xFF);
                    comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-1] = (uint8_t)(crc_idx & 0xFF);

                    comm_config->state = PROTOCOL_SEND_DATA;
                    break;
                case TXB:
					comm_config->data_length = (g_register_data[IDX].value[1] << 8) | g_register_data[IDX].value[0];
					if (comm_config->data_length == 4) {
						nascerr_protocol_prepareEmptyResponse(comm_config);
					} else {
						comm_config->data_txbuff = g_register_data[TXB].value;
					}

					comm_config->state = PROTOCOL_SEND_DATA;
                    break;
                case CMD:
                    comm_config->data_rxbuff = g_register_data[CMD].value;
                    comm_config->state = PROTOCOL_LENGTH_DATA_HI;
                    break;
                case ACK:
					uint16_t crc_ack = calculate_crc16(g_register_data[comm_config->command_reg].value, g_register_data[comm_config->command_reg].size-2);
					comm_config->data_txbuff = g_register_data[comm_config->command_reg].value;
					comm_config->data_length = g_register_data[comm_config->command_reg].size;
					comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-2] = (uint8_t)((crc_ack >> 8) & 0xFF);
					comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-1] = (uint8_t)(crc_ack & 0xFF);
					comm_config->state = PROTOCOL_SEND_DATA;
					break;
                case SLEEP:
                	comm_config->data_rxbuff = g_register_data[SLEEP].value;
                	comm_config->state = PROTOCOL_RECEIVE_DATA;

                	break;
                case WAKEUP:
                	comm_config->data_rxbuff = g_register_data[WAKEUP].value;
                	comm_config->state = PROTOCOL_RECEIVE_DATA;
                	break;
                case HW_STAT:
					/* GET Hardware status request */
					comm_config->data_rxbuff = g_register_data[HW_STAT].value;
					comm_config->state = PROTOCOL_RECEIVE_DATA;

                	break;
                case HW_TXB:
					comm_config->data_length = g_register_data[HW_TXB].size;
					comm_config->data_txbuff = g_register_data[HW_TXB].value;
					comm_config->state = PROTOCOL_SEND_DATA;

					break;
                case LOG_SUM:
                {
                	uint16_t crc_send = calculate_crc16(g_register_data[comm_config->command_reg].value,
														g_register_data[comm_config->command_reg].size-2);

					comm_config->data_txbuff = g_register_data[comm_config->command_reg].value;
					comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-1] = (uint8_t)(crc_send & 0xFF);
					comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-2] = (uint8_t)((crc_send >> 8) & 0xFF);

					comm_config->data_length = g_register_data[comm_config->command_reg].size;
					comm_config->state = PROTOCOL_SEND_DATA;
					break;
                }

                case LOG_SET_PAGE:
					comm_config->data_rxbuff = g_register_data[LOG_SET_PAGE].value;
					comm_config->state = PROTOCOL_RECEIVE_DATA;
					break;

                case LOG_READ_PAGE:
                {
					uint16_t crc_send = calculate_crc16(g_register_data[comm_config->command_reg].value,
														g_register_data[comm_config->command_reg].size-2);

                	comm_config->data_txbuff = g_register_data[comm_config->command_reg].value;
					comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-1] = (uint8_t)(crc_send & 0xFF);
					comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-2] = (uint8_t)((crc_send >> 8) & 0xFF);

					comm_config->data_length = g_register_data[comm_config->command_reg].size;
					comm_config->state = PROTOCOL_SEND_DATA;

					break;
                }
                case EXP_STAT:

                case EXP_SUM_DATA:
                {
					uint16_t crc_send = calculate_crc16(g_register_data[comm_config->command_reg].value,
														g_register_data[comm_config->command_reg].size-2);

                	comm_config->data_txbuff = g_register_data[comm_config->command_reg].value;
					comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-1] = (uint8_t)(crc_send & 0xFF);
					comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-2] = (uint8_t)((crc_send >> 8) & 0xFF);

					comm_config->data_length = g_register_data[comm_config->command_reg].size;
					comm_config->state = PROTOCOL_SEND_DATA;
					break;
                }

                case EXP_SUM_IDX:
					comm_config->data_rxbuff = g_register_data[EXP_SUM_IDX].value;
					comm_config->state = PROTOCOL_RECEIVE_DATA;
					break;

                case GPS:
					comm_config->data_rxbuff = g_register_data[GPS].value;
					comm_config->state = PROTOCOL_RECEIVE_DATA;
					break;

                case DB_CLEAR:
					comm_config->data_rxbuff = g_register_data[DB_CLEAR].value;
					comm_config->state = PROTOCOL_RECEIVE_DATA;
					break;

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

                case BOOT_DATA:
                	comm_config->data_rxbuff = g_register_data[BOOT_DATA].value;
                	comm_config->state = PROTOCOL_LENGTH_DATA_HI;
                	break;

                case ROBUST_MEM_HEALTH:
					uint16_t crc_rob_mem = calculate_crc16(g_register_data[comm_config->command_reg].value, g_register_data[comm_config->command_reg].size-2);

                	comm_config->data_txbuff = g_register_data[ROBUST_MEM_HEALTH].value;
                	comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-2] = (uint8_t)((crc_rob_mem >> 8) & 0xFF);
                	comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-1] = (uint8_t)(crc_rob_mem & 0xFF);

					comm_config->data_length = g_register_data[comm_config->command_reg].size;
                	comm_config->state = PROTOCOL_SEND_DATA;
                	break;

                case IMAGE_STAT:
                	uint16_t crc_img_stat = calculate_crc16(g_register_data[comm_config->command_reg].value, g_register_data[comm_config->command_reg].size-2);
					comm_config->data_txbuff = g_register_data[comm_config->command_reg].value;
					comm_config->data_length = g_register_data[comm_config->command_reg].size;
					comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-2] = (uint8_t)((crc_img_stat >> 8) & 0xFF);
					comm_config->data_txbuff[g_register_data[comm_config->command_reg].size-1] = (uint8_t)(crc_img_stat & 0xFF);
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
            *(comm_config->data_rxbuff + comm_config->current_idx++) = g_comm_byte;
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
            *(comm_config->data_rxbuff + comm_config->current_idx++) = g_comm_byte;
            comm_config->status_comm = COMMUNICATION_INPROCESS;
            comm_config->state = PROTOCOL_RECEIVE_DATA;

            break;
        case PROTOCOL_RECEIVE_DATA:

        	if(comm_config->command_reg != CMD && comm_config->command_reg != BOOT_DATA)
        	{
        		comm_config->data_length = g_register_data[comm_config->command_reg].size;
				comm_config->status_comm = COMMUNICATION_INPROCESS;
			}

            if ((!protocol_checkAccess(comm_config)) || (comm_config->status_bus == TRANSMITTING_DATA)) {
                comm_config->status_comm = COMMUNICATION_FAILURE;
                break;
            }

            *(comm_config->data_rxbuff + comm_config->current_idx) = g_comm_byte;
            if (++comm_config->current_idx >= comm_config->data_length) {
            	/* Realizando a verificacao do dado de CRC */
            	uint16_t received_crc = (comm_config->data_rxbuff[comm_config->data_length - 1] |
            	                         (comm_config->data_rxbuff[comm_config->data_length - 2] << 8));
                uint16_t calculated_crc = calculate_crc16(comm_config->data_rxbuff, comm_config->data_length - 2);

                /* Verificacao se o tamanho recebido e igual ao que informa na mensagem */
                if ((comm_config->current_idx != comm_config->data_length)||(received_crc!=calculated_crc)) {
                    comm_config->status_comm = COMMUNICATION_FAILURE;
                    comm_config->state = PROTOCOL_IDLE;
                    break;
                }

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
            if ((!protocol_checkAccess(comm_config)) || (comm_config->status_bus == RECEIVING_DATA)) {
                comm_config->status_comm = COMMUNICATION_FAILURE;
                break;
            }

            if ((comm_config->status_bus == TRANSMITTING_DATA) && (comm_config->data_length != 0)) {
            	nascerr_log_register_tx_result(nascerr_protocol_send(comm_config->data_txbuff, comm_config->data_length), comm_config->command_reg);
            }

            if(comm_config->command_reg == LOG_READ_PAGE) {
                g_new_command = LOG_MARK_READ;
                g_receive_command = true; // Sinaliza para o scheduler processar este novo comando.
            }

            comm_config->data_length = 0;
            comm_config->current_idx = 0;

            if(comm_config->command_reg==TXB) {
            	nascerr_registers_changeExpStatus(nascerr_registers_getStatusSystem().OP.EXP, REQ_EXP_NO_REQUEST);
            	nascerr_protocol_processData(comm_config->command_reg);
            }

            if(comm_config->command_reg==HW_TXB) {
            	nascerr_registers_changeHwStatus(OP_HW_OK, REQ_HW_NO_REQUEST);
            }

            if(comm_config->command_reg==ROBUST_MEM_HEALTH) {
				nascerr_protocol_processData(comm_config->command_reg);
			}

            nascerr_protocol_commInit(comm_config);
            break;
        case PROTOCOL_PROCESS_DATA:
            /* Process received data */
            nascerr_protocol_processData(comm_config->command_reg);

            comm_config->current_idx = 0;
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
            g_new_command = CMD_RECEIVED;
            status = COMMUNICATION_SUCCESS;
            g_master_receive_response = false;
            g_receive_command = true;
            break;

        case SLEEP:
        	/* apenas e repassado para a camada acima quando tudo e validado */
			g_new_command = COMMAND_TO_SLEEP;
			status = COMMUNICATION_SUCCESS;
			g_master_receive_response = false;
			g_receive_command = true;
			break;

        case WAKEUP:
        	/* apenas e repassado para a camada acima quando tudo e validado */
			g_new_command = COMMAND_TO_WAKEUP;
			status = COMMUNICATION_SUCCESS;
			g_master_receive_response = false;
			g_receive_command = true;
			break;

        case HW_STAT:
			status = COMMUNICATION_SUCCESS;
			g_master_receive_response = false;
			g_receive_command = false;
			nascerr_hw_processHWStatusReq();
        	break;

        case BOOT_DATA:
            g_new_command = BOOT_COMMAND;
            status = COMMUNICATION_SUCCESS;
            g_master_receive_response = false;

            if(boot_in_process) {
            	g_receive_boot_data = true;
				g_receive_command = false;
			}
			else {
				g_receive_boot_data = false;
				g_receive_command = true;
			}

            break;

        case EXP_SUM_IDX:
			g_new_command = SET_PAGE_SUMARY;
			status = COMMUNICATION_SUCCESS;
			g_master_receive_response = false;
			g_receive_command = true;
			break;

        case LOG_SET_PAGE:
			g_new_command = LOG_SUMMARY;
			status = COMMUNICATION_SUCCESS;
			g_master_receive_response = false;
			g_receive_command = true;
			break;

        case DB_CLEAR:
			g_new_command = CLEAR_DB_COMMAND;
			status = COMMUNICATION_SUCCESS;
			g_master_receive_response = false;
			g_receive_command = true;
			break;

        case GPS:
			g_new_command = GPS_DATA_RECEIVED;
			status = COMMUNICATION_SUCCESS;
			g_master_receive_response = false;
			g_receive_command = true;
			break;

        case TXB:
			g_new_command = EXP_TRANSFER_COMPLETE;
			status = COMMUNICATION_SUCCESS;
			g_master_receive_response = false;
			g_receive_command = true;
			break;

        case ROBUST_MEM_HEALTH:
        	g_new_command = ROBUST_MEM_HEALTH_READ;
			status = COMMUNICATION_SUCCESS;
			g_master_receive_response = false;
			g_receive_command = true;
			break;

        default:
            status = DATA_CORRUPTION;
            g_new_command = NO_COMMANDS;
            g_master_receive_response = false;
            g_receive_command = false;
            return status;
            break;
    }

    if(nascerr_registers_getStatusSystem().OP.HW == OP_HW_SLEEP && g_new_command != COMMAND_TO_WAKEUP){  g_receive_command = false; }

    return status;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_getHWStatReq
 *  @brief     : Get the hardware status requisition
 *  @param     : none
 *  @retval    : Status of the system (NASCERR_HARDWARE_STATUS)
 * ******************************************************************************
 */
NASCERR_HARDWARE_STATUS nascerr_protocol_getHWStatReq(void)
{
	NASCERR_HARDWARE_STATUS hardware_stat_req;

	hardware_stat_req = (NASCERR_HARDWARE_STATUS) *(g_register_data[HW_STAT].value);

	return hardware_stat_req;
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
 *  Function Name: nascerr_protocol_receivedNewBootData
 *  @brief     : Check if a new command has been received
 *  @param     : none
 *  @retval    : Status of the new boot data (true/false)
 * ******************************************************************************
 */
uint8_t nascerr_protocol_receivedNewBootData(void)
{
    return g_receive_boot_data;
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_receivedHWStatRequest
 *  @brief     : Check if a new hardware status request has been received
 *  @param     : none
 *  @retval    : Status of the new hardware request (true/false)
 * ******************************************************************************
 */

uint8_t nascerr_protocol_receivedHWStatRequest(void)
{
	return g_receive_hw_stat;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_processBootData
 *  @brief     : Process the received command boot data
 *  @param     : none
 *  @retval    : Command type (NASCERR_COMMANDS)
 * ******************************************************************************
 */
NASCERR_COMMANDS nascerr_protocol_processBootData(void)
{
    NASCERR_COMMANDS command = g_new_command;

    g_receive_boot_data = false;
    g_new_command = NO_COMMANDS;

    return command;
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
 *  Function Name: nascerr_HWStat_casting (FUNCTION USED FOR UPPER LAYERS)
 *  @brief     : casts the measurements into a well defined structure
 *  @param     : buffer_res - Pointer to structure which holds the values of
 *  current, power and temperature
 *  data - reading from the ADC (it can a temperature, current or power info)
 *  @retval    : none
 * ******************************************************************************
 */

void nascerr_HWStat_casting(NASCERR_HW_RESPONSE *buffer_res, float *data)
{
    // Copia currents[0..4] de data[0..4]
    memcpy(buffer_res->currents, &data[0], 5 * sizeof(float));

    // temp = data[5]
    buffer_res->temp = data[5];

    // Copia powers[0..4] de data[6..10]
    memcpy(buffer_res->powers, &data[6], 5 * sizeof(float));

    // total_power = data[11]
    buffer_res->total_power = data[11];
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_protocol_prepareEmptyResponse (LOCAL FUNCTION)
 *  @brief     : Sends an standard response when the is requisition of read
 *  without an command sent is made
 *  @param     : comm_config - Pointer to the communication configuration structure
 *  @retval    : none
 * ******************************************************************************
 */

void nascerr_protocol_prepareEmptyResponse(ProtocolTypeDef *comm_config) {
    static uint8_t no_data_response[4] = {0x00, 0x00, 0x00, 0x00};

    uint16_t crc = calculate_crc16(no_data_response, 2);
    no_data_response[2] = (uint8_t)((crc >> 8) & 0xFF);
    no_data_response[3] = (uint8_t)(crc & 0xFF);

    comm_config->data_txbuff = no_data_response;
    comm_config->data_length = sizeof(no_data_response);
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

/*
 * ******************************************************************************
 *  Function Name: protocol_process_ack (LOCAL FUNCTION)
 *  @brief     : Clear after being read the sucess flag
 *  @param     : None
 *  @retval    : None
 * ******************************************************************************
 */
void protocol_process_ack()
{
	if((ACK_PHASE)g_register_data[ACK].value[0] == ACK_SUCESS)
	{
		g_register_data[ACK].value[0] = ACK_IDLE;
	}
}

void nascerr_log_register_tx_result(StatusCommTypeDef status_comm, RegistersTypeDef reg){
	switch (reg) {
		case TXB:
			if(!(status_comm == COMMUNICATION_SUCCESS))
			{
				LOG_AND_NOTIFY(LOG_TRANSMIT_FINISH, LOG_STATUS_ERROR, ERR_I2C_TRANSFER);
			}
			break;
		case HW_TXB:
			if(!(status_comm == COMMUNICATION_SUCCESS))
			{
				LOG_AND_NOTIFY(LOG_TRANSMIT_HW_FINISH, LOG_STATUS_ERROR, ERR_I2C_TRANSFER);
			}
			break;
		default:
			break;
	}
}


