/**
 * ******************************************************************************
 *  @file    : NASCERR_Memory_16_to_32.h
 *  @brief   : Header file for memory-related functions, specifically for 16-bit to 32-bit memory operations.
 *             Provides functions for memory manipulation, including reading and writing
 *             to SRAM with error correction codes (ECC).
 *  @author  : Clara
 * ******************************************************************************
 */

#ifndef INC_NASCERR_MEMORY_32BITS_H_
#define INC_NASCERR_MEMORY_32BITS_H_

/*
 * ******************************************************************************
 *  Includes
 * ******************************************************************************
 */
#include <stdlib.h>
#include "NASCERR_Protocol.h"
#include "NASCERR_IOs.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */
#define NASCERR_SRAM_ADDRESS 0x60000000UL
#define GPIO1 GPIOA // pins ecc_sel0 and ecc_sel1
#define ECC_SEL_0 GPIO_PIN_4
#define ECC_SEL_1 GPIO_PIN_5
#define ECC_SEL_2 GPIO_PIN_8
#define START_SABOTEUR GPIO_PIN_9

/*
 * ******************************************************************************
 *  Exported functions
 * ******************************************************************************
 */
void nascerr_memory_start_saboteur();
void nascerr_memory_select_ecc(NASCERR_MODE ecc);
void nascerr_memory_write_word_cell(uint32_t data, NASCERR_MODE ecc, uint32_t offset_addr);
uint32_t nascerr_memory_write_sram_word(NASCERR_MODE ecc, W_DataTypeDef write_type, uint32_t offset_addr);
void nascerr_memory_write_block_cell(uint32_t* write_buffer, NASCERR_MODE ecc, uint32_t offset_addr, uint32_t length);
void nascerr_memory_write_sram_block(uint32_t* write_buffer, uint32_t offset_addr, NASCERR_MODE ecc, W_DataTypeDef write_type, uint32_t length);
void nascerr_memory_read_sram_block_bypass(uint32_t* read_buffer, uint32_t offset_addr, NASCERR_MODE ecc, uint32_t length);
uint32_t nascerr_memory_read_sram_word(uint32_t offset_addr, NASCERR_MODE ecc);
uint32_t nascerr_memory_read_sram_word_bypass(uint32_t offset_addr, NASCERR_MODE ecc);
void nascerr_memory_read_sram_block(uint32_t* read_buffer, uint32_t offset_addr, NASCERR_MODE ecc, uint32_t length);
void nascerr_memory_fill_buffer(uint32_t* buffer, W_DataTypeDef write_type, uint32_t length);

#endif /* INC_NASCERR_MEMORY_32BITS_H_ */
