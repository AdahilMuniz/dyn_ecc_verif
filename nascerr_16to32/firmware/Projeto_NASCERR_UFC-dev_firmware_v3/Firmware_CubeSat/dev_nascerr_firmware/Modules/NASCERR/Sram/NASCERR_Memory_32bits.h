/**
 * ******************************************************************************
 *  @file    : NASCERR_Memory_16_to_32.h
 *  @brief   : Header file for NASCERR_Memory_32bits.c
 *  @author  : Clara
 * ******************************************************************************
 */

#ifndef INC_NASCERR_MEMORY_32BITS_H_
#define INC_NASCERR_MEMORY_32BITS_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include <stdlib.h>
#include <inttypes.h>
#include "NASCERR_Protocol.h"
#include "NASCERR_IOs.h"
#include "NASCERR_Util.h"
#include "NASCERR_ECCs_32bits.h"
#include "NASCERR_Config.h"
/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */
#define NASCERR_SRAM_ADDRESS 0x60000000UL
#define NASCERR_SRAM_END  0x60200000UL

#define WORDS_PER_BLOCK (NASCERR_SRAM_END - NASCERR_SRAM_ADDRESS) >> 6 // divides by 2**6 = 64 (current number of blocks)
#define BLOCK_SHIFT 15 // total words per block now is 2^15

#define MAX_TOLERANCE_FLIGHT  0.02f
#define MEMORY_TEST_FRACTION  WORDS_PER_BLOCK >> 2 // divides by 2**8 -> 8912 numbers of words
#define NUM_MODES             4

#define ERROR_STRIDE_WORDS        64u
#define ERROR_BIT_TO_FLIP         8u           /* bit 0..15 — usado por nascerr_inject_error_u16 */
#define ERROR_CODEWORD_FLIP_MASK  0xFFFF0000UL /* bits da codeword (bp2<<16|bp1) a inverter bp2 - ecc | bp1 - data*/

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/* Memory selection */
typedef enum
{
	BOTH = 0,
	MEMORY_1,
	MEMORY_2,
} MemorySelect;

/* Memory Status */
typedef enum {
    SRAM_STATUS_OK       = 0x00,  	/* > OK */
    SRAM_STATUS_WARNING  = 0x01,    /* > Error detected but within acceptable limits */
    SRAM_STATUS_CRITICAL = 0x02,    /* > Error rate exceeds acceptable limits */
    SRAM_STATUS_FAILED   = 0x03     /* > Unrecoverable error detected */
} SRAM_HealthStatus;

typedef struct __attribute__((packed)) {

	//SRAM_HealthStatus health_status;          /* > SRAM health status (SRAM_HealthStatus) */

    //float error_rate_percent;       		  /* > Global error rate percentage */

    uint16_t errors_bypass1;        		  /* > Error count in BYPASS_1 mode */
    uint16_t errors_bypass2;        	      /* > Error count in BYPASS_2 mode */
    uint16_t errors_tbec;           		  /* > Error count in TBEC mode */
    uint16_t errors_mrsc;           		  /* > Error count in MRSC mode */

    //NASCERR_MODE worst_ecc_mode;    		  /* > ECC mode with highest error rate (NASCERR_MODE) */

    uint32_t total_tests_run;       		  /* > Total number of tests executed */

    uint64_t affected_blocks;			      /* > Flag indicating in which blocks errors were detected*/

} SRAM_Health_Struct;

typedef struct __attribute__((packed)){
	uint16_t errors_tbec;           		  	/* > Error count in TBEC mode 										 	*/
	uint16_t errors_mrsc;           		  	/* > Error count in MRSC mode				 						 	*/
	uint32_t total_tests_run;					/* > Total number of tests encoders/decoders check executed 		 	*/
} Encoder_Decoder_Health;

typedef struct __attribute__((packed)){
	uint16_t errors_mem_1;						/* > Indication of how many latchups occured in memory 1			 	*/
	uint16_t errors_mem_2;						/* > Indication of how many latchups occured in memory 2		     	*/
	uint64_t mem_1_affected_blocks;				/* > Flag indicating in which blocks of memory 1 errors were detected	*/
	uint64_t mem_2_affected_blocks;				/* > Flag indicating in which blocks of memory 2 errors were detected	*/
} SRAMs_Health;


typedef struct __attribute__((packed))
{
	Encoder_Decoder_Health enc_dec_health;
	SRAMs_Health		   srams_health;
}Robust_Memory_Health;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

void nascerr_memory_write_sram_block(W_DataTypeDef write_type, uint32_t offset_addr, NASCERR_MODE ecc, uint32_t length);
void nascerr_memory_write_block_cell(uint16_t data, NASCERR_MODE ecc, uint32_t offset_addr, uint32_t length);
uint16_t nascerr_write_type(W_DataTypeDef write_type);

uint8_t nascerr_erase_sram(MemorySelect memory);
uint8_t nascerr_sram_init(void);
uint8_t nascerr_verify_sram(void);

uint16_t *nascerr_memory_read(uint32_t offset_addr);
uint16_t *nascerr_memory_read_block(uint32_t offset_addr, NASCERR_MODE ecc);

Robust_Memory_Health nascerr_memory_encoder_decoders_selfcheck(void);
Robust_Memory_Health nascerr_memory_complete_selfcheck(void);

void nascerr_memory_alert_block_corruption(uint8_t corrupted_block_flag);

void nascerr_memory_clear_health_info(void);

void nascerr_memory_write_image_buffer(uint16_t *buffer, NASCERR_MODE ecc, uint32_t offset_addr, uint32_t length);

void print_bit_errors(uint32_t address, uint32_t expected, uint32_t read_value, uint32_t diff, const char *mem_name, int num_bits, int8_t ecc_result);

void print_bit_errors_ecc(uint32_t address, uint32_t expected, uint32_t read_value, uint32_t diff, const char *mem_name, int num_bits, int8_t ecc_result, uint16_t data_expected, uint16_t data_actual);

void nascerr_memory_inject_postenc_errors(uint32_t length);

#endif /* INC_NASCERR_MEMORY_32BITS_H_ */
