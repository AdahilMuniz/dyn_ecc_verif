/*
 * experimento.h
 *
 *  Created on: Aug 28, 2024
 *      Author: Clara
 */

#ifndef INC_NASCERR_EXPERIMENT_H_
#define INC_NASCERR_EXPERIMENT_H_

#include <stdint.h>
#include "stm32l4xx_hal.h"
#include "NASCERR_Control.h"
#include "NASCERR_IOs.h"


// Funções:
void nascerr_experiment_select_ecc(uint8_t ecc);
NASCERR_RESPONSE nascerr_experiment_test(NASCERR_CTRL command);
void nascerr_experiment_testrw(uint8_t* read8b, uint16_t* read16b, uint32_t* read32b);
uint32_t nascerr_experiment_error_counter(uint32_t length, uint16_t* data_write, uint16_t* data_read);
uint32_t nascerr_experiment_buffer_result(uint32_t length, uint16_t* data_write, uint16_t* data_read, ResDescTypeDef error_buffer[length]);
#endif /* INC_NASCERR_EXPERIMENT_H_ */
