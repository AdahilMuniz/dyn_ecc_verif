/*
 * NASCERR_Errors.h
 *
 *  Created on: Sep 25, 2024
 *      Author: clara
 */

#ifndef INC_NASCERR_ERRORS_H_
#define INC_NASCERR_ERRORS_H_

#include "NASCERR_Memory.h"


/*
 * ******************************************************************************
 *  @typedef    : ErrorsTypeDef
 *  @brief      : Enumeration to define the types of errors that can be introduced into data.
 *                Values:
 *                - NO_ERRORS      : No errors will be introduced.
 *                - ONE_BIT_ERRORS : One bit will be corrupted.
 *                - TWO_BIT_ERRORS : Two bits will be corrupted.
 *                - THREE_BIT_ERRORS: Three bits will be corrupted.
 *                - RANDOM_ERRORS  : A random number of bits (0 to 3) will be corrupted.
 * ******************************************************************************
 */
typedef enum
{
	NO_ERRORS = 0,      /* No errors introduced */
	ONE_BIT_ERRORS,    	/* One bit error */
	TWO_BIT_ERRORS,    	/* Two bit errors */
	THREE_BIT_ERRORS,   /* Three bit errors */
	RANDOM_ERRORS       /* Random number of errors (between 0 and 3) */
} ErrorsTypeDef;


void nascerr_errors_burst_TBEC(uint16_t* ptr, uint32_t data_length, ErrorsTypeDef errors);
void nascerr_errors_burst_Hamming_EDAC(uint16_t* ptr, uint32_t data_length, ErrorsTypeDef errors);
void nascerr_errors_choose_error_generator(uint16_t* ptr, NASCERR_MODE ecc, uint32_t data_length, ErrorsTypeDef errors);

#endif /* INC_NASCERR_ERRORS_H_ */
