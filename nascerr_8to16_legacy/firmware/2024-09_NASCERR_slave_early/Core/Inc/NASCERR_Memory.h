/*
 * NASCERR_Memory.h
 *
 *  Created on: Sep 3, 2024
 *      Author: Clara
 */

#ifndef INC_NASCERR_MEMORY_H_
#define INC_NASCERR_MEMORY_H_

#include <stdint.h>
#include <stdlib.h>
#include "stm32l4xx_hal.h"
#include "NASCERR_Control.h"
#include "NASCERR_Experiment.h"


#define NASCERR_SRAM_ADDRESS 0x60000000UL // Base address for SRAM


void nascerr_memory_write_sram(uint16_t* write_buffer, uint32_t address, NASCERR_MODE ecc, W_DataTypeDef write_type, uint32_t length);
//void nascerr_memory_write_sram8b(SRAM_HandleTypeDef *hsram, uint8_t* write_buffer, uint32_t address, NASCERR_MODE ecc, W_DataTypeDef write_type, uint32_t length);
void nascerr_memory_read_sram(uint16_t* read_buffer, uint32_t address, NASCERR_MODE ecc, uint32_t length);
void nascerr_memory_fill_buffer(uint16_t* buffer, W_DataTypeDef write_type, uint32_t length);

#endif /* INC_NASCERR_MEMORY_H_ */
