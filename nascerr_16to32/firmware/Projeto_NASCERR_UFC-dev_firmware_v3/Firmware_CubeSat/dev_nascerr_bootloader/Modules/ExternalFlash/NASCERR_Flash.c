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

#include "NASCERR_Flash.h"
#include "i2c_slave.h"
#include <string.h>

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

uint8_t vectorR[W25N01G_PAGE_SIZE];
uint8_t temp_buffer[W25N01G_PAGE_SIZE];

static uint8_t vectorW[W25N01G_PAGE_SIZE];
static uint32_t vectorW_index = 0;

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

	if (w25n01g_init() != true){ return retVal; }

	DEBUG_PRINT(LOG_INFO, "Flash Inicializada com sucesso... \r\n");

	retVal = STATUS_SUCCESS;

	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_eraseArea
 *  @brief     : Erases a specific flash memory area
 *  @param     : region - FlashRegionTypeDef indicating the region to be erased
 *  @retval    : SYSTEM_STATUS - STATUS_SUCCESS if successful, ERR_FLASH_ERASE otherwise
 * ******************************************************************************
 */
SYSTEM_STATUS nascerr_flash_eraseArea(FlashRegionTypeDef region)
{
	uint8_t retStatus = true;
	uint8_t attempt = 0;
	uint8_t erased = false;
	uint16_t block, num_bocks;

	switch (region) {
	case FLASH_HEADER:
		block = FLASH_HEADER_BLOCK;
		num_bocks = FLASH_HEADER_SIZE;
		break;
	case FLASH_LOGS:
		block = FLASH_LOGS_BLOCK;
		num_bocks = FLASH_LOGS_SIZE;
		break;
	case FLASH_SCHEDULER:
		block = FLASH_SCHEDULER_BLOCK;
		num_bocks = FLASH_SCHEDULER_SIZE;
		break;
	case FLASH_EXPERIMENT:
		block = FLASH_EXPERIMENT_BLOCK;
		num_bocks = FLASH_EXPERIMENT_SIZE;
		break;
	case FLASH_BACKUP_FIRMWARE:
		block = FLASH_BACKUP_FIRMWARE_BLOCK;
		num_bocks = FLASH_BACKUP_FIRMWARE_SIZE;
		break;
	case FLASH_IMAGE:
		block = FLASH_IMAGE_BLOCK;
		num_bocks = FLASH_IMAGE_SIZE;
		break;


	default:
		return ERR_FLASH_ERASE;
		break;
	}

	#if ENABLE_SABOTAGE_FLASH
	flash_fault_try_injection(FLASH_OP_ERASE);
	#endif

	for(int i = 0; i < num_bocks; i++)
	{
		attempt = 0;
		uint32_t blk = block + i;
		while (attempt < MAX_TRY)
		{
			if(w25n01g_eraseSector(W25N01G_BLOCK_TO_LINEAR(blk)))
			{
				erased = true;
				break;
			}

			#if ENABLE_SABOTAGE_FLASH
			flash_fault_check_attempt_count();
			#endif

			attempt++;
		}

		retStatus &= erased;
		i2c_slave_noBlockBus();
	}

	if (retStatus){ return STATUS_SUCCESS; }

	DEBUG_PRINT(LOG_ERROR, "ERRO de ERASE na FLASH NAO CORRIGIVEL: %d \n\r", region);
	return ERR_FLASH_ERASE;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_eraseAll
 *  @brief     : Apaga completamente toda a memoria flash externa.
 *  @param     : Nenhum
 *  @retval    : SYSTEM_STATUS - STATUS_SUCCESS se apagou com sucesso, ERR_FLASH_ERASE caso contrario.
 * ******************************************************************************
 */
SYSTEM_STATUS nascerr_flash_eraseAll(void)
{
	uint8_t attempt = 0;

	#if ENABLE_SABOTAGE_FLASH
    flash_fault_try_injection(FLASH_OP_ERASE);
	#endif

	while(attempt < MAX_TRY)
	{
		#if ENABLE_SABOTAGE_FLASH
		flash_fault_check_attempt_count();
		#endif

		if (w25n01g_eraseCompletely()) {
			return STATUS_SUCCESS;
		}

		attempt++;
	}

	DEBUG_PRINT(LOG_ERROR, "ERRO de ERASE na FLASH NAO CORRIGIVEL\n\r");
    return ERR_FLASH_ERASE;
}

/*
 * *******************************************************************************
 *  Function Name: nascerr_flash_writeAddress
 *  @brief       : Writes data to a specified address in the W25Nxx memory.
 *  @param       : address - The memory address to start writing to.
 *               : data - Pointer to the data buffer containing the bytes to write.
 *               : size - Number of bytes to write.
 *  @retval      : FLASHTypeDef - Structure containing:
 *                 - error (bool): true if an error occurred, false otherwise.
 *                 - start_addr (uint32_t): Initial address of the write operation.
 *                 - end_addr (uint32_t): Final address after the write.
 *                 - load_data (uint32_t): Number of bytes successfully written.
 *                 - corrupted (bool): true if data corruption was detected.
 * *******************************************************************************
 */
FLASHTypeDef nascerr_flash_writeAddress(uint32_t address, const uint8_t *data, uint32_t size)
{
    uint32_t addr = address;
    uint32_t index = 0;
    uint32_t remaining = size;
    uint8_t try = 0;

    FLASHTypeDef retVal = {
        .error = true,
        .start_addr = address,
        .end_addr = 0x00,
        .load_data = 0,
        .corrupted = false
    };

    #if ENABLE_SABOTAGE_FLASH
    flash_fault_try_injection(FLASH_OP_WRITE);
    #endif

    i2c_slave_noBlockBus();

    while (remaining > 0)
    {
        uint32_t to_copy = (remaining < (W25N01G_PAGE_SIZE - vectorW_index))
                               ? remaining
                               : (W25N01G_PAGE_SIZE - vectorW_index);

        memcpy(&vectorW[vectorW_index], &data[index], to_copy);
        vectorW_index += to_copy;
        index += to_copy;
        remaining -= to_copy;

        if (vectorW_index == W25N01G_PAGE_SIZE)
        {
            try = 0;
            uint8_t success = false;

            while (try < MAX_TRY)
            {
				#if ENABLE_SABOTAGE_FLASH
                flash_fault_check_attempt_count();
                #endif

                if (w25n01g_pageProgram(addr, vectorW, W25N01G_PAGE_SIZE))
                {
                    success = true;
                    break;
                }
                try++;
            }

            if (!success)
            {
                DEBUG_PRINT(LOG_ERROR, "ERRO de WRITE na FLASH NAO CORRIGIVEL\n\r");
                return retVal;
            }

            addr += W25N01G_PAGE_SIZE;
            vectorW_index = 0;
        }
        i2c_slave_noBlockBus();
    }

    retVal.error = false;
    retVal.end_addr = addr;
    retVal.load_data = index;
    return retVal;
}

/*
 * *******************************************************************************
 *  Function Name: nascerr_flash_flush
 *  @brief		 : Programs any remaining data from the internal page buffer (vectorW)
 *  into the W25Nxx NAND flash, executes the program operation, verifies data
 *  integrity via read-back, and retries on failure.
 *  @param       : address - The memory address where the remaining data will be written.
 *  @retval      : FLASHTypeDef - Structure containing:
 *                 - error (bool): true if an error occurred, false otherwise.
 *                 - start_addr (uint32_t): Initial address of the flush operation.
 *                 - end_addr (uint32_t): Final address after writing the remaining data.
 *                 - load_data (uint32_t): Number of bytes successfully written.
 *                 - corrupted (bool): true if data corruption was detected.
 * *******************************************************************************
 */
FLASHTypeDef nascerr_flash_flush(uint32_t address)
{
    FLASHTypeDef retVal = {
        .error = true,
        .start_addr = address,
        .end_addr = 0x00,
        .load_data = 0,
        .corrupted = false
    };

    if (vectorW_index == 0)
        return retVal;

    uint8_t attempt = 0;
    uint8_t success = 0;

	#if ENABLE_SABOTAGE_FLASH
    flash_fault_try_injection(FLASH_OP_FLUSH);
	#endif

    while (attempt <= MAX_TRY)
    {
        i2c_slave_noBlockBus();
        if (!w25n01g_pageProgram(address, vectorW, vectorW_index))
        {
            attempt++;

			#if ENABLE_SABOTAGE_FLASH
			flash_fault_check_attempt_count();
			#endif

            continue;
        }

        if (!w25n01g_flush())
        {
            attempt++;

			#if ENABLE_SABOTAGE_FLASH
			flash_fault_check_attempt_count();
			#endif

            continue;
        }


        FLASHTypeDef readReport = w25n01g_readBytes(address, temp_buffer, vectorW_index);

        if (readReport.error || readReport.corrupted)
        {
            attempt++;

			#if ENABLE_SABOTAGE_FLASH
			flash_fault_check_attempt_count();
			#endif

            continue;
        }

        if (memcmp(temp_buffer, vectorW, vectorW_index) == 0)
        {
            success = 1;
            break;
        }

		#if ENABLE_SABOTAGE_FLASH
		flash_fault_check_attempt_count();
		#endif

        attempt++;
    }

    if (!success)
	{
		retVal.corrupted = true;
		uint8_t palavra_igual = (memcmp(temp_buffer, vectorW, vectorW_index) == 0);
		DEBUG_PRINT(LOG_ERROR, "ERRO de FLUSH na FLASH NAO CORRIGIVEL: %d\n\r", palavra_igual);
		return retVal;
	}

    retVal.error = false;
    retVal.end_addr = address + vectorW_index;
    retVal.load_data = vectorW_index;

    vectorW_index = 0;

    i2c_slave_noBlockBus();

    return retVal;
}


/*
 * *******************************************************************************
 *  Function Name: nascerr_flash_readAddress
 *  @brief       : Reads data starting from a specific address in the W25Nxx memory.
 *  @param       : address - The memory address to start reading from.
 *               : data - Pointer to the buffer where the read bytes will be stored.
 *               : size - Number of bytes to read.
 *  @retval      : FLASHTypeDef - Structure containing:
 *                 - error (bool): true if an error occurred, false otherwise.
 *                 - start_addr (uint32_t): Initial address of the read operation.
 *                 - end_addr (uint32_t): Final address after the read operation.
 *                 - load_data (uint32_t): Number of bytes successfully read.
 *                 - corrupted (bool): true if data corruption was detected.
 * *******************************************************************************
 */
FLASHTypeDef nascerr_flash_readAddress(uint32_t address, uint8_t *data, uint32_t size)
{
    uint8_t try = 0;
    uint16_t data_area = W25N01G_PAGE_SIZE;
    uint32_t addr, remaining, length, index = 0;
    FLASHTypeDef readRest;
    FLASHTypeDef retVal = {
                    .error      = true,
                    .start_addr = address,
                    .end_addr   = 0x00,
                    .load_data  = 0,
                    .corrupted  = false
                };

    addr = address;
    remaining = size;

    #if ENABLE_SABOTAGE_FLASH
    flash_fault_try_injection(FLASH_OP_READ);
    #endif

    do
    {
        uint32_t offset = addr % data_area;
        uint32_t available_space = data_area - offset;
        length = (remaining < available_space) ? remaining : available_space;

        try = 0;
        uint8_t success = false;

        if (offset > 0) {
            while (try < MAX_TRY)
            {
                #if ENABLE_SABOTAGE_FLASH
                flash_fault_check_attempt_count();
                #endif

                readRest = w25n01g_readBytes((uint32_t)(addr - offset), temp_buffer, data_area);
                if (!readRest.error) {
                    memcpy(&data[index], &temp_buffer[offset], length);
                    success = true;
                    break;
                }

                try++;
            }

            if (!success) {
                retVal.corrupted = readRest.corrupted;
                break;
            }
        } else {
            while (try < MAX_TRY)
            {
                #if ENABLE_SABOTAGE_FLASH
                flash_fault_check_attempt_count();
                #endif

                readRest = w25n01g_readBytes((uint32_t)addr, &data[index], length);
                if (!readRest.error) {
                    success = true;
                    break;
                }

                try++;
            }

            if (!success) {
                retVal.corrupted = readRest.corrupted;
                break;
            }
        }

        if (readRest.load_data == 0) {
            break;
        }

        addr += length;
        index += length;
        remaining -= length;

        if (remaining == 0) {
            retVal.error     = false;
            retVal.end_addr  = addr;
            retVal.load_data = index;
            break;
        }

        i2c_slave_noBlockBus();
    } while (remaining > 0);

    if(retVal.error == true){
        DEBUG_PRINT(LOG_ERROR, "ERRO de LEITURA FLASH NAO CORRIGIVEL\n\r");
    }

    return retVal;
}
