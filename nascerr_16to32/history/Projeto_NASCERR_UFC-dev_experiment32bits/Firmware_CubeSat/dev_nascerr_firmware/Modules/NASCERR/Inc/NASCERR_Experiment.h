/**
 * ******************************************************************************
 *  @file    : experiment.h
 *  @brief   : Header file for the NASCERR experiment functions.
 *             Contains function declarations for testing, error counting, and
 *             processing the results in memory experiments with ECC.
 *  @author  : Clara
 * ******************************************************************************
 */

#ifndef INC_NASCERR_EXPERIMENT_H_
#define INC_NASCERR_EXPERIMENT_H_


/*
 * ******************************************************************************
 *  Includes
 * ******************************************************************************
 */
#include <string.h>
#include "stm32l4xx_hal.h"
#include "NASCERR_Control.h"
#include "NASCERR_IOs.h"
#include "NASCERR_Protocol.h"
#include "NASCERR_ECCs_32bits.h"
#include "NASCERR_Memory_32bits.h"
#include "NASCERR_Util.h"


/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */
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


/*
 * ******************************************************************************
 *  Exported functions
 * ******************************************************************************
 */
NASCERR_RESPONSE nascerr_experiment_test(NASCERR_CTRL command);
void nascerr_experiment_testrw(uint8_t* read8b, uint16_t* read16b, uint32_t* read32b);
uint32_t nascerr_experiment_error_counter(uint32_t length, uint32_t* data_write, uint32_t* data_read);
uint32_t nascerr_experiment_buffer_result(uint32_t length, uint32_t offset_addr, uint32_t* data_write, uint32_t* data_read);
void nascerr_experiment_word_by_word_data(NASCERR_CTRL command);
void nascerr_experiment_block_by_block_data(NASCERR_CTRL command);
#endif /* INC_NASCERR_EXPERIMENT_H_ */
