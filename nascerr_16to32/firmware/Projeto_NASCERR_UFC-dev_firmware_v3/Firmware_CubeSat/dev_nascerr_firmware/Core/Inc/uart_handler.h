/**
 * ******************************************************************************
 * @file    uart_handler.h
 * @brief   UART handler library for managing UART communication and buffer states.
 *
 * @author  : Joao Manoel
 * @details This header defines the structures, constants, and function prototypes
 *          for handling UART communication, including initializing the UART,
 *          managing a circular buffer, and handling UART interrupts.
 * ******************************************************************************
 */

#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include "main.h"
#include "NASCERR_SysTime.h"
#include "NASCERR_Registers.h"
#include "NASCERR_Util.h"
#include "usart.h"
#include <string.h>

//#define MAX_DATA_SIZE		256  		/**< Maximum size for UART data payload. */
#define UART_BOOT_BUFFER_SIZE 	528  		/**< Size of the circular buffer for UART data storage. */
//#define UART_IMG_BUFFER_SIZE 	2064

/**
 * @enum uart_buff_states
 * @brief Enumeration of UART buffer states.
 */
typedef enum {
    UART_OK = 0,        				/**< No errors, UART buffer is ready. */
    UART_RECEIVING,     				/**< Data is being received. */
    UART_PKG_OK,        				/**< A complete package has been received. */
    UART_TIMEOUT,       				/**< UART timeout occurred. */
    UART_ERROR          				/**< UART error occurred. */
} uart_buff_states;

/**
 * @struct UART_Buffer_t
 * @brief Circular buffer structure for UART communication.
 */
typedef struct {
    uint8_t buffer[REG_DATA_BOOT_SIZE]; 	/**< Data buffer for storing UART data. */
    volatile uint16_t head;           	/**< Head pointer for the circular buffer. */
    volatile uint16_t tail;           	/**< Tail pointer for the circular buffer. */
    volatile uint32_t lestTime;       	/**< Timestamp of the last received data. */
    volatile uart_buff_states state;  	/**< Current state of the UART buffer. */
} UART_Buffer_t;


typedef void (*UART_RxEventCallback_t)(UART_HandleTypeDef *huart, uint16_t size);

void UART_RegisterRxEventCallback(UART_RxEventCallback_t cb);
void UART_RegisterErrorCallback(UART_RxEventCallback_t cb);

void UART_Init_Bootreceive(UART_HandleTypeDef *huart);

void UART_ImgDMA_Init(UART_HandleTypeDef *huart, uint8_t *buf, uint16_t buff_size);

void UART_IRQHandler(UART_HandleTypeDef *huart);

uart_buff_states UART_Available(void);

uint8_t UART_CheckTimeout(uint32_t timeout_ms);

uint8_t UART_Write(uint8_t command, const uint8_t *data, uint32_t dataSize);

void UART_receive(void);


#endif // UART_HANDLER_H
