/**
 * ******************************************************************************
 * @file    : NASCERR_ECCs_32bits.h
 * @brief   : Header file for NASCERR Error Correction Codes (ECC) functions.
 * @author  : Clara; Joao Manoel; Cicero Oliveira
 * ******************************************************************************
 */

#ifndef INC_NASCERR_ECCS_32BITS_H_
#define INC_NASCERR_ECCS_32BITS_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include <stdint.h>
#include "NASCERR_Protocol.h"
#include "NASCERR_IOs.h"
#include "NASCERR_Memory_32bits.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

/**
 * @brief Alias for the GPIO port used for ECC selection pins.
 * Assumed to be GPIO Port A.
 */
#define GPIO1 GPIOA

/**
 * @brief GPIO pin used for the first ECC selection signal (ecc_sel0).
 */
#define ECC_SEL_0 GPIO_PIN_4

/**
 * @brief GPIO pin used for the second ECC selection signal (ecc_sel1).
 */
#define ECC_SEL_1 GPIO_PIN_5

/**
 * @brief GPIO pin used for the third ECC selection signal (ecc_sel2).
 */
#define ECC_SEL_2 GPIO_PIN_8

/**
 * @brief Macro to flip a bit in a 2D array `signal` and update a `flip_mask`.
 * @param r Row index in the `signal` array.
 * @param c Column index in the `signal` array.
 * @note This macro assumes `signal[4][4]` and `flip_mask` are defined in the
 * calling scope, typically within an ECC decoder function. The mask
 * records which bits were flipped.
 */
#define FLIP_BIT(r,c) \
    do { \
      signal[(r)][(c)] ^= 1; \
      flip_mask |= 1u << ((r)*4 + (c)); \
    } while (0)

#define GETBIT(w, n)  (((w) >> (n)) & 1u)

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/**
 * @struct ECC_DECODER
 * @brief Structure to hold the results of an ECC decoding operation.
 */
typedef struct {
    uint16_t data_raw;      	/* palavra bruta (signal antes dos flips) */
    uint16_t data_corrected;    /* palavra após correção (signal depois)  */
    uint8_t  uncorrectable; 	/* 1 se restou síndrome ≠0                */
} ECC_DECODER;


//// Utilizadas somente para checagem do funcionamento dos encoders/decoders da fpga

typedef enum
{
	ENCODE_FAIL = 0,
	ENCODE_SUCESS
} ENCODING_STAT;

typedef struct
{
	ENCODING_STAT tbec;
	ENCODING_STAT msrc;
} ECC_ENCODER_STATUS;


typedef enum
{
	DECODE_FAIL = 0,
	DECODE_SUCESS
} DECODING_STAT;

typedef struct
{
	DECODING_STAT tbec;
	DECODING_STAT msrc;
} ECC_DECODER_STATUS;

typedef struct
{
	ENCODING_STAT encode;
	DECODING_STAT decode;
} ECC_COMPOSED_STATUS;

typedef struct
{
	ECC_COMPOSED_STATUS tbec;
	ECC_COMPOSED_STATUS msrc;
} ECC_CHECK_STATUS;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

uint32_t nascerr_ecc_encode(uint16_t data, NASCERR_MODE ecc);
ECC_DECODER nascerr_ecc_decode(uint32_t data, NASCERR_MODE ecc);
void nascerr_select_ecc(NASCERR_MODE ecc);
ECC_CHECK_STATUS nascerr_ecc_check_encoders_decoders(void);
NASCERR_MODE nascerr_get_current_ecc(void);
uint16_t nascerr_inject_error_u16(uint32_t addr_word, uint16_t data);
#endif /* INC_NASCERR_ECCS_32BITS_H_ */
