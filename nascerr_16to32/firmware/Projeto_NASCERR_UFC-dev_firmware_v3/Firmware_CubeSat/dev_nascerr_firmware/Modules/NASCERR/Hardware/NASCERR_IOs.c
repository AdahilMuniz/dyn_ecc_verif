/**
 * ******************************************************************************
 *  @file    : NASCERR_IOs.c
 *  @brief   : Header file for I/O functions related to GPIO pin operations.
 *             Provides functions to set, reset, read, and delay on GPIO pins.
 *  @author  : Clara
 *  @note    :
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "NASCERR_IOs.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: nascerr_io_pin_set
 *  @brief       : Sets the specified GPIO pin to a high logic level (1).
 *  @param       : GPIOx - Pointer to the GPIO port where the pin is located.
 *                 GPIO_Pin - The specific GPIO pin to be set high.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_io_pin_set(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, 1);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_io_pin_reset
 *  @brief       : Resets the specified GPIO pin to a low logic level (0).
 *  @param       : GPIOx - Pointer to the GPIO port where the pin is located.
 *                 GPIO_Pin - The specific GPIO pin to be reset low.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_io_pin_reset(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, 0);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_io_read_pin
 *  @brief       : Reads the current state (high or low) of the specified GPIO pin.
 *  @param       : GPIOx - Pointer to the GPIO port where the pin is located.
 *                 GPIO_Pin - The specific GPIO pin to be read.
 *  @retval      : GPIO_PinState - Returns the current state of the GPIO pin:
 *                                  1 (GPIO_PIN_SET) or 0 (GPIO_PIN_RESET).
 * ******************************************************************************
 */
GPIO_PinState nascerr_io_read_pin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	return HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_io_delay
 *  @brief       : Inserts a delay for a specified number of milliseconds.
 *  @param       : delay - The number of milliseconds to delay.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_io_delay(uint32_t delay)
{
	vTaskDelay(pdMS_TO_TICKS(delay));
}
