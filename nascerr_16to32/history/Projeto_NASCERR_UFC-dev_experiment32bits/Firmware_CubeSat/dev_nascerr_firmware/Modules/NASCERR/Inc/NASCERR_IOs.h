/**
 * ******************************************************************************
 *  @file    : NASCERR_IOs.h
 *  @brief   : Header file for I/O functions related to GPIO pin operations.
 *             Provides functions to set, reset, read, and delay on GPIO pins.
 *  @author  : Clara
 * ******************************************************************************
 */

#ifndef INC_NASCERR_IOS_H_
#define INC_NASCERR_IOS_H_

/*
 * ******************************************************************************
 *  Exported functions
 * ******************************************************************************
 */
void nascerr_io_pin_set(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void nascerr_io_pin_reset(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
GPIO_PinState nascerr_io_read_pin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void nascerr_io_delay(uint32_t delay);

#endif /* INC_NASCERR_IOS_H_ */
