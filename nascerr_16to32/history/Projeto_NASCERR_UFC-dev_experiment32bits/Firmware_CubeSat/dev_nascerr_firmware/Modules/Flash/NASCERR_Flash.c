/**
 * ******************************************************************************
 *  @file    : nascerr_flash.c
 *  @brief   : Biblioteca para gravar dados na Flash
 *  @author  : Marcus Anderson
 *  @note    :
 *
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include <string.h>
#include "NASCERR_Flash.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

uint8_t g_flashBuffer[EXTERNAL_FLASH_MAX_LENGTH];
FlashTypeDef g_externalFlash;

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_init
 *  @brief     : Initializes the flash system by setting up g_externalFlash parameters
 *  @param     : None
 *  @retval    : SYSTEM_STATUS - Returns SUCCESS if initialization is completed
 *               successfully, otherwise ERROR
 * ******************************************************************************
 */
SYSTEM_STATUS nascerr_flash_init(void)
{
	SYSTEM_STATUS retVal = ERR_INIT;

	g_externalFlash.current_addr = 0x00;
	g_externalFlash.load_data    = 0;
	g_externalFlash.free_space   = W25Nxx_FLASH_SIZE;
	g_externalFlash.toread_flag  = false;
	g_externalFlash.error        = false;

	if (W25Nxx_Init() != STATUS_SUCCESS){ return retVal; }

	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_loadDataToWrite
 *  @brief     : Loads data into g_flashBuffer and updates the g_externalFlash
 *               structure
 *  @param     : addr - Start address in the flash
 *               data - Pointer to the data to be loaded
 *               size - Size of the data to be loaded
 *  @retval    : SYSTEM_STATUS - Returns SUCCESS if data is loaded, otherwise ERROR
 * ******************************************************************************
 */
SYSTEM_STATUS nascerr_flash_loadDataToWrite(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (size > EXTERNAL_FLASH_MAX_LENGTH || (g_externalFlash.free_space < size)) {
        return ERR_FLASH;
    }

    g_externalFlash.current_addr = addr;

    memcpy(g_flashBuffer, data, size);

    g_externalFlash.load_data   = size;

    return STATUS_SUCCESS;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_readDataFlash
 *  @brief     : Reads data from flash memory in chunks to support larger sizes.
 *  @param     : addr - Start address in flash
 *               data - Pointer to buffer where data will be stored
 *               size - Total size of data to be read
 *  @retval    : STATUS_SUCCESS - Returns SUCCESS if data is read successfully, otherwise ERR_FLASH
 * ******************************************************************************
 */
SYSTEM_STATUS nascerr_flash_readDataFlash(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t remaining = size;
    uint32_t offset = 0;
    uint16_t chunk_size;

    while (remaining > 0) {
        chunk_size = (remaining > UINT16_MAX) ? UINT16_MAX : (uint16_t)remaining;

        if (W25Nxx_ReadAddress(addr + offset, data + offset, chunk_size) != STATUS_SUCCESS) {
            g_externalFlash.error = true;
            return ERR_FLASH;
        }

        remaining -= chunk_size;
        offset += chunk_size;
    }

    return STATUS_SUCCESS;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_writeBufferToFlash
 *  @brief     : Writes the contents of g_flashBuffer to the external flash memory
 *               and updates g_externalFlash structure.
 *  @param     : None
 *  @retval    : FlashTypeDef - Returns the updated g_externalFlash structure
 * ******************************************************************************
 */
FlashTypeDef  nascerr_flash_writeBufferToFlash(void)
{
	uint32_t addr = g_externalFlash.current_addr;
	uint32_t size = g_externalFlash.load_data;

	if(W25Nxx_WriteAddress(addr, g_flashBuffer, size) != STATUS_SUCCESS)
	{
		g_externalFlash.error = true;
	}

	g_externalFlash.current_addr = addr + g_externalFlash.load_data;
    g_externalFlash.toread_flag = true;
    g_externalFlash.free_space -= size;
	g_externalFlash.load_data = 0;

	return g_externalFlash;
}
