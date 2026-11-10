/*
 * NASCERR_Memory_32bits.c
 *
 *  Created on: Oct 31, 2024
 *      Author: Clara
 */

#include "NASCERR_Memory_32bits.h"

volatile uint16_t * ptr = (uint16_t *)NASCERR_SRAM_ADDRESS;


/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_start_saboteur
 *  @brief       : Activates the saboteur in the FPGA.
 *  @param       : None
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_start_saboteur(){
	nascerr_io_pin_set(GPIO1, START_SABOTEUR);
	nascerr_io_pin_reset(GPIO1, START_SABOTEUR);
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_select_ecc
 *  @brief       : Configures the Error Correcting Code (ECC) selection using GPIO pins.
 *  @param       : ecc - An 8-bit unsigned integer specifying the ECC type to be selected.
 *                        Possible values:
 *                        1 - BYPASS_1 - Bypass to memory 1
 *                        2 - BYPASS_2 - Bypass to memory 2
 *                        3 - TBEC - TBEC ECC
 *                        4 - TBEC_1_TO_3 - TBEC ECC with 1 to 3 errors
 *                        5 - TBEC_1_TO_7 - TBEC ECC with 1 to 7 errors
 *                        6 - BYPASS_1_TO_7 - Bypass with 1 to 7 errors
 *                        7 - MRSC - MRSC ECC
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_select_ecc(NASCERR_MODE ecc){

	switch(ecc){
	case BYPASS_1: // TBEC
		nascerr_io_pin_reset(GPIO1,ECC_SEL_0);
		nascerr_io_pin_reset(GPIO1,ECC_SEL_1);
		nascerr_io_pin_reset(GPIO1,ECC_SEL_2);
		break;

	case BYPASS_2: // TBEC_RANDOM
		nascerr_io_pin_set(GPIO1,ECC_SEL_0);
		nascerr_io_pin_reset(GPIO1,ECC_SEL_1);
		nascerr_io_pin_reset(GPIO1,ECC_SEL_2);
		break;

	case TBEC: // EDAC
		nascerr_io_pin_reset(GPIO1,ECC_SEL_0);
		nascerr_io_pin_set(GPIO1,ECC_SEL_1);
		nascerr_io_pin_reset(GPIO1,ECC_SEL_2);
		break;

	case TBEC_1_TO_3:
		nascerr_io_pin_set(GPIO1,ECC_SEL_0);
		nascerr_io_pin_set(GPIO1,ECC_SEL_1);
		nascerr_io_pin_reset(GPIO1,ECC_SEL_2);
		break;

	case TBEC_1_TO_7:
		nascerr_io_pin_reset(GPIO1,ECC_SEL_0);
		nascerr_io_pin_reset(GPIO1,ECC_SEL_1);
		nascerr_io_pin_set(GPIO1,ECC_SEL_2);
		break;

	case BYPASS_1_TO_7:
		nascerr_io_pin_set(GPIO1,ECC_SEL_0);
		nascerr_io_pin_reset(GPIO1,ECC_SEL_1);
		nascerr_io_pin_set(GPIO1,ECC_SEL_2);
		break;
	case MRSC:
		nascerr_io_pin_reset(GPIO1,ECC_SEL_0);
		nascerr_io_pin_set(GPIO1,ECC_SEL_1);
		nascerr_io_pin_set(GPIO1,ECC_SEL_2);
		break;

	default:
		break;

	}
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_write_word_cell
 *  @brief       : Writes a single 16-bit word to a specified memory cell with a
 *                 selected ECC configuration.
 *  @param data  : 32-bit data to be written to the memory cell.
 *  @param ecc   : ECC mode selected for writing.
 *  @param offset_addr : Offset address within the memory where the data will be written.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_write_word_cell(uint32_t data, NASCERR_MODE ecc, uint32_t offset_addr){
	nascerr_memory_select_ecc(ecc);
	*(ptr + offset_addr) = data;
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_write_sram_word
 *  @brief       : Writes a single 16-bit word to SRAM memory using specified ECC
 *                 and data write type.
 *  @param ecc   : ECC mode to be used during the write operation.
 *  @param write_type : Type of data to be written.
 *                        Possible values:
 *                        0 - WRITE_ZEROS
 *                        1 - WRITE_ONES
 *                        2 - WRITE_x55h
 *                        3 - WRITE_xAAh
 *                        4 - WRITE_RANDOM
 *  @param offset_addr : Offset address within the memory where the data will be written.
 *  @retval      : uint16_t - The actual data written, which is returned to update the write buffer in test functions.
 * ******************************************************************************
 */
uint32_t nascerr_memory_write_sram_word(NASCERR_MODE ecc, W_DataTypeDef write_type, uint32_t offset_addr){
	uint32_t data;
	switch(write_type){
	case WRITE_ZEROS:
		data = 0x0000;
		break;
	case WRITE_ONES:
		data = 0xFFFF;
		break;
	case WRITE_x55h:
		data = 0x5555;
		break;
	case WRITE_xAAh:
		data = 0xAAAA;
		break;
	case WRITE_RANDOM:
		data = (rand() % 65536); // Escolhe um número aleatório entre 0 e 65535 (16 bits) para escrever na memória.
		break;
	}

	nascerr_memory_write_word_cell(data, ecc, offset_addr); // escreve o dado na posição de memória

	return data; // retorna para preencher o buffer de escrita na função de teste
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_write_block_cell
 *  @brief       : Writes a block of data to consecutive memory cells, starting from
 *                 a specified address and applying a selected ECC configuration.
 *  @param write_buffer : Pointer to buffer containing data to be written.
 *  @param ecc   : ECC mode to be applied for the write operation.
 *  @param offset_addr : Starting offset address in memory for the block write.
 *  @param length : Number of data words to be written from the buffer.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_write_block_cell(uint32_t* write_buffer, NASCERR_MODE ecc, uint32_t offset_addr, uint32_t length){
	nascerr_memory_select_ecc(ecc); // seleciona o ECC.

	for(int i = 0; i < length; i++){
		*(ptr + offset_addr + i) = write_buffer[i];
	}
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_write_sram_block
 *  @brief       : Fills a buffer with a specified data pattern, then writes the buffer
 *                 as a block to consecutive memory cells, applying the selected ECC.
 *  @param write_buffer : Pointer to the buffer that will be filled and written to memory.
 *  @param offset_addr : Starting offset address for the block write operation.
 *  @param ecc   : ECC mode to apply to the data block.
 *  @param write_type : Type of data pattern to be used in filling the buffer.
 *  @param length : Number of data words in the block to be written.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_write_sram_block(uint32_t* write_buffer, uint32_t offset_addr, NASCERR_MODE ecc, W_DataTypeDef write_type, uint32_t length){
	nascerr_memory_fill_buffer(write_buffer, write_type, length); // Preenche o buffer com o tipo de dado indicado no comando.
	nascerr_memory_write_block_cell(write_buffer, ecc, offset_addr, length); // escreve o bloco de memoria

}


/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_read_sram_word
 *  @brief       : Reads a single 32-bit word from a specified memory cell with a
 *                 selected ECC configuration.
 *  @param offset_addr : Offset address within the memory from which data will be read.
 *  @param ecc   : ECC mode to be applied during the read operation.
 *  @retval      : uint32_t - The data read from the specified memory address.
 * ******************************************************************************
 */
uint32_t nascerr_memory_read_sram_word(uint32_t offset_addr, NASCERR_MODE ecc){
	uint32_t bypass1;
	uint32_t bypass2;

	if (ecc == BYPASS_1 || ecc == BYPASS_2 || ecc == BYPASS_1_TO_7){
		nascerr_memory_select_ecc(BYPASS_1);
		bypass1 = *(ptr + offset_addr) << 16;

		nascerr_memory_select_ecc(BYPASS_2);
		bypass2 = *(ptr + offset_addr);

		return bypass2 | bypass1;
	}

	else{
		nascerr_memory_select_ecc(ecc);
		return *(ptr + offset_addr);
	}

}


/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_read_sram_word_bypass
 *  @brief       : Reads a single 32-bit word from a specified memory cell with a
 *                 BYPASS.
 *  @param offset_addr : Offset address within the memory from which data will be read.
 *  @param ecc   : ECC mode to be applied during the read operation, in this case it should be a bypass mode.
 *  @retval      : uint32_t - The data read from the specified memory address.
 * ******************************************************************************
 */
uint32_t nascerr_memory_read_sram_word_bypass(uint32_t offset_addr, NASCERR_MODE ecc){
	nascerr_memory_select_ecc(ecc);
	return *(ptr + offset_addr);
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_read_sram_block
 *  @brief       : Reads a block of data from consecutive memory cells, starting
 *                 from a specified address and applying a selected ECC configuration.
 *  @param read_buffer : Pointer to buffer where the read data will be stored.
 *  @param offset_addr : Starting offset address for the block read.
 *  @param ecc   : ECC mode to be applied for the read operation.
 *  @param length : Number of data words to be read into the buffer.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_read_sram_block(uint32_t* read_buffer, uint32_t offset_addr, NASCERR_MODE ecc, uint32_t length){
	uint32_t bypass1;
	uint32_t bypass2;
	uint32_t bypass_res;

	if (ecc == BYPASS_1 || ecc == BYPASS_2 || ecc == BYPASS_1_TO_7){
		for(int i = 0; i < length; i++){
			nascerr_memory_select_ecc(BYPASS_1);
			bypass1 = *(ptr + offset_addr + i) << 16;

			nascerr_memory_select_ecc(BYPASS_2);
			bypass2 = *(ptr + offset_addr + i);

			bypass_res = bypass2 | bypass1;

			read_buffer[i] = bypass_res;
		}
	}

	else {
		nascerr_memory_select_ecc(ecc);
		for(int i = 0; i < length; i++){
			read_buffer[i] = *(ptr + offset_addr + i);
		}
	}
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_read_sram_block_bypass
 *  @brief       : Reads a block of data from consecutive memory cells, starting
 *                 from a specified address and using BYPASS.
 *  @param read_buffer : Pointer to buffer where the read data will be stored.
 *  @param offset_addr : Starting offset address for the block read.
 *  @param ecc   : ECC mode to be applied for the read operation, in this case it should be a bypass mode.
 *  @param length : Number of data words to be read into the buffer.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_read_sram_block_bypass(uint32_t* read_buffer, uint32_t offset_addr, NASCERR_MODE ecc, uint32_t length){
	nascerr_memory_select_ecc(ecc);
	for(int i = 0; i < length; i++){
		read_buffer[i] = *(ptr + offset_addr + i);
	}
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_fill_buffer
 *  @brief       : Fills a buffer with a specific data pattern or random data,
 *                 based on the specified write type.
 *  @param buffer : Pointer to the buffer to be filled.
 *  @param write_type : Type of data pattern to be used for filling the buffer.
 *                      Possible values:
 *                        0 - WRITE_ZEROS
 *                        1 - WRITE_ONES
 *                        2 - WRITE_x55h
 *                        3 - WRITE_xAAh
 *                        4 - WRITE_RANDOM
 *  @param length : Number of data words to fill in the buffer.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_fill_buffer(uint32_t* buffer, W_DataTypeDef write_type, uint32_t length){
	uint32_t data_cell; // Guarda o tipo de palavra a ser armazenada.
	switch(write_type){
	// Preenche o buffer com zeros.
	case WRITE_ZEROS:
		data_cell = 0x0000;
		for(uint32_t i = 0; i<(length); i++){
			buffer[i] = data_cell;
		}
		break;

		// Preenche o buffer com 0xFF.
	case WRITE_ONES:
		data_cell = 0xFFFF;
		for(uint32_t i = 0; i<(length); i++){
			buffer[i] = data_cell;
		}
		break;

		// Preenche o buffer com 0x55.
	case WRITE_x55h:
		data_cell = 0x5555;
		for(uint32_t i = 0; i<(length); i++){
			buffer[i] = data_cell;
		}
		break;

		// Preenche o buffer com 0xAA.
	case WRITE_xAAh:
		data_cell = 0xAAAA;
		for(uint32_t i = 0; i<(length); i++){
			buffer[i] = data_cell;
		}
		break;

		// Preenche o buffer com dados aleatórios.
	case WRITE_RANDOM:
		for(uint32_t i = 0; i<(length); i++){
			data_cell = (rand() % 65536); // Escolhe um número aleatório entre 0 e 65535 (16 bits) para colocar no buffer.
			buffer[i] = data_cell;
		}
		break;
	}
}


