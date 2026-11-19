/*
 * NASCERR_ECCs.h
 *
 *  Created on: Sep 16, 2024
 *      Author: Clara
 */

#ifndef INC_NASCERR_ECCS_H_
#define INC_NASCERR_ECCS_H_

#include <stdint.h>
#include "NASCERR_System.h"
#include "NASCERR_Control.h"

uint16_t nascerr_eccs_edac_encoder(uint16_t data_in);
uint16_t nascerr_eccs_hamming_encoder(uint16_t data_in);
uint16_t nascerr_eccs_tbec_encoder(uint16_t data_in);
void nascerr_eccs_encoded_buffer(uint16_t* write_buffer, uint16_t* encoded_buffer, NASCERR_MODE ecc, nascerr_block length);





#endif /* INC_NASCERR_ECCS_H_ */
