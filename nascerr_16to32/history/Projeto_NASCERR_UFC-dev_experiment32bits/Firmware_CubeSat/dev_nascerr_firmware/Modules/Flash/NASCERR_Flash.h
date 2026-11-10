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
#include "NASCERR_W25Nxx.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

#define EXTERNAL_FLASH_MAX_FLOAT	(20000)
#define EXTERNAL_FLASH_MAX_LENGTH	(4*EXTERNAL_FLASH_MAX_FLOAT)

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/* Flash Struct */
typedef struct
{
	uint32_t current_addr;
	uint32_t load_data;
	uint32_t free_space;
	uint8_t  toread_flag;
	uint8_t  error;
} FlashTypeDef;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

SYSTEM_STATUS nascerr_flash_init(void);
SYSTEM_STATUS nascerr_flash_loadDataToWrite(uint32_t addr, uint8_t *data, uint32_t size);
SYSTEM_STATUS nascerr_flash_readDataFlash(uint32_t addr, uint8_t *data, uint32_t size);
FlashTypeDef  nascerr_flash_writeBufferToFlash(void);

#endif /* INC_NASCERR_FLASH_H_ */
