/*
 * NASCERR_IOs.h
 *
 *  Created on: Sep 5, 2024
 *      Author: Clara
 */

#ifndef INC_NASCERR_IOS_H_
#define INC_NASCERR_IOS_H_

void nascerr_io_pin_set(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void nascerr_io_pin_reset(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void nascerr_io_delay(uint32_t delay);

#endif /* INC_NASCERR_IOS_H_ */
