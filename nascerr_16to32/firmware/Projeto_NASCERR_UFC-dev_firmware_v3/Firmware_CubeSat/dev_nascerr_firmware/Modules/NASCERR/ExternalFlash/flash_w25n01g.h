/**
 * ******************************************************************************
 *  @file    : flash_w25n01g.h
 *  @brief   : Header for flash_w25n01g.c file.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

#ifndef INC_FLASH_W25N01G_H_
#define INC_FLASH_W25N01G_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "main.h"
#include "spi.h"
#include "NASCERR_System.h"
#include "NASCERR_SysTime.h"

#if ENABLE_SABOTAGE_FLASH
#include "flash_saboteur.h"
#endif

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

/* Config: STM32 SPI PORT */
#define W25Nxx_SPIHANDLE            		&hspi3
#define W25Nxx_CS_PORT						MCU_FLASH_NCS_GPIO_Port
#define W25Nxx_CS_PIN						MCU_FLASH_NCS_Pin
#define W25Nxx_MFR_ID						0xEF
#define W25Nxx_SIZE				    		W25Nxx_SIZE_1GBIT
#define W25Nxx_MEMTYPE						0xAA

/* Device size parameters */
#define W25N01G_PAGE_SIZE         			2112
#define W25N01G_PAGES_PER_BLOCK   			64
#define W25N01G_BLOCKS_PER_DIE 				1024
#define W25N01G_NUM_PAGES     				65536
#define W25N01G_NUM_BLOCKS     				1024
#define W25N01G_ECC_AREA                    64      /* 64 Bytes ECC per column addr */

/* BB replacement area */
#define W25N01G_BB_MARKER_BLOCKS           	1
#define W25N01G_BB_REPLACEMENT_BLOCKS      	20
#define W25N01G_BB_MANAGEMENT_BLOCKS       	(W25N01G_BB_REPLACEMENT_BLOCKS + W25N01G_BB_MARKER_BLOCKS)
/* blocks are zero-based index */
#define W25N01G_BB_REPLACEMENT_START_BLOCK 	(W25N01G_BLOCKS_PER_DIE - W25N01G_BB_REPLACEMENT_BLOCKS)
#define W25N01G_BB_MANAGEMENT_START_BLOCK  	(W25N01G_BLOCKS_PER_DIE - W25N01G_BB_MANAGEMENT_BLOCKS)
#define W25N01G_BB_MARKER_BLOCK            	(W25N01G_BB_REPLACEMENT_START_BLOCK - W25N01G_BB_MARKER_BLOCKS)

/* Instructions */
#define W25N01G_INSTRUCTION_RDID             			0x9F
#define W25N01G_INSTRUCTION_DEVICE_RESET     			0xFF
#define W25N01G_INSTRUCTION_READ_STATUS_REG  			0x05
#define W25N01G_INSTRUCTION_READ_STATUS_ALTERNATE_REG  	0x0F
#define W25N01G_INSTRUCTION_WRITE_STATUS_REG 			0x01
#define W25N01G_INSTRUCTION_WRITE_STATUS_ALTERNATE_REG 	0x1F
#define W25N01G_INSTRUCTION_WRITE_ENABLE     			0x06
#define W25N01G_INSTRUCTION_DIE_SELECT       			0xC2
#define W25N01G_INSTRUCTION_BLOCK_ERASE      			0xD8
#define W25N01G_INSTRUCTION_READ_BBM_LUT     			0xA5
#define W25N01G_INSTRUCTION_BB_MANAGEMENT    			0xA1
#define W25N01G_INSTRUCTION_PROGRAM_DATA_LOAD        	0x02
#define W25N01G_INSTRUCTION_RANDOM_PROGRAM_DATA_LOAD 	0x84
#define W25N01G_INSTRUCTION_PROGRAM_EXECUTE  			0x10
#define W25N01G_INSTRUCTION_PAGE_DATA_READ   			0x13
#define W25N01G_INSTRUCTION_READ_DATA        			0x03
#define W25N01G_INSTRUCTION_FAST_READ        			0x1B
#define W25N01G_INSTRUCTION_FAST_READ_QUAD_OUTPUT 		0x6B

/* Config/status register addresses */
#define W25N01G_PROT_REG 0xA0
#define W25N01G_CONF_REG 0xB0
#define W25N01G_STAT_REG 0xC0

/* Bits in config/status register 1 (W25N01G_PROT_REG) */
#define W25N01G_PROT_CLEAR                	(0)
#define W25N01G_PROT_SRP1_ENABLE          	(1 << 0)
#define W25N01G_PROT_WP_E_ENABLE          	(1 << 1)
#define W25N01G_PROT_TB_ENABLE            	(1 << 2)
#define W25N01G_PROT_PB0_ENABLE           	(1 << 3)
#define W25N01G_PROT_PB1_ENABLE           	(1 << 4)
#define W25N01G_PROT_PB2_ENABLE           	(1 << 5)
#define W25N01G_PROT_PB3_ENABLE           	(1 << 6)
#define W25N01G_PROT_SRP2_ENABLE          	(1 << 7)

/* Bits in config/status register 2 (W25N01G_CONF_REG) */
#define W25N01G_CONFIG_ECC_ENABLE         	(1 << 4)
#define W25N01G_CONFIG_BUFFER_READ_MODE   	(1 << 3)

/* Bits in config/status register 3 (W25N01G_STATREG)*/
#define W25N01G_STATUS_BBM_LUT_FULL       	(1 << 6)
#define W25N01G_STATUS_FLAG_ECC_POS       	4
#define W25N01G_STATUS_FLAG_ECC_MASK      	((1 << 5)|(1 << 4))
#define W25N01G_STATUS_FLAG_ECC(status)   	(((status) & W25N01G_STATUS_FLAG_ECC_MASK) >> 4)
#define W25N01G_STATUS_PROGRAM_FAIL       	(1 << 3)
#define W25N01G_STATUS_ERASE_FAIL         	(1 << 2)
#define W25N01G_STATUS_FLAG_WRITE_ENABLED 	(1 << 1)
#define W25N01G_STATUS_FLAG_BUSY          	(1 << 0)

#define W25N01G_BBLUT_TABLE_ENTRY_COUNT     20
#define W25N01G_BBLUT_TABLE_ENTRY_SIZE      4  // in bytes

/* Bits in LBA for BB LUT */
#define W25N01G_BBLUT_STATUS_ENABLED 		(1 << 15)
#define W25N01G_BBLUT_STATUS_INVALID 		(1 << 14)
#define W25N01G_BBLUT_STATUS_MASK    		(W25N01G_BBLUT_STATUS_ENABLED | W25N01G_BBLUT_STATUS_INVALID)

/* Some useful defs and macros */
#define W25N01G_LINEAR_TO_COLUMN(laddr) 	((laddr) % W25N01G_PAGE_SIZE)
#define W25N01G_LINEAR_TO_PAGE(laddr) 		((laddr) / W25N01G_PAGE_SIZE)
#define W25N01G_LINEAR_TO_BLOCK(laddr) 		(W25N01G_LINEAR_TO_PAGE(laddr) / W25N01G_PAGES_PER_BLOCK)
#define W25N01G_BLOCK_TO_PAGE(block) 		((block) * W25N01G_PAGES_PER_BLOCK)
#define W25N01G_BLOCK_TO_LINEAR(block) 		(W25N01G_BLOCK_TO_PAGE(block) * W25N01G_PAGE_SIZE)

// IMPORTANT: Timeout values are currently required to be set to the highest value required by any of the supported flash chips by this driver

// The timeout values (2ms minimum to avoid 1 tick advance in consecutive calls to millis).
#define W25N01G_TIMEOUT_PAGE_READ_MS        5   // tREmax = 60us (ECC enabled)
#define W25N01G_TIMEOUT_PAGE_PROGRAM_MS     5   // tPPmax = 700us
#define W25N01G_TIMEOUT_BLOCK_ERASE_MS      15  // tBEmax = 10ms
#define W25N01G_TIMEOUT_RESET_MS            500 // tRSTmax = 500ms


/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/* W25Nxx memory size type */
typedef enum
{
  W25Nxx_SIZE_ERROR   = 0,
  W25Nxx_SIZE_1MBIT   = 0x11,
  W25Nxx_SIZE_2MBIT   = 0x12,
  W25Nxx_SIZE_4MBIT   = 0x13,
  W25Nxx_SIZE_8MBIT   = 0x14,
  W25Nxx_SIZE_16MBIT  = 0x15,
  W25Nxx_SIZE_32MBIT  = 0x16,
  W25Nxx_SIZE_64MBIT  = 0x17,
  W25Nxx_SIZE_128MBIT = 0x18,
  W25Nxx_SIZE_256MBIT = 0x19,
  W25Nxx_SIZE_512MBIT = 0x20,
  W25Nxx_SIZE_1GBIT   = 0x21,
} W25Nxx_SizeTypeDef;

/* W25Nxx handler type */
typedef struct
{
  SPI_HandleTypeDef      *HSpi;
  GPIO_TypeDef           *Gpio;
  W25Nxx_SizeTypeDef     Size;
  uint8_t                Manufactor;
  uint8_t                Inited;
  uint8_t                MemType;
  uint8_t                Lock;
  uint8_t                Reserved;
  uint32_t               Pin;
  uint32_t               PageCnt;
  uint32_t               BlockCnt;
} W25Nxx_HandleTypeDef;

/* Flash Struct */
#pragma pack(1)
typedef struct
{
	uint32_t start_addr;
	uint32_t end_addr;
	uint32_t load_data;
	uint8_t  error;
	uint8_t  corrupted;
} FLASHTypeDef;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

void w25n01g_programDataLoad(uint16_t columnAddress, uint8_t *data, int length);
void w25n01g_randomProgramDataLoad(uint16_t columnAddress, uint8_t *data, int length);
void w25n01g_programExecute(uint32_t pageAddress);
void w25n01g_pageProgramBegin(uint32_t address);
void w25n01g_pageProgramContinue(uint8_t *data, int length);
void w25n01g_pageProgramFinish(void);


/* **************************************************************************** */
/* 						FUNCTIONS FOR UPPER LAYERS								*/
/* **************************************************************************** */
uint8_t w25n01g_init(void);
void w25n01g_deviceReset(void);
uint8_t w25n01g_eraseSector(uint32_t address);
uint8_t w25n01g_eraseCompletely(void);
uint8_t w25n01g_pageProgram(uint32_t address, uint8_t *data, int length);
uint8_t w25n01g_flush(void);
FLASHTypeDef  w25n01g_readBytes(uint32_t address, uint8_t *buffer, int length);

#endif /* INC_FLASH_W25N01G_H_ */
