/**
 * ******************************************************************************
 *  @file    : NASCERR_ECCs_32bits.h
 *  @brief   : Header file for 32-bit ECC encoding functions.
 *             Provides functions for encoding data using different ECC schemes
 *             (TBEC, MRSC) and encoding buffers.
 *  @author  : Clara
 * ******************************************************************************
 */

#ifndef INC_NASCERR_ECCS_32BITS_H_
#define INC_NASCERR_ECCS_32BITS_H_

/*
 * ******************************************************************************
 *  Includes
 * ******************************************************************************
 */
#include <stdint.h>
#include "NASCERR_Protocol.h"

/*
 * ******************************************************************************
 *  Exported functions
 * ******************************************************************************
 */
uint32_t nascerr_eccs_tbec_encoder(uint32_t data_in);
uint32_t nascerr_eccs_mrsc_encoder(uint32_t data_in);
void nascerr_eccs_encoded_buffer(uint32_t* write_buffer, uint32_t* encoded_buffer, NASCERR_MODE ecc, nascerr_block length);

#endif /* INC_NASCERR_ECCS_32BITS_H_ */
