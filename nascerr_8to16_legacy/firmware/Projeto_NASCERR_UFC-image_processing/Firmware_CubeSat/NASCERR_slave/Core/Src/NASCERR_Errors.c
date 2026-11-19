/*
 * NASCERR_Errors.c
 *
 *  Created on: Sep 25, 2024
 *      Author: clara
 */

#include "NASCERR_Errors.h"


/*
 * ******************************************************************************
 *  Function Name: nascerr_errors_burst_TBEC
 *  @brief       : Introduces burst errors for TBEC (Triple Bit Error Correction) mode
 *                 by manipulating a random number of bits in each word of data.
 *  @param       : ptr - Pointer to the data where errors will be introduced.
 *                 data_length - The length of the data (in words) to introduce errors into.
 *                 errors - Specifies the type or amount of errors to introduce:
 *                          RANDOM_ERRORS - A random number of errors (0-3) per word.
 *                          NO_ERRORS - Zero errors per word.
							ONE_BIT_ERRORS - 1 error per word.
							TWO_BIT_ERRORS - 2 errors per word.
							THREE_BIT_ERRORS - 3 errors per word.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_errors_burst_TBEC(uint16_t* ptr, uint32_t data_length, ErrorsTypeDef errors){
	int error_position;
	int bin;
	int errors_amount;

	nascerr_memory_select_ecc(BYPASS_TO_SRAM);

	if(errors == RANDOM_ERRORS){
		for(int i = 0; i < data_length; i++){
			errors_amount = rand() % 4; // escolhe um valor aleatório entre 0 e 3
			bin = (0b1<<errors_amount)-1;

			if(errors_amount != 0){
				error_position = rand() % (16 - errors_amount + 1); // escolhe posição aleatória na palavra
				*(ptr + i) = *(ptr + i) ^ (bin << error_position);
//				nascerr_io_delay(1);
			}
		}
	}
	else{
		errors_amount = errors;
		bin = (0b1<<errors_amount)-1;

		for(int i = 0; i < data_length; i++){
			error_position = rand() % (16 - errors_amount + 1); // escolhe posição aleatória na palavra
			*(ptr + i) = *(ptr + i) ^ (bin << error_position);
//			nascerr_io_delay(1);
		}
	}
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_errors_burst_Hamming_EDAC
 *  @brief       : Introduces burst errors for Hamming and EDAC (Error Detection And Correction) modes
 *                 by manipulating a random number of bits in each word of data.
 *  @param       : ptr - Pointer to the data where errors will be introduced.
 *                 data_length - The length of the data (in words) to introduce errors into.
 *                 errors - Specifies the type or amount of errors to introduce:
 *                          RANDOM_ERRORS - A random number of errors (0-3) per word.
 *                          NO_ERRORS - Zero errors per word.
							ONE_BIT_ERRORS - 1 error per word.
							TWO_BIT_ERRORS - 2 errors per word.
							THREE_BIT_ERRORS - 3 errors per word.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_errors_burst_Hamming_EDAC(uint16_t* ptr, uint32_t data_length, ErrorsTypeDef errors){
	int error_position;
	int bin;
	int errors_amount;

	nascerr_memory_select_ecc(BYPASS_TO_SRAM);

	if(errors == RANDOM_ERRORS){
		for(int i = 0; i < data_length; i++){
			errors_amount = rand() % 4; // escolhe um valor aleatório entre 0 e 3
			bin = (0b1<<errors_amount)-1;

			if(errors_amount != 0){
				error_position = rand() % (12 - errors_amount + 1); // escolhe posição aleatória na palavra
				*(ptr + i) = *(ptr + i) ^ (bin << error_position);
//				nascerr_io_delay(1);
			}
		}
	}
	else{
		errors_amount = errors;
		bin = (0b1<<errors_amount)-1;

		for(int i = 0; i < data_length; i++){
			error_position = rand() % (12 - errors_amount + 1); // escolhe posição aleatória na palavra
			*(ptr + i) = *(ptr + i) ^ (bin << error_position);
//			nascerr_io_delay(1);
		}
	}
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_errors_choose_error_generator
 *  @brief       : Chooses the appropriate error generator function based on the selected ECC mode.
 *  @param       : ptr - Pointer to the data where errors will be introduced.
 *                 ecc - Specifies the ECC mode to be used:
 *                       ECC_MOD1 - Hamming
 *                       ECC_MOD2 - TBEC
 *                       ECC_MOD3 - EDAC
 *                 data_length - The length of the data (in words) to introduce errors into.
 *                 errors - Specifies the type or amount of errors to introduce.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_errors_choose_error_generator(uint16_t* ptr, NASCERR_MODE ecc, uint32_t data_length, ErrorsTypeDef errors){
	switch(ecc){
	// 10
	case ECC_MOD1: // Hamming
		nascerr_errors_burst_Hamming_EDAC(ptr, data_length, errors);
		break;

		// 01
	case ECC_MOD2: // TBEC
		nascerr_errors_burst_TBEC(ptr, data_length, errors);
		break;

		// 11
	case ECC_MOD3: // EDAC
		nascerr_errors_burst_Hamming_EDAC(ptr, data_length, errors);
		break;

	default:
		break;

	}
}
