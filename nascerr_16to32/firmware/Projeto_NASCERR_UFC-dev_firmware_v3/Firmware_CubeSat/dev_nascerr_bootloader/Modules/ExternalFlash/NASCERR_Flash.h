/**
 * ******************************************************************************
 *  @file    : nascerr_flash.h
 *  @brief   : Header for nascerr_flash.c file.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

#ifndef INC_NASCERR_FLASH_H_
#define INC_NASCERR_FLASH_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "main.h"
#include "NASCERR_System.h"
#include "NASCERR_Util.h"
#include "flash_w25n01g.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

#define FLASH_HEADER_BLOCK           (0)
#define FLASH_LOGS_BLOCK             (1)
#define FLASH_SCHEDULER_BLOCK        (2)
#define FLASH_EXPERIMENT_BLOCK       (3)
#define FLASH_IMAGE_BLOCK            (6)
#define FLASH_BACKUP_FIRMWARE_BLOCK  (38)

#define FLASH_HEADER_SIZE            (1)
#define FLASH_LOGS_SIZE              (1)
#define FLASH_SCHEDULER_SIZE         (1)
#define FLASH_EXPERIMENT_SIZE        (3)
#define FLASH_IMAGE_SIZE             (32)   /* ← NOVO: 32 blocos = 4 MB */
#define FLASH_BACKUP_FIRMWARE_SIZE	 (W25N01G_NUM_BLOCKS - FLASH_SCHEDULER_BLOCK - FLASH_HEADER_SIZE - FLASH_LOGS_SIZE - FLASH_EXPERIMENT_SIZE - FLASH_IMAGE_SIZE)

#define FLASH_IN_USE_BYTES				0xAAAAAAAA

#define MAX_TRY 						3
#define FLASH_IMAGE_MAX_BYTES        (FLASH_IMAGE_SIZE \
                                      * W25N01G_PAGES_PER_BLOCK \
                                      * W25N01G_PAGE_SIZE)

#define IMG_CHUNK_SIZE 2112

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/* Flash region type */
typedef enum {
    FLASH_HEADER = 0,
    FLASH_LOGS,
    FLASH_SCHEDULER,
    FLASH_EXPERIMENT,
    FLASH_IMAGE,
    FLASH_BACKUP_FIRMWARE,
    MAX_FLASH_REGION,
} FlashRegionTypeDef;

/* Comum Header Flash NASCERR */
#pragma pack(1)
typedef struct
{
	uint32_t in_use;
	uint32_t start_addr;
	uint32_t current_addr;
	uint32_t data_load;
} CHeaderTypeDef;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

SYSTEM_STATUS nascerr_flash_init(void);
SYSTEM_STATUS nascerr_flash_eraseArea(FlashRegionTypeDef area);
SYSTEM_STATUS nascerr_flash_eraseAll(void);
FLASHTypeDef nascerr_flash_writeAddress(uint32_t address, const uint8_t *data, uint32_t size);
FLASHTypeDef  nascerr_flash_flush(uint32_t address);
FLASHTypeDef  nascerr_flash_readAddress(uint32_t address, uint8_t *data, uint32_t size);

#endif /* INC_NASCERR_FLASH_H_ */
