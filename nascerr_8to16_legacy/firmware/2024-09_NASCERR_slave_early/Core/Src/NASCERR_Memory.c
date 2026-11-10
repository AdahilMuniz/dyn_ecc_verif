/*
 * NASCERR_Memory.c
 *
 *  Created on: Sep 3, 2024
 *      Author: Clara
 */

#include "NASCERR_Memory.h"


volatile uint16_t * ptr = (uint16_t *)NASCERR_SRAM_ADDRESS;


/*
 * ******************************************************************************
 *  Function Name: nascerr_write_sram
 *  @brief       : Writes a block of data to SRAM memory.
 *  @param       : hsram - A pointer to the SRAM_HandleTypeDef structure that
 *                          contains the configuration information for SRAM.
 *  @param       : offset - A 32-bit value indicating the starting address
 *                          in SRAM where the data should be written.
 *  @param       : data - An 8-bit value specifying the type of data to write.
 *                        Possible values:
 *                        00 - Fill the block with zeros.
 *                        01 - Fill the block with ones (0xFFFF).
 *                        02 - Fill the block with 0x5555.
 *                        03 - Fill the block with 0xAAAA.
 *                        04 - Fill the block with random 16-bit values.
 *  @param       : length - A 32-bit value specifying the size of the block to
 *                          be written in bytes. It should be an even number.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_write_sram(uint16_t* write_buffer, uint32_t address, NASCERR_MODE ecc, W_DataTypeDef write_type, uint32_t length){
	nascerr_memory_fill_buffer(write_buffer, write_type, length); // Preenche o buffer com o tipo de dado indicado no comando.
	nascerr_experiment_select_ecc(ecc); // seleciona o ECC.

	for(int i = 0; i < length; i++){
		*(ptr + i) = write_buffer[i];
	}
}

//void nascerr_memory_write_sram8b(SRAM_HandleTypeDef *hsram, uint8_t* write_buffer, uint32_t address, NASCERR_MODE ecc, W_DataTypeDef write_type, uint32_t length){
////	nascerr_fill_buffer(write_buffer, write_type, length); // Preenche o buffer com o tipo de dado indicado no comando.
//	nascerr_experiment_select_ecc(ecc); // seleciona o ECC.
//
//	for(int i = 0; i < length; i++){
//		*(ptr + i) = write_buffer[i];
//	}
//}


/*
 * ******************************************************************************
 *  Function Name: nascerr_read_sram
 *  @brief       : Reads a block of data from SRAM memory.
 *  @param       : hsram - A pointer to the SRAM_HandleTypeDef structure that
 *                          contains the configuration information for SRAM.
 *  @param       : offset - A 32-bit value indicating the starting address
 *                          in SRAM from where the data should be read.
 *  @param       : buffer - A pointer to a memory area where the read data
 *                          will be stored.
 *  @param       : length - A 32-bit value specifying the size of the block to
 *                          be read in bytes. It should be an even number.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_read_sram(uint16_t* read_buffer, uint32_t address, NASCERR_MODE ecc, uint32_t length){
	nascerr_experiment_select_ecc(ecc);
	for(int i = 0; i < length; i++){
		read_buffer[i] = *(ptr + address + i);
	}
}


void nascerr_memory_fill_buffer(uint16_t* buffer, W_DataTypeDef write_type, uint32_t length){
	uint8_t data_cell; // Guarda o tipo de palavra a ser armazenada.
	switch(write_type){
	// Preenche o buffer com zeros.
	case 0:
		data_cell = 0x00;
		for(uint32_t i = 0; i<(length/2); i++){
			buffer[i] = data_cell;
		}
		break;

	// Preenche o buffer com 0xFFFF.
	case 1:
		data_cell = 0xFF;
		for(uint32_t i = 0; i<(length/2); i++){
			buffer[i] = data_cell;
		}
		break;

	// Preenche o buffer com 0x5555.
	case 2:
		data_cell = 0x55;
		for(uint32_t i = 0; i<(length/2); i++){
			buffer[i] = data_cell;
		}
		break;

	// Preenche o buffer com 0xAAAA.
	case 3:
		data_cell = 0xAA;
		for(uint32_t i = 0; i<(length/2); i++){
			buffer[i] = data_cell;
		}
		break;

	// Preenche o buffer com dados aleatórios.
	case 4:
		for(uint32_t i = 0; i<(length/2); i++){
			data_cell = (rand() % 256); // Escolhe um número aleatório entre 0 e 255 (8 bits) para colocar no buffer.
			buffer[i] = data_cell;
		}
		break;
	}
}

