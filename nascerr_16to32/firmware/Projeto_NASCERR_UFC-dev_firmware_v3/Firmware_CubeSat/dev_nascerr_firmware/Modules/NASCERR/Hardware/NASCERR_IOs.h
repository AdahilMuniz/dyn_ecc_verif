/**
 * ******************************************************************************
 *  @file    : NASCERR_IOs.h
 *  @brief   : Header for nascerr_ios.c file.
 *  @author  : Clara
 * ******************************************************************************
 */

#ifndef INC_NASCERR_IOS_H_
#define INC_NASCERR_IOS_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "main.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */
void nascerr_io_pin_set(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void nascerr_io_pin_reset(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
GPIO_PinState nascerr_io_read_pin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void nascerr_io_delay(uint32_t delay);

#endif /* INC_NASCERR_IOS_H_ */
