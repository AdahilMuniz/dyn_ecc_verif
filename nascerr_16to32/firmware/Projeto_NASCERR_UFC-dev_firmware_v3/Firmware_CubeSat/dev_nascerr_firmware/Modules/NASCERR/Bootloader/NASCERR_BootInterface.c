/**
 * ******************************************************************************
 *  @file    : NASCERR_ bootInterface.h
 *  @brief   : NASCERR interface for bootloader.
 *  @author  : Joao Manoel
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "NASCERR_BootInterface.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */
static nascerr_boot_comm_handler_t commHandler;
extern UART_Buffer_t rxBuffer;
static NASCERR_COMMANDS new_boot_command;

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: uart_receive_data
 *  @brief       : Recebe dados da UART e preenche a estrutura BOOT_DATA_PACKET.
 *  @param       : packet – Ponteiro para estrutura BOOT_DATA_PACKET.
 *  @retval      : 1 se sucesso, 0 caso contrário.
 * ******************************************************************************
 */
static uint8_t uart_receive_data(BOOT_DATA_PACKET *packet)
{
    if (rxBuffer.state == UART_PKG_OK)
    {
        packet->interface = COMM_UART;
        packet->dataSize = (rxBuffer.buffer[rxBuffer.tail + 1] << 8) |
                            rxBuffer.buffer[rxBuffer.tail];
        packet->data = &rxBuffer.buffer[rxBuffer.tail + 3];
        return 1;
    }
    return 0;
}

/*
 * ******************************************************************************
 *  Function Name: i2c_receive_data
 *  @brief       : Recebe dados do barramento I2C e processa o comando recebido.
 *  @param       : packet – Ponteiro para estrutura BOOT_DATA_PACKET.
 *  @retval      : 1 se sucesso, 0 caso contrário.
 * ******************************************************************************
 */
static uint8_t i2c_receive_data(BOOT_DATA_PACKET *packet)
{
    if (nascerr_protocol_receivedNewBootData())
    {
        new_boot_command = nascerr_protocol_processBootData();
        if (new_boot_command == BOOT_COMMAND)
        {
            return nascerr_control_castingBootCMD(packet, g_register_data[BOOT_DATA].value);
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Funções internas para envio de dados via barramentos                      */
/* ------------------------------------------------------------------------- */

/*
 * ******************************************************************************
 *  Function Name: uart_send_data
 *  @brief       : Envia dados via UART (stub atual).
 *  @param       : packet – Ponteiro para estrutura NASCERR_BOOT_STATUS.
 *  @retval      : 1 (implementação de envio ainda não definida).
 * ******************************************************************************
 */
static uint8_t uart_send_data(NASCERR_BOOT_STATUS *packet)
{
    // return UART_Write(packet->cmd, packet->data, packet->dataSize);
    return 1;
}

/*
 * ******************************************************************************
 *  Function Name: i2c_send_data
 *  @brief       : Envia dados via I2C através do registrador de status.
 *  @param       : packet – Ponteiro para estrutura NASCERR_BOOT_STATUS.
 *  @retval      : 1 (sucesso).
 * ******************************************************************************
 */
static uint8_t i2c_send_data(NASCERR_BOOT_STATUS *packet) // verificar se realmente é um registro de boot, ou se flags
{
//    nascerr_registers_changeBootStatus(packet->OP_BOOT, packet->value);
    return 1;
}


/*
 * ******************************************************************************
 *  Public Functions
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: nascerr_boot_comm_init
 *  @brief       : Inicializa a interface de comunicação com o tipo selecionado.
 *  @param       : interface – Tipo da interface (COMM_UART ou COMM_I2C).
 *  @retval      : none
 * ******************************************************************************
 */
void nascerr_boot_comm_init(BootCommInterface_t interface)
{
    if (interface == COMM_UART)
    {
        commHandler.receive = uart_receive_data;
        commHandler.send = uart_send_data;
    }
    else if (interface == COMM_I2C)
    {
        commHandler.receive = i2c_receive_data;
        commHandler.send = i2c_send_data;
    }
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_boot_comm_receive
 *  @brief       : Chama a função de recepção conforme a interface inicializada.
 *  @param       : packet – Ponteiro para estrutura BOOT_DATA_PACKET.
 *  @retval      : 1 se dados recebidos com sucesso, 0 caso contrário.
 * ******************************************************************************
 */
uint8_t nascerr_boot_comm_receive(BOOT_DATA_PACKET *packet)
{
    if (commHandler.receive)
    {
        return commHandler.receive(packet);
    }
    return false;
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_boot_comm_send
 *  @brief       : Chama a função de envio conforme a interface inicializada.
 *  @param       : packet – Ponteiro para estrutura NASCERR_BOOT_STATUS.
 *  @retval      : 1 se enviado com sucesso, 0 caso contrário.
 * ******************************************************************************
 */
uint8_t nascerr_boot_comm_send(NASCERR_BOOT_STATUS *packet) {
	if (commHandler.send) {
		return commHandler.send(packet);
	}
	return false;
}
