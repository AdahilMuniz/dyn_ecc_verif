/*
 * NASCERR_Experiment.c
 *
 *  Created on: Aug 28, 2024
 *      Author: Clara
 */

#include "NASCERR_Experiment.h"

#include "NASCERR_Registers.h"

#define ERRORS TWO_BIT_ERRORS


uint32_t write_buffer[COMM_BUFFER_SIZE];
uint32_t read_ecc[COMM_BUFFER_SIZE];
uint32_t encoded_buffer[COMM_BUFFER_SIZE];
uint32_t read_bypass[COMM_BUFFER_SIZE];
ResDescTypeDef response_result[COMM_BUFFER_SIZE];
NASCERR_RESPONSE nascerr_response;
uint32_t errors_mem_test[2] = {0, 0};
uint16_t size;


/*
 * ******************************************************************************
 *  Function Name: nascerr_experiment_test
 *  @brief       : Executes an error correction experiment with ECC encoding and decoding,
 *                 processing data in multiple repetitions and calculating error statistics.
 *  @param command : A NASCERR_CTRL structure containing the experiment parameters:
 *                   - data_lenght: The length of the data to be processed.
 *                   - repeat: The number of repetitions for the experiment.
 *                   - mode: The ECC mode to use for encoding and decoding.
 *                   - delay: The delay between repetitions of the experiment.
 *                   - data: Specifies whether data should be written block-by-block or word-by-word.
 *  @retval      : A NASCERR_RESPONSE structure containing the results of the experiment:
 *                 - id: The command ID.
 *                 - data_lenght: The length of the processed data.
 *                 - number_errors: The number of errors detected.
 *                 - corrected_data: The mean number of corrected errors across all repetitions.
 *                 - result: An array of results from each repetition.
 *                 - percent_error: The percentage of errors relative to the data length.
 *  @note        : The function uses a state machine to handle different stages of the experiment:
 *                 - STATE_INIT: Validates the input parameters.
 *                 - STATE_WRITE_READ: Performs write and read operations on the data.
 *                 - STATE_PROCESS: Encodes and decodes data, checking for errors.
 *                 - STATE_STORE_RESULT: Stores the results for each repetition.
 *                 - STATE_CALCULATE_MEAN: Computes the mean errors for all repetitions.
 *                 - STATE_FILL_RESPONSE: Fills the response structure with the final results.
 *                 - STATE_ERROR: Handles errors and returns an error response.
 * ******************************************************************************
 */
NASCERR_RESPONSE nascerr_experiment_test(NASCERR_CTRL command) {
	nascerr_experiment_state_t state = STATE_INIT;
	uint32_t errors_corrected[command.repeat], errors_total[command.repeat];
	uint32_t mean_errors_corrected, mean_errors_total;
	uint8_t i;

	nascerr_memory_start_saboteur(); // prepara o sabotador da FPGA


	while (state != STATE_DONE) {
		switch (state) {
		case STATE_INIT:
			// Verificação de parâmetros

			DEBUG_PRINT(LOG_INFO, "Verificacao de parametros iniciada... \r\n");

			if (command.data_lenght <= 0 || command.data_lenght > COMM_BUFFER_SIZE) {
				state = STATE_ERROR;
				DEBUG_PRINT(LOG_WARN, "Experimento interrompido! Dado de tamanho invalido. \r\n");
				break;
			}

			if(command.data_lenght + command.offset > NASCERR_SRAM_END - NASCERR_SRAM_ADDRESS){
				state = STATE_ERROR;
				break;
				DEBUG_PRINT(LOG_WARN, "Experimento interrompido! Estouro de regiao de memoria. \r\n");
			}

			if(command.repeat < 1){
				state = STATE_ERROR;
				DEBUG_PRINT(LOG_WARN, "Experimento interrompido! Numero de repeticoes invalido. \r\n");
				break;
			}

			if(command.delay < 0){
				state = STATE_ERROR;
				DEBUG_PRINT(LOG_WARN, "Experimento interrompido! Valor de delay invalido. \r\n");
				break;
			}

			// Inicialize variáveis, buffers e contadores
			i = 0;
			mean_errors_total = 0;
			mean_errors_corrected = 0;
			state = STATE_WRITE_READ;
			DEBUG_PRINT(LOG_INFO, "Verificacao de parametros concluida. \r\n");
			// TESTE MEMORIA
//			nascerr_memory_write_all_memory_test();
//			uint32_t errors_reg = nascerr_memory_region_test(MEMORY_1, 0x1000, 1024);
//			nascerr_memory_march_test(MEMORY_1);
//			uint32_t errors_ret = nascerr_memory_retention_test(BOTH, 1000);
			break;



		case STATE_WRITE_READ:
			// Realiza escrita e leitura de dados
			if (command.data == BOCK_BY_BOCK) {
				nascerr_experiment_block_by_block_data(command);
			}
			else if (command.data == WORD_BY_WORD) {
				nascerr_experiment_word_by_word_data(command);
			}
			state = STATE_PROCESS;
			break;

		case STATE_PROCESS:
			// Processa os dados lidos
			if (command.mode != BYPASS_1 && command.mode != BYPASS_2 && command.mode != BYPASS_1_TO_7) {
				nascerr_eccs_encoded_buffer(write_buffer, encoded_buffer, command.mode, command.data_lenght);
				errors_total[i] = nascerr_experiment_buffer_result(command.data_lenght, command.offset, encoded_buffer, read_bypass);
				errors_corrected[i] = nascerr_experiment_error_counter(command.data_lenght, write_buffer, read_ecc);
			}
			else {
				errors_total[i] = nascerr_experiment_buffer_result(command.data_lenght, command.offset, write_buffer, read_bypass);
				errors_corrected[i] = nascerr_experiment_error_counter(command.data_lenght, write_buffer, read_ecc);
			}
			state = STATE_STORE_RESULT;
			break;

		case STATE_STORE_RESULT:
			// Armazena os resultados da iteração atual
			mean_errors_total += errors_total[i];
			mean_errors_corrected += errors_corrected[i];
			i++;
			if (i >= command.repeat) {
				state = STATE_CALCULATE_MEAN;
			} else {
				state = STATE_WRITE_READ;
			}
			break;

		case STATE_CALCULATE_MEAN:
			// Calcula as médias
			mean_errors_total = mean_errors_total / command.repeat;
			mean_errors_corrected = mean_errors_corrected / command.repeat;
			state = STATE_FILL_RESPONSE;
			break;

		case STATE_FILL_RESPONSE:
			// Preenche a estrutura de resposta e retorna
			nascerr_response.id = command.id;
			nascerr_response.data_lenght = command.data_lenght;
			nascerr_response.number_errors = errors_total[command.repeat - 1];
			nascerr_response.corrected_data = mean_errors_corrected;
			nascerr_response.result = response_result; // Certifique-se de inicializar essa variável
			nascerr_response.percent_error = ((float)mean_errors_total / command.data_lenght) * 100.0;
			state = STATE_DONE;
			break;

		case STATE_ERROR:
			// Tratamento de erro - preencher a resposta com a mensagem de erro
			nascerr_response.id = command.id;
			nascerr_response.data_lenght = 0;
			nascerr_response.number_errors = 0;
			nascerr_response.corrected_data = 0;
			nascerr_response.result = 0; // Define um valor específico para sinalizar erro
			nascerr_response.percent_error = 0;

			// Adiciona a mensagem de erro à estrutura de resposta (se aplicável)
			state = STATE_DONE;
			break;

		default:
			break;
		}
	}

	return nascerr_response;
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_experiment_error_counter
 *  @brief       : Counts the total number of errors by comparing two data buffers.
 *                 Verify how many words were read as expected from memory.
 *  @param length : The number of data words to compare.
 *  @param data_write : Pointer to the buffer containing the written data.
 *  @param data_read  : Pointer to the buffer containing the read data.
 *  @retval      : The total count of matching data words (no error detected).
 * ******************************************************************************
 */
uint32_t nascerr_experiment_error_counter(uint32_t length, uint32_t* data_write, uint32_t* data_read) {
	uint32_t error = 0;
	uint32_t error_total = 0;
	for (uint32_t i = 0; i < length; i++) {

		// Perform XOR operation between data_write and data_read and store the result in error
		error = data_write[i] ^ data_read[i];
		if (error == 0)
			error_total++;
	}
	return error_total;
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_experiment_buffer_result
 *  @brief       : Compares two data buffers and stores the results of detected
 *                 errors, including their addresses, in a response buffer.
 *                 Verify how many words in memory were as expected.
 *  @param length : The number of data words to compare.
 *  @param offset_addr : The starting offset address for the comparison.
 *  @param data_write : Pointer to the buffer containing the written data.
 *  @param data_read  : Pointer to the buffer containing the read data.
 *  @retval      : The total count of data words with detected errors.
 * ******************************************************************************
 */
uint32_t nascerr_experiment_buffer_result(uint32_t length, uint32_t offset_addr, uint32_t* data_write, uint32_t* data_read){
	uint32_t error = 0;
	uint32_t error_counter = 0;
	uint32_t result_idx = 0;
	for (uint32_t i = 0; i < length; i++) {

		// Perform XOR operation between data_write and data_read and store the result in error
		error = data_write[i] ^ data_read[i];
		if (error != 0){
			error_counter++;
			response_result[result_idx].result = error;
			response_result[result_idx].addr = (nascerr_addr)NASCERR_SRAM_ADDRESS + offset_addr + 2*i;
			result_idx++;
		}

	}
	return error_counter;
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_experiment_word_by_word_data
 *  @brief       : Executes word-by-word write and read operations in SRAM based on
 *                 the specified command parameters, supporting ECC or bypass modes.
 *  @param command : A NASCERR_CTRL structure containing the operation parameters:
 *                   - mode: Specifies the ECC or bypass mode for the operation.
 *                   - data_lenght: Number of words to be processed.
 *                   - write_type: Specifies the data pattern to be written.
 *                   - delay: Delay between write and read operations.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_experiment_word_by_word_data(NASCERR_CTRL command){
	// realiza as escritas e leituras a cada palavra
	if (command.mode == BYPASS_1 || command.mode == BYPASS_2 || command.mode == BYPASS_1_TO_7){
		for(uint32_t i = 0; i < command.data_lenght; i++){
			write_buffer[i] = nascerr_memory_write_sram_word(command.mode, command.write_type, command.offset + i);
			nascerr_io_delay(command.delay);
			read_ecc[i] = nascerr_memory_read_sram_word_bypass(command.offset + i, command.mode); // lê pelo ecc ou bypass
			read_bypass[i] = nascerr_memory_read_sram_word_bypass(command.offset + i, command.mode);
		}
	}
	else{
		for(uint32_t i = 0; i < command.data_lenght; i++){
			write_buffer[i] = nascerr_memory_write_sram_word(command.mode, command.write_type, command.offset + i);
			nascerr_io_delay(command.delay);
			read_ecc[i] = nascerr_memory_read_sram_word(command.offset + i, command.mode); // lê pelo ecc ou bypass
			read_bypass[i] = nascerr_memory_read_sram_word(command.offset + i, BYPASS_1);
		}
	}
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_experiment_block_by_block_data
 *  @brief       : Executes block-based write and read operations in SRAM based on
 *                 the specified command parameters, supporting ECC or bypass modes.
 *  @param command : A NASCERR_CTRL structure containing the operation parameters:
 *                   - mode: Specifies the ECC or bypass mode for the operation.
 *                   - data_lenght: Number of words in the block to be processed.
 *                   - write_type: Specifies the data pattern to be written to the block.
 *                   - delay: Delay between write and read operations.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_experiment_block_by_block_data(NASCERR_CTRL command){
	// preenche o bloco
	nascerr_memory_write_sram_block(write_buffer, command.offset, command.mode, command.write_type, command.data_lenght);
	nascerr_io_delay(command.delay);

	// lê o bloco
	if (command.mode == BYPASS_1 || command.mode == BYPASS_2 || command.mode == BYPASS_1_TO_7){
		nascerr_memory_read_sram_block_bypass(read_ecc, command.offset, command.mode, command.data_lenght);
		nascerr_memory_read_sram_block_bypass(read_bypass, command.offset, command.mode, command.data_lenght);
	}
	else{
		nascerr_memory_read_sram_block(read_ecc, command.offset, command.mode, command.data_lenght);
		nascerr_memory_read_sram_block(read_bypass, command.offset, BYPASS_1, command.data_lenght);
	}

}





