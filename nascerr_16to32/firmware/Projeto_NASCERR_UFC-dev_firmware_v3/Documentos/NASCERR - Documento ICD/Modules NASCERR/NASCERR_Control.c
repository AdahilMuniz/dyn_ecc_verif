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

NASCERR_CTRL *ctrl_resp;

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


/**
 * ******************************************************************************
 *  Function Name: nascerr_control_castingCMD
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

uint8_t nascerr_control_castingCMD(NASCERR_CTRL *ctrl, uint8_t *cmd_value) {

	ctrl_resp = (NASCERR_CTRL *)cmd_value;

    // Converte os valores do vetor CMD para os campos da estrutura NASCERR_CTRL
    ctrl->bytes_msg = (cmd_value[0] << 8) | cmd_value[1]; //Ajustar         					// 2 bytes para bytes_msg (uint16_t)
    ctrl->cmd_type = (CmdTypeDef) cmd_value[2];                    								// 1 byte para cmd_type (uint8_t)
    ctrl->id = cmd_value[3]; // Implementor no master             								// 1 byte para id (uint8_t) -> Ainda não está implementado no python
    ctrl->data_lenght = (nascerr_block)                             							// 4 bytes para data_lenght (uint32_t)
        ((cmd_value[4] << 24) | (cmd_value[5] << 16) | (cmd_value[6] << 8) | cmd_value[7]);
    ctrl->repeat = (nascerr_rpt) ((cmd_value[8] << 8) | cmd_value[9]);  						// 2 bytes para repeat (uint16_t)
    ctrl->write_type = (W_DataTypeDef) cmd_value[10];              								// 1 byte para write_type (uint8_t)
    ctrl->data = (Seq_ReadTypeDef) cmd_value[11];                  								// 1 byte para data (uint8_t)
    ctrl->mode = (NASCERR_MODE) cmd_value[12];                     								// 1 byte para mode (uint8_t)
    ctrl->delay = (cmd_value[13] << 8) | cmd_value[14]; // 2 bytes para delay (uint16_t)
    ctrl->imagem_exp.image = (ImageTypeDef) cmd_value[15];
    ctrl->imagem_exp.filter = (FilterTypeDef) cmd_value[16];
    ctrl->imagem_exp.compression = (CompTypeDef) cmd_value[17];
    ctrl->crc16 = (cmd_value[18] << 8) | cmd_value[19]; 										// 2 bytes para crc16 (uint16_t)
    return nascerr_control_dataLenght(ctrl);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_control_dataLenght
 *  @brief     : Validates if the data length in the NASCERR_CTRL structure is within
 *               the allowed maximum length.
 *  @param     : ctrl - Pointer to a NASCERR_CTRL structure
 *  @retval    : uint8_t - Returns 1 (true) if the data length is valid, otherwise 0 (false)
 * ******************************************************************************
 */
uint8_t nascerr_control_dataLenght(NASCERR_CTRL *ctrl)
{
	if(ctrl->data_lenght > REG_MAX_DATA_LENGHT){
		return false;
	} else{
		return true;
	}
}



