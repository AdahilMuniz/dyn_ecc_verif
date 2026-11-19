/**
 * @file    : stm_flash_memory.c
 * @brief   : Library for managing UART communication with circular buffer and CRC validation.
 * @details : This code implements functionalities for initializing and managing data transmission/reception via UART.
 *            It uses a circular buffer to store received data, handles timeout checks,
 *            and validates packets using CRC.
 *
 * @author  : Joao Manoel
 * @date    : Current date (add the date here)
 *
 * @note    : Ensure that buffer sizes (`UART_BUFFER_SIZE`) and other macros are defined correctly in your project.
 */

/**
 * @section Includes
 * Includes necessary header files.
 */

#include "uart_handler.h"

/**
 * @brief Global UART handler used throughout the library.
 * @brief Circular buffer structure to manage received data.
 */

static UART_HandleTypeDef *uartHandle;
UART_Buffer_t rxBuffer;
static UART_RxEventCallback_t s_rx_event_cb = NULL;
static UART_RxEventCallback_t s_rx_error_cb = NULL;
//uint8_t g_img_dma_buf[UART_IMG_BUFFER_SIZE];

/**
 * @brief Initializes the UART communication and enables interrupt-based reception.
 *
 * @param huart Pointer to the UART handler structure.
 */

void UART_Init_Bootreceive(UART_HandleTypeDef *huart)
{
    uartHandle = huart;
    HAL_UART_Receive_IT(uartHandle, &rxBuffer.buffer[rxBuffer.head], 1);
}

/**
 * @brief Checks for timeout in data reception.
 *
 * @param timeout_ms Timeout duration in milliseconds.
 * @return HAL_OK if no timeout occurred, HAL_ERROR otherwise.
 */

uint8_t UART_CheckTimeout(uint32_t timeout_ms)
{
    uint32_t currentTime = HAL_GetTick();
    if (((currentTime - rxBuffer.lestTime) > timeout_ms) && (rxBuffer.head>1))
    {
        rxBuffer.state = UART_TIMEOUT;
        return true;
    }
    return false;
}

/**
 * @brief UART interrupt handler.
 *
 * @param huart Pointer to the UART handler structure.
 */

void UART_IRQHandler(UART_HandleTypeDef *huart)
{
    if (huart->Instance == uartHandle->Instance)
    {
        uint16_t nextHead = (rxBuffer.head + 1) % UART_BOOT_BUFFER_SIZE;
        if (nextHead != rxBuffer.tail)
        {
            rxBuffer.head = nextHead;
            rxBuffer.state = UART_RECEIVING;
        }

        UART_CheckTimeout(TIMER_10ms);
        UART_Available();

        if (rxBuffer.state == UART_RECEIVING || rxBuffer.state == UART_OK)
        {
			rxBuffer.lestTime = HAL_GetTick();
            HAL_UART_Receive_IT(uartHandle, &rxBuffer.buffer[rxBuffer.head], 1);
        }
    }
}

/**
 * @brief Handles UART data reception and clears error flags.
 */

void UART_receive(void){
    __HAL_UART_CLEAR_OREFLAG(uartHandle);

    while (__HAL_UART_GET_FLAG(uartHandle, UART_FLAG_RXNE)) {
     (void)(uartHandle->Instance->RDR);
    }

    HAL_UART_Receive_IT(uartHandle, &rxBuffer.buffer[rxBuffer.head], 1);
}

/**
 * @brief Checks if data is available in the buffer and validates complete packets.
 *
 * @return Current buffer state (UART_PKG_OK, UART_ERROR, etc.).
 */

uart_buff_states UART_Available(void)
{
	uint16_t availableBytes;
	availableBytes = (UART_BOOT_BUFFER_SIZE + rxBuffer.head - rxBuffer.tail) % UART_BOOT_BUFFER_SIZE;
	if(availableBytes>=2)
	{
    uint16_t dataSize = (rxBuffer.buffer[rxBuffer.tail+1] << 8) | (rxBuffer.buffer[rxBuffer.tail]);
    uint16_t packetLength = dataSize + 5;

		if (availableBytes >= packetLength || dataSize > REG_DATA_BOOT_SIZE)
		{
			uint16_t crc_received = (rxBuffer.buffer[rxBuffer.head - 1] << 8) | rxBuffer.buffer[rxBuffer.head - 2];
			if (verify_crc(&rxBuffer.buffer[rxBuffer.tail], packetLength - 2, crc_received) && (availableBytes == packetLength))
			{
				rxBuffer.state = UART_PKG_OK;
//				Protocol_StateMachine();
				return UART_PKG_OK;
			} else
			{
				rxBuffer.state = UART_ERROR;
				return UART_ERROR;
			}
		}
	}
    return rxBuffer.state;
}

void UART_ImgDMA_Init(UART_HandleTypeDef *huart, uint8_t *buf, uint16_t buff_size)
{
    while (__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE))
        (void)huart->Instance->RDR;

    HAL_StatusTypeDef st = HAL_UARTEx_ReceiveToIdle_DMA(huart, buf, buff_size);

    if (st != HAL_OK)
    {
        huart->RxState       = HAL_UART_STATE_READY;
        huart->hdmarx->State = HAL_DMA_STATE_READY;
        huart->hdmarx->Lock  = HAL_UNLOCKED;
        st = HAL_UARTEx_ReceiveToIdle_DMA(huart, buf, buff_size);
    }

    if (st == HAL_OK)
    {
        __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT);
        __HAL_UART_DISABLE_IT(huart, UART_IT_ERR);  /* ← desabilita NE/FE/ORE interrupt */
    }
}

void UART_RegisterRxEventCallback(UART_RxEventCallback_t cb)
{
    s_rx_event_cb = cb;
}

void UART_RegisterErrorCallback(UART_RxEventCallback_t cb)
{
    s_rx_error_cb = cb;
}

/* Weak symbol do HAL — implementado aqui, na camada de periférico */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    if (s_rx_event_cb != NULL)
        s_rx_event_cb(huart, size);
}

/* Chamado pelo HAL quando NE/FE/ORE aborta o DMA */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (s_rx_error_cb != NULL)
        s_rx_error_cb(huart, 0u);
}

/**
 * @brief Sends data over UART with packet encapsulation and CRC.
 *
 * @param command Command byte to be sent.
 * @param data Pointer to the data to be sent.
 * @param dataSize Size of the data to be sent.
 * @return HAL_OK if transmission succeeded, HAL_ERROR otherwise.
 */

uint8_t UART_Write(uint8_t command, const uint8_t *data, uint32_t dataSize)
{
    if (uartHandle == NULL)
    {
        return false;
    }

    uint16_t packetSize = (2 + 1 + dataSize + 2);
    uint8_t packet[UART_BOOT_BUFFER_SIZE];

    packet[0] = (dataSize >> 8) & 0xFF;
    packet[1] = dataSize & 0xFF;
    packet[2] = command;

    if (dataSize > 0)
    {
        memcpy(&packet[3], data, dataSize);
    }

    uint16_t crc = calculate_crc16(packet, 2 + 1 + dataSize);
    packet[3 + dataSize] = (crc >> 8) & 0xFF;
    packet[4 + dataSize] = crc & 0xFF;

    if (HAL_UART_Transmit_IT(uartHandle, packet, packetSize) != HAL_OK)
    {
    	return false;
    }
    return true;
}




