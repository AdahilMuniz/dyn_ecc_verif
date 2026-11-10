/**
 * ******************************************************************************
 *  @file    : experiment.h
 *  @brief   : Header for nascerr_experiment.c file.
 *  @author  : Cicero Oliveira, Joao Manoel
 * ******************************************************************************
 */

#ifndef INC_NASCERR_EXPERIMENT_H_
#define INC_NASCERR_EXPERIMENT_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "main.h"
#include "NASCERR_Control.h"
#include "NASCERR_IOs.h"
#include "NASCERR_Protocol.h"
#include "NASCERR_ECCs_32bits.h"
#include "NASCERR_Memory_32bits.h"
#include "NASCERR_Util.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

#define NUM_BLOCKS 64
#define NUM_PAGES_PER_BLOCK 32
#define NUM_WORDS_PER_PAGE ((NASCERR_SRAM_END - NASCERR_SRAM_ADDRESS)/(NUM_BLOCKS*NUM_PAGES_PER_BLOCK))
//#define BLOCK_SIZE_WORDS (NASCERR_SRAM_END - NASCERR_SRAM_ADDRESS) / NUM_BLOCKS //32k WORDS

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */


uint8_t nascerr_experiment_init(NASCERR_SCHEDULER command);
ECC_Telemetry_t nascerr_experiment_read_result(NASCERR_SCHEDULER command);
nascerr_data nascerr_experiment_number_errors(ECC_Telemetry_t buffer_read);
nascerr_data nascerr_experiment_number_corrected(ECC_Telemetry_t buffer_read);
float nascerr_experiment_synthesise_error_info(nascerr_data number_of_errors, nascerr_data number_of_corrected);
void nascerr_count_region(ECC_Telemetry_t *telemetry, uint16_t diff);

//void run_tests(void); //Teste da escrita e leitura

#endif /* INC_NASCERR_EXPERIMENT_H_ */
