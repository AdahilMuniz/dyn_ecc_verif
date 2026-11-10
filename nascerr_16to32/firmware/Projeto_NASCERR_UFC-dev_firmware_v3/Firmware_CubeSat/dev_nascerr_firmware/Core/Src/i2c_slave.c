/**
 * ******************************************************************************
 *  @file    : i2c_slave.c
 *  @brief   : Source file for I2C Slave Mode.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "i2c_slave.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

extern ProtocolTypeDef g_comm_config;
extern I2C_HandleTypeDef hi2c1;
extern IWDG_HandleTypeDef hiwdg;

#if SOFTWARE_DEBUG
extern volatile uint32_t g_i2c_last_kick_ms;
extern volatile uint8_t  g_i2c_timeout_flag;
extern uint32_t systimer_counter;
#endif

PARAM_STANDARD transmitted, received = Action_Succeeded;

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name : i2c_slave_init
 *  @brief        : Initializes the I2C Slave mode.
 *  @param        : None
 *  @retval       : SYSTEM_STATUS - Status of the operation.
 * ******************************************************************************
 */
SYSTEM_STATUS i2c_slave_init(void)
{
	g_comm_config.addr = (uint8_t)((hi2c1.Init.OwnAddress1) >> 1);
	nascerr_protocol_init(&g_comm_config);


	if (HAL_I2C_EnableListen_IT(&hi2c1) != HAL_OK)
	{
	  return ERR_INIT;
	}

	return STATUS_OK;
}

/*
 * ******************************************************************************
 *  Function Name : i2c_slave_reset
 *  @brief        : Resets the I2C Slave configuration.
 *  @param        : None
 *  @retval       : None
 * ******************************************************************************
 */
void i2c_slave_reset(void)
{
	HAL_I2C_DisableListen_IT(&hi2c1);
	HAL_I2C_DeInit(&hi2c1);
	HAL_I2C_Init(&hi2c1);
	HAL_I2C_EnableListen_IT(&hi2c1);
}

/*
 * ******************************************************************************
 *  Function Name : i2c_slave_receive
 *  @brief        : Receives data in I2C Slave mode.
 *  @param        : uint8_t *pData - Pointer to the buffer where received data will be stored.
 *                  uint16_t Size - Size of the data buffer.
 *  @retval       : SYSTEM_STATUS - Status of the operation.
 * ******************************************************************************
 */
SYSTEM_STATUS i2c_slave_receive(uint8_t *pData, uint16_t Size)
{
	uint8_t error_com = false;
	uint32_t XferOptions;
	StatusCommTypeDef comm_status = g_comm_config.status_comm;

	switch(comm_status) {
		case COMMUNICATION_INIT:
			XferOptions = I2C_FIRST_FRAME;
			break;
		case COMMUNICATION_INPROCESS:
			XferOptions = I2C_NEXT_FRAME;
			break;
		case PROCESSING_DATA:
			XferOptions = I2C_LAST_FRAME;
			break;
		default:
			XferOptions = I2C_LAST_FRAME;
			error_com = true;
			break;
	}

	if ((HAL_I2C_Slave_Seq_Receive_IT(&hi2c1, pData, Size, XferOptions) != HAL_OK) || error_com == true)
	{
		__HAL_I2C_GENERATE_NACK(&hi2c1);
		return ERR_COMM;
	}

	received = Action_Failed;

	return STATUS_OK;
}

/*
 * ******************************************************************************
 *  Function Name : i2c_slave_send
 *  @brief        : Sends data from the I2C Slave.
 *  @param        : uint8_t *pData - Pointer to the data buffer to be sent.
 *                  uint16_t Size - Size of the data buffer.
 *  @retval       : SYSTEM_STATUS - Status of the operation.
 * ******************************************************************************
 */
SYSTEM_STATUS i2c_slave_send(uint8_t *pData, uint16_t Size)
{
	if (HAL_I2C_Slave_Seq_Transmit_IT(&hi2c1, pData, Size, I2C_LAST_FRAME) != HAL_OK)
	{
		return ERR_COMM;
	}
	transmitted = Action_Failed;

	return STATUS_OK;
}

/*
 * ******************************************************************************
 *  Function Name : i2c_slave_noBlockBus
 *  @brief        : Checks the I2C bus state in Slave mode and refreshes the
 *                  watchdog if in "Listen" state to avoid a system reset.
 *  @param        : None
 *  @retval       : None
 * ******************************************************************************
 */
void i2c_slave_noBlockBus(void)
{
    HAL_I2C_StateTypeDef stateI2C = HAL_I2C_GetState(&hi2c1);

    if (stateI2C == HAL_I2C_STATE_LISTEN)
    {
#if !SOFTWARE_DEBUG
        HAL_IWDG_Refresh(&hiwdg);
#else
        g_i2c_last_kick_ms = systimer_counter;
#endif
    }
}

/*
 * ******************************************************************************
 ** HAL STm32 Functions
 * ******************************************************************************
 */

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
	HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{

	StatusCommTypeDef comm_status;

	comm_status = nascerr_protocol_poll(&g_comm_config, (BusTypeDef) TransferDirection);

	if (comm_status == COMMUNICATION_FAILURE)
	{
		nascerr_protocol_commInit(&g_comm_config);
		HAL_I2C_DisableListen_IT(hi2c);
		HAL_I2C_DeInit(hi2c);
		HAL_I2C_Init(hi2c);
	}

}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	transmitted = Action_Succeeded;
	nascerr_protocol_commInit(&g_comm_config);
	HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	StatusCommTypeDef comm_status;

	received = Action_Succeeded;
	comm_status = nascerr_protocol_poll(&g_comm_config, RECEIVING_DATA);

	if (comm_status == PROCESSING_DATA)
	{
		nascerr_protocol_poll(&g_comm_config, LISTENING_MODE);
		nascerr_protocol_commInit(&g_comm_config);
		HAL_I2C_EnableListen_IT(hi2c);
	}

	if (comm_status == COMMUNICATION_FAILURE)
	{
		nascerr_protocol_commInit(&g_comm_config);
		HAL_I2C_DisableListen_IT(hi2c);
		HAL_I2C_DeInit(hi2c);
		HAL_I2C_Init(hi2c);
		HAL_I2C_EnableListen_IT(hi2c);
	}

}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	//HAL_I2C_ERROR_NONE       0x00000000U    /*!< No error           */
	//HAL_I2C_ERROR_BERR       0x00000001U    /*!< BERR error         */
	//HAL_I2C_ERROR_ARLO       0x00000002U    /*!< ARLO error         */
	//HAL_I2C_ERROR_AF         0x00000004U    /*!< Ack Failure error  */
	//HAL_I2C_ERROR_OVR        0x00000008U    /*!< OVR error          */
	//HAL_I2C_ERROR_DMA        0x00000010U    /*!< DMA transfer error */
	//HAL_I2C_ERROR_TIMEOUT    0x00000020U    /*!< Timeout Error      */
	uint32_t error_code = HAL_I2C_GetError(hi2c);
	if (error_code != HAL_I2C_ERROR_AF){}
	HAL_I2C_EnableListen_IT(hi2c);

}
