/*
 * NASCERR_Memory.c
 *
 *  Created on: Sep 3, 2024
 *      Author: Clara
 */

#include "NASCERR_Memory.h"


volatile uint16_t * ptr = (uint16_t *)NASCERR_SRAM_ADDRESS;


void nascerr_memory_start_saboteur(){
	nascerr_io_pin_set(GPIO_ECC, GPIO_PIN_3);
	nascerr_io_pin_reset(GPIO_ECC, GPIO_PIN_3);
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_select_ecc
 *  @brief       : Configures the Error Correcting Code (ECC) selection using GPIO pins.
 *  @param       : ecc - An 8-bit unsigned integer specifying the ECC type to be selected.
 *                        Possible values:
 *                        01 - HAMMING
 *                        02 - TBEC
 *                        03 - EDAC
 *                        04 - BYPASS (no ECC)
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_select_ecc(NASCERR_MODE ecc){

	switch(ecc){
	// 10
	case ECC_MOD1: // TBEC
		nascerr_io_pin_set(GPIO_ECC, GPIO_PIN_0);
		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_1);
		break;

		// 01
	case ECC_MOD2: // TBEC_RANDOM
		nascerr_io_pin_reset(GPIOC,GPIO_PIN_0);
		nascerr_io_pin_set(GPIOC, GPIO_PIN_1);
		break;

		// 11
	case ECC_MOD3: // EDAC
		nascerr_io_pin_set(GPIO_ECC, GPIO_PIN_0);
		nascerr_io_pin_set(GPIO_ECC, GPIO_PIN_1);
		break;

		// 00
	case BYPASS_TO_SRAM:
		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_0);
		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_1);
		break;

	default:
		break;

	}
}


//void nascerr_memory_select_ecc(NASCERR_MODE ecc){
//
//	switch(ecc){
//		// 0000
//	case BYPASS_TO_SRAM:
//		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_0);
//		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_1);
//		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_2);
//		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_3);
//		break;
//		// 0001
//	case ECC_MOD1: // hamming
//		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_0);
//		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_1);
//		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_2);
//		nascerr_io_pin_set(GPIO_ECC,GPIO_PIN_3);
//		break;
//		// 0010
//	case ECC_MOD2: // TBEC
//		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_0);
//		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_1);
//		nascerr_io_pin_set(GPIO_ECC,GPIO_PIN_2);
//		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_3);
//		break;
//		// 0011
//	case ECC_MOD3: // EDAC
//		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_0);
//		nascerr_io_pin_reset(GPIO_ECC,GPIO_PIN_1);
//		nascerr_io_pin_set(GPIO_ECC,GPIO_PIN_2);
//		nascerr_io_pin_set(GPIO_ECC,GPIO_PIN_3);
//		break;
//	default:
//		break;
//
//	}
//}


void nascerr_memory_write_word_cell(uint16_t data, NASCERR_MODE ecc, uint32_t offset_addr){
	nascerr_memory_select_ecc(ecc);
	*(ptr + offset_addr) = data;
}


uint16_t nascerr_memory_write_sram_word(NASCERR_MODE ecc, W_DataTypeDef write_type, uint32_t offset_addr){
	uint16_t data;
//	nascerr_memory_select_ecc(ecc);
	switch(write_type){
	case WRITE_ZEROS:
		data = 0x00;
		break;
	case WRITE_ONES:
		data = 0xFF;
		break;
	case WRITE_x55h:
		data = 0x55;
		break;
	case WRITE_xAAh:
		data = 0xAA;
		break;
	case WRITE_RANDOM:
		data = (rand() % 256); // Escolhe um número aleatório entre 0 e 255 (8 bits) para escrever na memória.
		break;
	}

	nascerr_memory_write_word_cell(data, ecc, offset_addr); // escreve o dado na posição de memória

	return data; // retorna para preencher o buffer de escrita na função de teste
}


void nascerr_memory_write_block_cell(uint16_t* write_buffer, NASCERR_MODE ecc, uint32_t offset_addr, uint32_t length){
	nascerr_memory_select_ecc(ecc); // seleciona o ECC.

	for(int i = 0; i < length; i++){
		*(ptr + offset_addr + i) = write_buffer[i];
	}
}


void nascerr_memory_write_sram_block(uint16_t* write_buffer, uint32_t offset_addr, NASCERR_MODE ecc, W_DataTypeDef write_type, uint32_t length){
	nascerr_memory_fill_buffer(write_buffer, write_type, length); // Preenche o buffer com o tipo de dado indicado no comando.
//	nascerr_memory_select_ecc(ecc); // seleciona o ECC.
//
//	for(int i = 0; i < length; i++){
//		*(ptr + offset_addr + i) = write_buffer[i];
//	}

	nascerr_memory_write_block_cell(write_buffer, ecc, offset_addr, length); // escreve o bloco de memoria

}


uint16_t nascerr_memory_read_sram_word(uint32_t offset_addr, NASCERR_MODE ecc){
	nascerr_memory_select_ecc(ecc);
	return *(ptr + offset_addr);
}


void nascerr_memory_read_sram_block(uint16_t* read_buffer, uint32_t offset_addr, NASCERR_MODE ecc, uint32_t length){
	nascerr_memory_select_ecc(ecc);
	for(int i = 0; i < length; i++){
		read_buffer[i] = *(ptr + offset_addr + i);
	}
}


void nascerr_memory_fill_buffer(uint16_t* buffer, W_DataTypeDef write_type, uint32_t length){
	uint16_t data_cell; // Guarda o tipo de palavra a ser armazenada.
	switch(write_type){
	// Preenche o buffer com zeros.
	case WRITE_ZEROS:
		data_cell = 0x00;
		for(uint32_t i = 0; i<(length); i++){
			buffer[i] = data_cell;
		}
		break;

		// Preenche o buffer com 0xFF.
	case WRITE_ONES:
		data_cell = 0xFF;
		for(uint32_t i = 0; i<(length); i++){
			buffer[i] = data_cell;
		}
		break;

		// Preenche o buffer com 0x55.
	case WRITE_x55h:
		data_cell = 0x55;
		for(uint32_t i = 0; i<(length); i++){
			buffer[i] = data_cell;
		}
		break;

		// Preenche o buffer com 0xAA.
	case WRITE_xAAh:
		data_cell = 0xAA;
		for(uint32_t i = 0; i<(length); i++){
			buffer[i] = data_cell;
		}
		break;

		// Preenche o buffer com dados aleatórios.
	case WRITE_RANDOM:
		for(uint32_t i = 0; i<(length); i++){
			data_cell = (rand() % 256); // Escolhe um número aleatório entre 0 e 255 (8 bits) para colocar no buffer.
			buffer[i] = data_cell;
		}
		break;
	}
}
