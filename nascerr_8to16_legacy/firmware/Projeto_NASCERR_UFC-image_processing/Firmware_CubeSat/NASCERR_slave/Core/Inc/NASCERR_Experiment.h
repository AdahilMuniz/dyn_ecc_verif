/*
 * experimento.h
 *
 *  Created on: Aug 28, 2024
 *      Author: Clara
 */

#ifndef INC_NASCERR_EXPERIMENT_H_
#define INC_NASCERR_EXPERIMENT_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "stm32l4xx_hal.h"
#include "NASCERR_Control.h"
#include "NASCERR_IOs.h"
#include "NASCERR_Memory.h"
#include "NASCERR_Protocol.h"
#include "NASCERR_ECCs.h"
#include "NASCERR_Errors.h"

typedef enum {
    STATE_INIT,
    STATE_WRITE_READ,
    STATE_PROCESS,
    STATE_STORE_RESULT,
    STATE_CALCULATE_MEAN,
	STATE_FILL_RESPONSE,
	STATE_ERROR,
    STATE_DONE,
} nascerr_experiment_state_t;


// Funções:
NASCERR_RESPONSE nascerr_experiment_test(NASCERR_CTRL command);
void nascerr_experiment_testrw(uint8_t* read8b, uint16_t* read16b, uint32_t* read32b);
uint32_t nascerr_experiment_error_counter(uint32_t length, uint16_t* data_write, uint16_t* data_read);
uint32_t nascerr_experiment_buffer_result(uint32_t length, uint32_t offset_addr, uint16_t* data_write, uint16_t* data_read);
uint32_t nascerr_experiment_error_counter_matrix(int width, int height, uint8_t data_write[height][width], uint8_t data_read[height][width]);
uint32_t nascerr_experiment_buffer_result_matrix(int width, int height, uint8_t data_write[height][width], uint8_t data_read[height][width], ResDescTypeDef comp_result[width*height]);
void nascerr_experiment_word_by_word_data(NASCERR_CTRL command);
void nascerr_experiment_block_by_block_data(NASCERR_CTRL command);

#endif /* INC_NASCERR_EXPERIMENT_H_ */
