/*
 * experimento.c
 *
 *  Created on: Aug 28, 2024
 *      Author: Clara
 */

#include <NASCERR_Experiment.h>
#include <NASCERR_Memory.h>
#include <stdlib.h>

uint16_t write_buffer[1024];
uint16_t read_ecc[1024];
uint16_t read_bypass[1024];
//ResDescTypeDef errors[1024];

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
void nascerr_experiment_select_ecc(NASCERR_MODE ecc){
	//	ptr = (uint16_t *)NASCERR_SRAM_ADDRESS;

	switch(ecc){
	// 10
	case ECC_MOD1: // Hamming
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_0,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_1,GPIO_PIN_RESET);
		break;

		// 01
	case ECC_MOD2: // TBEC
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_0,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_1,GPIO_PIN_SET);
		break;

		// 11
	case ECC_MOD3: // EDAC
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_0,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_1,GPIO_PIN_SET);
		break;

		// 00
	case BYPASS_TO_SRAM:
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_0,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_1,GPIO_PIN_RESET);
		break;

	}
}


NASCERR_RESPONSE nascerr_experiment_test(NASCERR_CTRL command){
	uint32_t errors_corrected, errors_total;
//	NASCERR_RESPONSE nascerr_response [command.repeat];
	NASCERR_RESPONSE nascerr_response;
	for(int i = 0; i < command.repeat; i++){
		ResDescTypeDef errors[1024];

		nascerr_memory_write_sram((uint16_t*)write_buffer, 0, command.mode, command.write_type, command.data_lenght);
		nascerr_io_delay(command.delay);

		nascerr_memory_read_sram(read_ecc, 0, command.mode, command.data_lenght);
		nascerr_memory_read_sram(read_bypass, 0, BYPASS_TO_SRAM, command.data_lenght);

		errors_total = nascerr_experiment_buffer_result(command.data_lenght, write_buffer, read_bypass, errors);
		errors_corrected = nascerr_experiment_error_counter(command.data_lenght, read_ecc, read_bypass);

		//Depois implementar a repetição de mensagens
		//		nascerr_response[i].id = command.id;
		//		nascerr_response[i].data_lenght =  command.data_lenght;
		//		nascerr_response[i].number_errors = errors_total;
		//		nascerr_response[i].corrected_data = errors_corrected;
		//		nascerr_response[i].result = errors;

		nascerr_response.id = command.id;
		nascerr_response.data_lenght = command.data_lenght;
		nascerr_response.number_errors = errors_total;
		nascerr_response.corrected_data = errors_corrected;
		nascerr_response.result = errors;
	}
	return nascerr_response;
}

//XOR comparison between two buffers, counting the total amount of errors
uint32_t nascerr_experiment_error_counter(uint32_t length, uint16_t* data_write, uint16_t* data_read) {
	uint16_t error = 0;
	uint32_t error_total = 0;
	for (uint32_t i = 0; i < length; ++i) {

		// Perform XOR operation between data_write and data_read and store the result in error
		error = data_write[i] ^ data_read[i];
		if (error != 0)
			for (uint16_t mask = 1; mask != 0; mask <<= 1) {
				if (error & mask)
					error_total++;
			}
	}
	return error_total;
}

//XOR comparison between two buffers, storing the result in case of error inside a struct array
uint32_t nascerr_experiment_buffer_result(uint32_t length, uint16_t* data_write, uint16_t* data_read, ResDescTypeDef error_buffer[length]) {
	uint16_t error = 0;
	uint32_t error_counter = 0, error_total = 0;
	for (uint32_t i = 0; i < length; ++i) {

		// Perform XOR operation between data_write and data_read and store the result in error
		error = data_write[i] ^ data_read[i];
		if (error != 0){
			error_buffer[error_counter].result = error;
			error_buffer[error_counter].addr = NASCERR_SRAM_ADDRESS + 2*i;
			error_counter++;
			for (uint16_t mask = 1; mask != 0; mask <<= 1) {
				if (error & mask)
					error_total++;
			}
		}
	}
	return error_total;
}
