/*
 * experimento.c
 *
 *  Created on: Aug 28, 2024
 *      Author: Clara
 */

#include "NASCERR_Experiment.h"

#define ERRORS TWO_BIT_ERRORS
#define OFFSET 0


uint16_t write_buffer[COMM_BUFFER_SIZE];
uint16_t read_ecc[COMM_BUFFER_SIZE];
uint16_t encoded_buffer[COMM_BUFFER_SIZE];
uint16_t read_bypass[COMM_BUFFER_SIZE];
ResDescTypeDef response_result[COMM_BUFFER_SIZE];
NASCERR_RESPONSE nascerr_response;

uint16_t size;


//NASCERR_RESPONSE nascerr_experiment_test(NASCERR_CTRL command){
//	uint32_t errors_corrected[command.repeat], errors_total[command.repeat];
//	uint32_t mean_errors_corrected = 0, mean_errors_total = 0;
//
//	for(int i = 0; i < command.repeat; i++){
//		// REALIZA ESCRITAS E LEITURAS
//		if (command.data == BOCK_BY_BOCK){
//			nascerr_experiment_block_by_block_data(command);
//		}
//		else if (command.data == WORD_BY_WORD){
//			nascerr_experiment_word_by_word_data(command);
//		}
//
//		// REALIZA O PROCESSAMENTO DOS DADOS LIDOS
//		if(command.mode != BYPASS_TO_SRAM){
//			nascerr_eccs_encoded_buffer(write_buffer, encoded_buffer, command.mode, command.data_lenght);
//
//			errors_total[i] = nascerr_experiment_buffer_result(command.data_lenght, encoded_buffer, read_bypass);
//			errors_corrected[i] = nascerr_experiment_error_counter(command.data_lenght, write_buffer, read_ecc);
//		}
//		else{
//			errors_total[i] = nascerr_experiment_buffer_result(command.data_lenght, write_buffer, read_bypass);
//			errors_corrected[i] = errors_total[i];
//		}
//
//		// GUARDA A SOMA DOS RESULTADOS
//		mean_errors_total += errors_total[i];
//		mean_errors_corrected += errors_corrected[i];
//	}
//
//	// CALCULA AS MÉDIAS
//	mean_errors_total = mean_errors_total / command.repeat;
//	mean_errors_corrected = mean_errors_corrected / command.repeat;
//
//
//	// PREENCHE A ESTRUTURA DE RESPOSTA
//	nascerr_response.id = command.id;
//	nascerr_response.data_lenght = command.data_lenght;
//	nascerr_response.number_errors = mean_errors_total;
//	nascerr_response.corrected_data = mean_errors_corrected;
//	nascerr_response.result = response_result;
//
//	return nascerr_response;
//}


NASCERR_RESPONSE nascerr_experiment_test(NASCERR_CTRL command) {
	nascerr_experiment_state_t state = STATE_INIT;
	uint32_t errors_corrected[command.repeat], errors_total[command.repeat];
	uint32_t mean_errors_corrected, mean_errors_total;
	int i;
	char* error_message = NULL; // mensagem de erro

	nascerr_memory_start_saboteur(); // prepara o sabotador da FPGA

	while (state != STATE_DONE) {
		switch (state) {
		case STATE_INIT:
			// Verificação de parâmetros
			if (command.data_lenght <= 0 || command.data_lenght > 1024) {
				state = STATE_ERROR;
				error_message = "Tamanho da mensagem invalido";
				break;
			}

			if(command.data_lenght + OFFSET > 1024){
				state = STATE_ERROR;
				error_message = "Estouro de memoria";
				break;
			}

			if(command.repeat < 1){
				state = STATE_ERROR;
				error_message = "Numero de repeticoes deve ser >= 1";
				break;
			}

			if(command.delay < 0){
				state = STATE_ERROR;
				error_message = "Delay < 0";
				break;
			}

			// Inicialize variáveis, buffers e contadores
			i = 0;
			mean_errors_total = 0;
			mean_errors_corrected = 0;
			state = STATE_WRITE_READ;
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
			if (command.mode != BYPASS_TO_SRAM) {
				nascerr_eccs_encoded_buffer(write_buffer, encoded_buffer, command.mode, command.data_lenght);
				errors_total[i] = nascerr_experiment_buffer_result(command.data_lenght, OFFSET, encoded_buffer, read_bypass);
				errors_corrected[i] = nascerr_experiment_error_counter(command.data_lenght, write_buffer, read_ecc);
			}
			else {
				errors_total[i] = nascerr_experiment_buffer_result(command.data_lenght, OFFSET, write_buffer, read_bypass);
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
			nascerr_response.number_errors = mean_errors_total;
			nascerr_response.corrected_data = mean_errors_corrected;
			if (nascerr_response.data_lenght>0)
			{
				nascerr_response.percent_error = (nascerr_response.number_errors*100)/nascerr_response.data_lenght;
			} else{
				nascerr_response.percent_error = 0;
			}
			nascerr_response.result = response_result; // Certifique-se de inicializar essa variável
			state = STATE_DONE;
			break;

		case STATE_ERROR:
			// Tratamento de erro - preencher a resposta com a mensagem de erro
			nascerr_response.id = command.id;
			nascerr_response.data_lenght = 0;
			nascerr_response.number_errors = 0;
			nascerr_response.corrected_data = 0;
			nascerr_response.percent_error = 0;
			nascerr_response.result = 0; // Define um valor específico para sinalizar erro

			// Adiciona a mensagem de erro à estrutura de resposta (se aplicável)
			//			nascerr_response.error_message = error_message; // Assumindo que a struct tenha um campo para mensagem de erro
			state = STATE_DONE;
			break;

		default:
			break;
		}
	}

	return nascerr_response;
}


uint32_t nascerr_experiment_error_counter(uint32_t length, uint16_t* data_write, uint16_t* data_read) {
	uint16_t error = 0;
	uint32_t error_total = 0;
	for (uint32_t i = 0; i < length; i++) {

		// Perform XOR operation between data_write and data_read and store the result in error
		error = data_write[i] ^ data_read[i];
		if (error == 0)
			error_total++;
	}
	return error_total;
}



uint32_t nascerr_experiment_buffer_result(uint32_t length, uint32_t offset_addr, uint16_t* data_write, uint16_t* data_read){
	uint16_t error = 0;
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



uint32_t nascerr_experiment_error_counter_matrix(int width, int height, uint8_t data_write[height][width], uint8_t data_read[height][width]) {
	uint8_t error = 0;
	uint32_t error_total = 0;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++){
			// Perform XOR operation between data_write and data_read and store the result in error
			error = data_write[i][j] ^ data_read[i][j];
			if (error == 0)
				error_total++;
		}
	}
	return error_total;
}


uint32_t nascerr_experiment_buffer_result_matrix(int width, int height, uint8_t data_write[height][width], uint8_t data_read[height][width], ResDescTypeDef comp_result[width*height]){
	uint16_t error = 0;
	uint32_t error_counter = 0;
	uint32_t result_idx = 0;
	for (int i = 0; i < height; i++) {
		for(int j = 0; j < width; j++){
			// Perform XOR operation between data_write and data_read and store the result in error
			error = data_write[i][j] ^ data_read[i][j];
			if (error != 0){
				error_counter++;
				comp_result[result_idx].result = error;
				comp_result[result_idx].addr = (nascerr_addr)NASCERR_SRAM_ADDRESS + 2*i + 2*j;
				result_idx++;
			}
		}
	}
	return error_counter;
}


void nascerr_experiment_word_by_word_data(NASCERR_CTRL command){
	// realiza as escritas e leituras a cada palavra
	for(int i = 0; i < command.data_lenght; i++){
		write_buffer[i] = nascerr_memory_write_sram_word(command.mode, command.write_type, OFFSET + i);
		nascerr_io_delay(command.delay);
//		nascerr_errors_choose_error_generator((uint16_t *)NASCERR_SRAM_ADDRESS + OFFSET + i, command.mode, 1, ERRORS);
//		nascerr_io_delay(1);
		read_ecc[i] = nascerr_memory_read_sram_word(OFFSET + i, command.mode); // lê pelo ecc ou bypass
		read_bypass[i] = nascerr_memory_read_sram_word(OFFSET + i, BYPASS_TO_SRAM);
	}
}

void nascerr_experiment_block_by_block_data(NASCERR_CTRL command){
	// preenche o bloco
	nascerr_memory_write_sram_block(write_buffer, OFFSET, command.mode, command.write_type, command.data_lenght);
	nascerr_io_delay(command.delay);

	// aplica o erro
//	nascerr_errors_choose_error_generator((uint16_t *)NASCERR_SRAM_ADDRESS + OFFSET, command.mode, command.data_lenght, ERRORS);
//	nascerr_io_delay(1);

	// lê o bloco
	nascerr_memory_read_sram_block(read_ecc, OFFSET, command.mode, command.data_lenght);
	nascerr_memory_read_sram_block(read_bypass, OFFSET, BYPASS_TO_SRAM, command.data_lenght);
}


