/**
 * ******************************************************************************
 *  @file    : NASCERR_Control.c
 *  @brief   : Projeto NASCERR – Nanosatelite com Eletrônica Robusta a Radiação
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "NASCERR_Control.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */


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
 *  Function Name: nascerr_protocol_init
 *  @brief     : Initialize the communication protocol
 *  @param     : comm_config - Pointer to the communication configuration structure
 *  @retval    : none
 * ******************************************************************************
 */

/**
 * ******************************************************************************
 *  Function Name: nascerr_casting
 *  @brief     : Converts the CMD register data into a NASCERR_CTRL structure
 *  @param     : ctrl - Pointer to the NASCERR_CTRL structure to be filled
 *               cmd_value - Pointer to the CMD register data array (uint8_t)
 *  @retval    : none
 * ******************************************************************************
 *  @details   : This function takes the data from the `cmd_value` array and
 *               populates the corresponding fields in the NASCERR_CTRL structure.
 *               Multi-byte fields are handled by shifting and combining bytes
 *               from the `cmd_value` array.
 *
 *               Conversion process:
 *               - The first two bytes are used to form `bytes_msg` (16-bit).
 *               - The remaining fields like `data_lenght`, `repeat`, `delay`,
 *                 and `crc16` are constructed from multiple bytes.
 * ******************************************************************************
 */

void nascerr_casting(NASCERR_CTRL *ctrl, uint8_t *cmd_value) {


    // Converte os valores do vetor CMD para os campos da estrutura NASCERR_CTRL
    ctrl->bytes_msg = (cmd_value[1] << 8) | cmd_value[0]; //Ajustar         					// 2 bytes para bytes_msg (uint16_t)
    ctrl->cmd_type = (CmdTypeDef) cmd_value[0];                    								// 1 byte para cmd_type (uint8_t)
    ctrl->id = 01; // Implementor no master             								// 1 byte para id (uint8_t) -> Ainda não está implementado no python
    ctrl->data_lenght = (nascerr_block)                             							// 4 bytes para data_lenght (uint32_t)
        ((cmd_value[4] << 24) | (cmd_value[3] << 16) | (cmd_value[2] << 8) | cmd_value[1]);
    ctrl->repeat = (nascerr_rpt) ((cmd_value[6] << 8) | cmd_value[5]);  						// 2 bytes para repeat (uint16_t)
    ctrl->write_type = (W_DataTypeDef) cmd_value[7];              								// 1 byte para write_type (uint8_t)
    ctrl->data = (Seq_ReadTypeDef) cmd_value[8];                  								// 1 byte para data (uint8_t)
    ctrl->mode = (NASCERR_MODE) cmd_value[9];                     								// 1 byte para mode (uint8_t)
    ctrl->delay = (cmd_value[11] << 8) | cmd_value[10];            								// 2 bytes para delay (uint16_t)
    ctrl->crc16 = (cmd_value[13] << 8) | cmd_value[12];            								// 2 bytes para crc16 (uint16_t)
}


//vericações: quando recebe verificar se está chegando o registro do Rx
//Tratamento da mensagem caso recebou: Verificar o tamanho que recebeu e verificar se o CRG está igual
//fazer a conversão apenas quando verificar o passo anterior
//
