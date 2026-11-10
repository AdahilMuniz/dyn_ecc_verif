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
#include "NASCERR_IOs.h"


#define NASCERR_SRAM_ADDRESS 0x60000000UL // Base address for SRAM
#define GPIO_ECC GPIOC


void nascerr_memory_start_saboteur();
void nascerr_memory_select_ecc(NASCERR_MODE ecc);
void nascerr_memory_write_word_cell(uint16_t data, NASCERR_MODE ecc, uint32_t offset_addr);
uint16_t nascerr_memory_write_sram_word(NASCERR_MODE ecc, W_DataTypeDef write_type, uint32_t offset_addr);
void nascerr_memory_write_block_cell(uint16_t* write_buffer, NASCERR_MODE ecc, uint32_t offset_addr, uint32_t length);
void nascerr_memory_write_sram_block(uint16_t* write_buffer, uint32_t address, NASCERR_MODE ecc, W_DataTypeDef write_type, uint32_t length);
uint16_t nascerr_memory_read_sram_word(uint32_t offset_addr, NASCERR_MODE ecc);
void nascerr_memory_read_sram_block(uint16_t* read_buffer, uint32_t address, NASCERR_MODE ecc, uint32_t length);
void nascerr_memory_fill_buffer(uint16_t* buffer, W_DataTypeDef write_type, uint32_t length);


#endif /* INC_NASCERR_MEMORY_H_ */
