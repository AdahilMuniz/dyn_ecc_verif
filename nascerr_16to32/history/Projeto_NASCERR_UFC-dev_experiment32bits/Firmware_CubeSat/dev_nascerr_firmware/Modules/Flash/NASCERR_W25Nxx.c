/**
 * ******************************************************************************
 *  @file    : nascerr_W25Nxx.c
 *  @brief   : Biblioteca para memoria flash externa no modelo W25Nxx
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

#include "NASCERR_W25Nxx.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

W25Nxx_HandleTypeDef HandleW25Nxx;
uint32_t W25Nxx_time[3] = {0,0,0};

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

void W25Nxx_Delay(uint32_t Delay);
void W25Nxx_CSPin(uint8_t Select);
void W25Nxx_Lock(void);
void W25Nxx_UnLock(void);
uint8_t W25Nxx_Transmit(uint8_t *pData, size_t Size, uint32_t Timeout);
uint8_t W25Nxx_Receive(uint8_t *pData, size_t Size, uint32_t Timeout);
uint8_t W25Nxx_TransmitReceive(uint8_t *pTxData, uint8_t *pRxData, size_t Size, uint32_t Timeout);
uint8_t W25Nxx_ReadID(void);
uint8_t W25Nxx_WriteEnable(void);
uint8_t W25Nxx_WriteDisable(void);
uint8_t W25Nxx_WaitForWriting(timer_value Timeout);
uint8_t W25Nxx_ReadReg1(void);
uint8_t W25Nxx_ReadReg2(void);
uint8_t W25Nxx_ReadReg3(void);
uint8_t W25Nxx_WritePg(uint16_t PageNumber, uint8_t *Data, uint16_t Size, uint16_t Offset);
uint8_t W25Nxx_ReadPg(uint32_t Address, uint8_t *Data, uint32_t Size);

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_Init
 *  @brief     : Initializes the W25Nxx memory chip and checks its status
 *  @param     : None
 *  @retval    : Returns a value of type SYSTEM_STATUS indicating the result of
 *               the initialization. Possible return values are:
 *               - STATUS_OK: Initialization successful
 *               - ERR_INIT: Initialization failed
 * ******************************************************************************
 */
SYSTEM_STATUS W25Nxx_Init(void)
{
	SYSTEM_STATUS retVal = ERR_INIT;
	uint8_t id = 0;
	do
	{
		HandleW25Nxx.HSpi = W25Nxx_SPIHANDLE;
		HandleW25Nxx.Gpio = W25Nxx_CS_PORT;
		HandleW25Nxx.Pin = W25Nxx_CS_PIN;
		HandleW25Nxx.PageCnt = W25Nxx_NUM_PAGES;
		HandleW25Nxx.BlockCnt = W25Nxx_NUM_BLOCKS;
		W25Nxx_CSPin(1);

		id = W25Nxx_ReadID();

		if (W25Nxx_WriteDisable() == false) { return retVal; }

		if (id)
		{
			retVal = STATUS_SUCCESS;
			HandleW25Nxx.Inited = true;
		}

	} while (0);

  return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_EraseBlock
 *  @brief     : Performs a block erase operation on the W25Nxx memory.
 *  @param     : Block - Address of the block to be erased.
 *  @retval    : SYSTEM_STATUS - Returns STATUS_SUCCESS if the erase operation
 *               was successful, or ERR_FLASH_ERASE if there was an error.
 * ******************************************************************************
 */
SYSTEM_STATUS W25Nxx_EraseBlock(uint32_t Block)
{
	SYSTEM_STATUS retVal = ERR_FLASH_ERASE;
	uint16_t address = W25Nxx_BlockToPage(Block);
	uint8_t tx[4];

	W25Nxx_Lock();

	if (Block >= HandleW25Nxx.BlockCnt) { return retVal; }

	if (W25Nxx_WriteEnable() == false) { return retVal; }

	W25Nxx_CSPin(0);

	tx[0] = W25Nxx_CMD_BLOCKERASE;
	tx[1] = 0xFF; // Dummy Byte
	tx[2] = (address & 0xFF00) >> 8;
	tx[3] = (address & 0x00FF);
	if (W25Nxx_Transmit(tx, 4, 100) == false)
	{
	  W25Nxx_CSPin(1);
	  return retVal;
	}

	W25Nxx_CSPin(1);

	if (W25Nxx_WaitForWriting(TIMER_1ms*W25Nxx_TIME_ERASE_BLOCK))
	{
		retVal = STATUS_SUCCESS;
	}

	W25Nxx_WriteDisable();
	W25Nxx_UnLock();

	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_WriteAddress
 *  @brief       : Writes data to a specified address in the W25Nxx memory.
 *  @param       : Address - The memory address to start writing to.
 *               : Data - Pointer to the data buffer containing the bytes to write.
 *               : Size - Number of bytes to write.
 *  @retval      : SYSTEM_STATUS - Returns STATUS_SUCCESS if the write operation
 *                 was successful, or ERR_FLASH_BUSY if there was an error.
 * ******************************************************************************
 */
SYSTEM_STATUS W25Nxx_WriteAddress(uint32_t Address, uint8_t *Data, uint32_t Size)
{
	SYSTEM_STATUS retVal = ERR_FLASH;
	uint32_t page, add, offset, remaining, length, maximum, index = 0;
	add = Address;
	remaining = Size;

	W25Nxx_Lock();

	do
	{
		page = W25Nxx_AddressToPage(add);
		offset = add % W25Nxx_PAGE_SIZE;
		maximum = W25Nxx_PAGE_SIZE - offset;

		if (remaining <= maximum){ length = remaining; }
		else { length = maximum;}

		if (W25Nxx_WritePg(page, &Data[index], length, offset) == false){ break; }

		add += length;
		index += length;
		remaining -= length;

		if (remaining == 0)
		{
		  retVal = STATUS_SUCCESS;
		  break;
		}
	} while (remaining > 0);

	W25Nxx_UnLock();
	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_WritePage
 *  @brief       : Writes data to a specified page in the W25Nxx memory with an offset.
 *  @param       : PageNumber - The page number where data will be written.
 *               : Data - Pointer to the data buffer containing the bytes to write.
 *               : Size - Number of bytes to write.
 *               : Offset - Offset within the page where the write operation begins.
 *  @retval      : Returns a SYSTEM_STATUS indicating the result of the write operation.
 * ******************************************************************************
 */
SYSTEM_STATUS W25Nxx_WritePage(uint32_t PageNumber, uint8_t *Data, uint32_t Size, uint32_t Offset)
{
	SYSTEM_STATUS retVal = ERR_FLASH;

	W25Nxx_Lock();

	if (W25Nxx_WritePg(PageNumber, Data, Size, Offset) == true) { retVal = STATUS_SUCCESS; }

	W25Nxx_UnLock();
	return retVal;
}


/*
 * ******************************************************************************
 *  Function Name: W25Nxx_WriteBlock
 *  @brief       : Writes data to a specified block in the W25Nxx memory with an offset.
 *  @param       : BlockNumber - The block number to write to.
 *               : Data - Pointer to the buffer containing the bytes to be written.
 *               : Size - Number of bytes to write.
 *               : Offset - Offset within the block where the write operation begins.
 *  @retval      : Returns a SYSTEM_STATUS indicating the result of the write operation.
 * ******************************************************************************
 */
SYSTEM_STATUS W25Nxx_WriteBlock(uint32_t BlockNumber, uint8_t *Data, uint32_t Size, uint32_t Offset)
{
	SYSTEM_STATUS retVal = STATUS_SUCCESS;

	W25Nxx_Lock();

	if (Offset >= W25Nxx_BLOCK_SIZE){ return ERR_FLASH; }
	if (Size > (W25Nxx_BLOCK_SIZE - Offset)){ Size = W25Nxx_BLOCK_SIZE - Offset; }

	uint32_t bytesWritten = 0;
	uint32_t pageNumber = W25Nxx_BlockToPage(BlockNumber);
	pageNumber += Offset / W25Nxx_PAGE_SIZE;

	uint32_t remainingBytes = Size;
	uint32_t pageOffset = Offset % W25Nxx_PAGE_SIZE;

	while (remainingBytes > 0 && pageNumber < ((BlockNumber + 1) * (W25Nxx_BLOCK_SIZE / W25Nxx_PAGE_SIZE)))
	{
		uint32_t bytesToWrite = (remainingBytes > (W25Nxx_PAGE_SIZE - pageOffset)) ? (W25Nxx_PAGE_SIZE - pageOffset) : remainingBytes;
		if (W25Nxx_WritePg(pageNumber, Data + bytesWritten, bytesToWrite, pageOffset) == false)
		{
			retVal = ERR_FLASH;
			break;
		}
		bytesWritten += bytesToWrite;
		remainingBytes -= bytesToWrite;
		pageNumber++;
		pageOffset = 0;
	}

	W25Nxx_UnLock();
	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_ReadAddress
 *  @brief       : Reads data starting from a specific address in the W25Nxx memory.
 *  @param       : Address - The memory address to start reading from.
 *               : Data - Pointer to the buffer where the read bytes will be stored.
 *               : Size - Number of bytes to read.
 *  @retval      : Returns a SYSTEM_STATUS indicating the result of the read operation.
 * ******************************************************************************
 */
SYSTEM_STATUS W25Nxx_ReadAddress(uint32_t Address, uint8_t *Data, uint32_t Size)
{
	SYSTEM_STATUS retVal = ERR_FLASH;

	W25Nxx_Lock();

	if (W25Nxx_ReadPg(Address, Data, Size) == true) { retVal = STATUS_SUCCESS; }

	W25Nxx_UnLock();
	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_ReadPage
 *  @brief       : Reads data from a specified page in the W25Nxx memory with an offset.
 *  @param       : PageNumber - The page number to read from.
 *               : Data - Pointer to the buffer where the read bytes will be stored.
 *               : Size - Number of bytes to read.
 *               : Offset - Offset within the page where the read operation begins.
 *  @retval      : Returns a SYSTEM_STATUS indicating the result of the read operation.
 * ******************************************************************************
 */
SYSTEM_STATUS W25Nxx_ReadPage(uint32_t PageNumber, uint8_t *Data, uint32_t Size, uint32_t Offset)
{
	SYSTEM_STATUS retVal = ERR_FLASH;
	uint32_t address = W25Nxx_PageToAddress(PageNumber) + Offset;
	uint32_t maximum = W25Nxx_PAGE_SIZE - Offset;

	W25Nxx_Lock();

	if (Size > maximum) { Size = maximum; }
	if (W25Nxx_ReadPg(address, Data, Size) == true) { retVal = STATUS_SUCCESS; }

	W25Nxx_UnLock();
	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_ReadBlock
 *  @brief       : Reads data from a specified block in the W25Nxx memory with an offset.
 *  @param       : BlockNumber - The block number to read from.
 *               : Data - Pointer to the buffer where the read bytes will be stored.
 *               : Size - Number of bytes to read.
 *               : Offset - Offset within the block where the read operation begins.
 *  @retval      : Returns a SYSTEM_STATUS indicating the result of the read operation.
 * ******************************************************************************
 */
SYSTEM_STATUS W25Nxx_ReadBlock(uint32_t BlockNumber, uint8_t *Data, uint32_t Size, uint32_t Offset)
{
	SYSTEM_STATUS retVal = ERR_FLASH;
	uint32_t address = W25Nxx_BlockToAddress(BlockNumber) + Offset;
	uint32_t maximum = W25Nxx_BLOCK_SIZE - 1 - Offset;

	W25Nxx_Lock();

	if (Size > maximum) { Size = maximum; }
	if (W25Nxx_ReadPg(address, Data, Size) == true) { retVal = STATUS_SUCCESS; }

	W25Nxx_UnLock();
	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_Delay (LOCAL)
 *  @brief       : Delays the operation for a specified duration.
 *  @param       : Delay - The amount of time in milliseconds to delay.
 *  @retval      : None
 * ******************************************************************************
 */
void W25Nxx_Delay(uint32_t Delay)
{
#if W25Nxx_RTOS == W25Nxx_RTOS_DISABLE
  HAL_Delay(Delay);
#elif (W25Nxx_RTOS == W25Nxx_RTOS_CMSIS_V1) || (W25Nxx_RTOS == W25Nxx_RTOS_CMSIS_V2)
  uint32_t d = (configTICK_RATE_HZ * Delay) / 1000;
  if (d == 0)
      d = 1;
  osDelay(d);
#endif
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_CSPin (LOCAL)
 *  @brief       : Controls the Chip Select (CS) pin for the W25Nxx memory chip.
 *  @param       : Select - The value to set the CS pin (0 for LOW, 1 for HIGH).
 *  @retval      : None
 * ******************************************************************************
 */
void W25Nxx_CSPin(uint8_t Select)
{
  HAL_GPIO_WritePin(HandleW25Nxx.Gpio, HandleW25Nxx.Pin, (GPIO_PinState)Select);
  for (int i = 0; i < 10; i++);
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_Lock (LOCAL)
 *  @brief       : Waits until the W25Nxx resource is unlocked, then locks it.
 *  @param       : None
 *  @retval      : None
 * ******************************************************************************
 */
void W25Nxx_Lock(void)
{
  while (HandleW25Nxx.Lock)
  {
	  W25Nxx_Delay(1);
  }
  HandleW25Nxx.Lock = 1;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_UnLock (LOCAL)
 *  @brief       : Unlocks the W25Nxx resource for other operations.
 *  @param       : None
 *  @retval      : None
 * ******************************************************************************
 */
void W25Nxx_UnLock(void)
{
	HandleW25Nxx.Lock = 0;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_Transmit  (LOCAL)
 *  @brief       : Transmits a buffer of data to the W25Nxx memory device.
 *  @param       : pData - Pointer to the data buffer to be transmitted.
 *                 Size - Number of bytes to transmit.
 *                 Timeout - Timeout duration in milliseconds for the transmission.
 *  @retval      : Returns 0 if successful, non-zero if a timeout or error occurs.
 * ******************************************************************************
 */
uint8_t W25Nxx_Transmit(uint8_t *pData, size_t Size, uint32_t Timeout)
{
	uint8_t retVal = false;

//	if (HAL_SPI_Transmit(HandleW25Nxx.HSpi, pData, Size, Timeout) == HAL_OK)
//	{
//		retVal = true;
//	}

	if (SPI2_Transmit(HandleW25Nxx.HSpi, pData, Size, Timeout) == STATUS_OK)
	{
		retVal = true;
	}

	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_Receive  (LOCAL)
 *  @brief       : Receives a buffer of data from the W25Nxx memory device.
 *  @param       : pData - Pointer to the data buffer to store the received data.
 *                 Size - Number of bytes to receive.
 *                 Timeout - Timeout duration in milliseconds for the reception.
 *  @retval      : Returns 0 if successful, non-zero if a timeout or error occurs.
 * ******************************************************************************
 */
uint8_t W25Nxx_Receive(uint8_t *pData, size_t Size, uint32_t Timeout)
{
	uint8_t retVal = false;

//	if (HAL_SPI_Receive(HandleW25Nxx.HSpi, pData, Size, Timeout) == HAL_OK)
//	{
//		retVal = true;
//	}

	if (SPI2_Receive(HandleW25Nxx.HSpi, pData, Size, Timeout) == STATUS_OK)
	{
		retVal = true;
	}

	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_TransmitReceive  (LOCAL)
 *  @brief     : Transmits and receives data over SPI
 *  @param     : pTxData - Pointer to data to be transmitted
 *               pRxData - Pointer to data to be received
 *               Size - Amount of data to be transmitted/received
 *               Timeout - Timeout duration for SPI operation
 *  @retval    : uint8_t - Returns true (1) if successful, false (0) otherwise
 * ******************************************************************************
 */
uint8_t W25Nxx_TransmitReceive(uint8_t *pTxData, uint8_t *pRxData, size_t Size, uint32_t Timeout)
{
	uint8_t retVal = false;

  if (HAL_SPI_TransmitReceive(HandleW25Nxx.HSpi, pTxData, pRxData, Size, Timeout) == HAL_OK)
  {
    retVal = true;
  }

  return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_ReadID  (LOCAL)
 *  @brief     : Reads the ID via SPI and compares it with the defined values
 *  @param     : void
 *  @retval    : uint8_t - Returns true (1) if the ID matches the defined values,
 *                         otherwise returns false (0)
 * ******************************************************************************
 */
uint8_t W25Nxx_ReadID(void)
{
    uint8_t tx[2] = {W25Nxx_CMD_JEDECID, 0xFF};  /* > JEDEC ID command with bytes set for receiving the response */
    uint8_t rx[3];
    uint32_t id = 0;

    W25Nxx_CSPin(0);

    if (W25Nxx_Transmit(tx, 2, 100) == false)
    {
        W25Nxx_CSPin(1);
        return false;
    }

    if (W25Nxx_Receive(rx, 3, 100) == false)
	{
		W25Nxx_CSPin(1);
		return false;
	}

    W25Nxx_CSPin(1);

    /* > Update the HandleW25Nxx structure fields with the received values */
    HandleW25Nxx.Manufactor = rx[0];
    HandleW25Nxx.MemType = rx[1];
    HandleW25Nxx.Size = rx[2];

    /* > Construct the 32-bit ID */
    id = (rx[0] << 16) | (rx[1] << 8) | rx[2];

    /* > Directly compare the ID with the defined values */
    if ((id >> 16) == W25Nxx_MFR_ID &&
        ((id >> 8) & 0xFF) == W25Nxx_MEMTYPE &&
        (id & 0xFF) == W25Nxx_SIZE)
    {
        return true;
    }

    return false;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_WriteEnable
 *  @brief     : Enables write operations on the W25Nxx memory chip
 *  @param     : None
 *  @retval    : Returns an 8-bit value indicating the status of the operation
 * ******************************************************************************
 */
uint8_t W25Nxx_WriteEnable(void)
{
	uint8_t retVal = true;
	uint8_t tx[1] = {W25Nxx_CMD_WRITEENABLE};

	W25Nxx_CSPin(0);

	if (W25Nxx_Transmit(tx, 1, 100) == false)
	{
		retVal = false;
	}
	W25Nxx_CSPin(1);

	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_WriteDisable
 *  @brief     : Disables write operations on the W25Nxx memory chip
 *  @param     : None
 *  @retval    : Returns an 8-bit value indicating the status of the operation
 * ******************************************************************************
 */
uint8_t W25Nxx_WriteDisable(void)
{
	uint8_t retVal = true;
	uint8_t tx[1] = {W25Nxx_CMD_WRITEDISABLE};

	W25Nxx_CSPin(0);

	if (W25Nxx_Transmit(tx, 1, 100) == false)
	{
		retVal = false;
	}

	W25Nxx_CSPin(1);

	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_WaitForWriting
 *  @brief     : Waits for the write process to complete on the W25Nxx device.
 *  @param     : timer_value Timeout - Maximum wait time for the write completion.
 *  @retval    : uint8_t - Returns true (1) if the write completed successfully,
 *               or false (0) if a timeout occurred before completion.
 * ******************************************************************************
 */
uint8_t W25Nxx_WaitForWriting(timer_value Timeout)
{
	uint8_t retVal = false;

	nascerr_systimer_reset(W25Nxx_time);

	while (1)
	{
		W25Nxx_Delay(1);
		if (nascerr_systimer(Timeout, W25Nxx_time) == Set)
		{
		  break;
		}
		if ((W25Nxx_ReadReg1() & W25Nxx_STATUS1_BUSY) == 0)
		{
		  retVal = true;
		  break;
		}
	}
	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_ReadReg1
 *  @brief     : Reads status register 1 from the W25Nxx device.
 *  @param     : None.
 *  @retval    : uint8_t - The value of status register 1 read from the device.
 * ******************************************************************************
 */
uint8_t W25Nxx_ReadReg1(void)
{
	uint8_t retVal = 0;
	uint8_t tx[2] = {W25Nxx_CMD_READSTATUS1, W25Nxx_DUMMY_BYTE};
	uint8_t rx[2];

	W25Nxx_CSPin(0);

	if (W25Nxx_TransmitReceive(tx, rx, 2, 100) == true)
	{
		retVal = rx[1];
	}
	W25Nxx_CSPin(1);

	return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_WritePg (LOCAL)
 *  @brief       : Writes data to a specified page in the W25Nxx memory.
 *  @param       : PageNumber - The page number to write to.
 *               : Data - Pointer to the data buffer containing the bytes to write.
 *               : Size - Number of bytes to write.
 *               : Offset - Offset within the page where writing begins.
 *  @retval      : Returns a uint8_t indicating the result of the write operation.
 * ******************************************************************************
 */
uint8_t W25Nxx_WritePg(uint16_t PageNumber, uint8_t *Data, uint16_t Size, uint16_t Offset)
{
	uint8_t retVal = false;
	uint16_t address = 0, maximum = W25Nxx_PAGE_SIZE - Offset;
	uint8_t tx[5];

    if (PageNumber >= HandleW25Nxx.PageCnt) { return retVal; }
    if (Offset >= W25Nxx_PAGE_SIZE){ return retVal; }
    if (Size > maximum) { Size = maximum; }

    address = W25Nxx_PageToAddress(PageNumber) + Offset;

    if (W25Nxx_WriteEnable() == false) { return retVal; }

    W25Nxx_CSPin(0);

    tx[0] = W25Nxx_CMD_PAGEPROG;
	tx[1] = 0xFF; // Dummy Byte
	tx[2] = (address & 0xFF00) >> 8;
	tx[3] = (address & 0x00FF);
	if (W25Nxx_Transmit(tx, 4, 100) == false)
	{
	  W25Nxx_CSPin(1);
	  return retVal;
	}

    if (W25Nxx_Transmit(Data, Size, 1000) == false)
    {
    	W25Nxx_CSPin(1);
    	return retVal;
    }

    W25Nxx_CSPin(1);

    if (W25Nxx_WaitForWriting(TIMER_1ms*W25Nxx_TIME_WRITE_PAGE))
	{
	  retVal = STATUS_SUCCESS;
	}

  W25Nxx_WriteDisable();
  return retVal;
}

/*
 * ******************************************************************************
 *  Function Name: W25Nxx_ReadPg (LOCAL)
 *  @brief       : Reads data from a specified page in the W25Nxx memory.
 *  @param       : Address - The memory address to start reading from.
 *               : Data - Pointer to the buffer where the read bytes will be stored.
 *               : Size - Number of bytes to read.
 *  @retval      : Returns a uint8_t indicating the result of the read operation.
 * ******************************************************************************
 */
uint8_t W25Nxx_ReadPg(uint32_t Address, uint8_t *Data, uint32_t Size)
{
	uint8_t retVal = false;
	uint8_t tx[5];

    W25Nxx_CSPin(0);

    /* Definindo address de 32 ou 24 bits */
    if (HandleW25Nxx.BlockCnt >= 512)
    {
      tx[0] = W25Nxx_CMD_READDATA4ADD;
      tx[1] = (Address & 0xFF000000) >> 24;
      tx[2] = (Address & 0x00FF0000) >> 16;
      tx[3] = (Address & 0x0000FF00) >> 8;
      tx[4] = (Address & 0x000000FF);
      if (W25Nxx_Transmit(tx, 5, 100) == false)
      {
    	  W25Nxx_CSPin(1);
		  return retVal;
      }
    }
    else
    {
      tx[0] = W25Nxx_CMD_READDATA3ADD;
      tx[1] = (Address & 0x00FF0000) >> 16;
      tx[2] = (Address & 0x0000FF00) >> 8;
      tx[3] = (Address & 0x000000FF);
      if (W25Nxx_Transmit(tx, 4, 100) == false)
      {
    	  W25Nxx_CSPin(1);
		  return retVal;
      }
    }

    if (W25Nxx_Receive(Data, (uint16_t)Size, 2000) == false)
    {
    	W25Nxx_CSPin(1);
    	return retVal;
    }

    W25Nxx_CSPin(1);

    retVal = true;
    return retVal;
}
