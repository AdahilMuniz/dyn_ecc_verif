/**
 * ******************************************************************************
 *  @file    : flash_w25n01g.c
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
#include "flash_w25n01g.h"
#include "i2c_slave.h"
#include "NASCERR_Log.h"
#include "NASCERR_Util.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */
uint8_t id = 0;
W25Nxx_HandleTypeDef HandleW25Nxx;
uint32_t W25Nxx_time[3] = {0,0,0};

static uint32_t currentPage = UINT32_MAX;
static uint32_t programStartAddress;
static uint32_t programLoadAddress;
uint8_t bufferDirty = false;
uint8_t isProgramming = false;
uint8_t readReg;

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

void w25n01g_setTimeout(uint32_t timeoutMillis);
void W25N01G_CSP(uint8_t Select);
uint8_t w25n01g_transmit(uint8_t *pData, size_t Size, uint32_t Timeout);
uint8_t w25n01g_receive(uint8_t *pData, size_t Size, uint32_t Timeout);
uint8_t w25n01g_transmitReceive(uint8_t *pTxData, uint8_t *pRxData, size_t Size, uint32_t Timeout);
void w25n01g_performOneByteCommand(uint8_t command);
void w25n01g_performCommandWithPageAddress(uint8_t command, uint32_t pageAddress);
uint8_t w25n01g_readRegister(uint8_t reg);
void w25n01g_writeRegister(uint8_t reg, uint8_t data);
uint8_t w25n01g_isReady(void);
uint8_t w25n01g_waitForReady(timer_value Timeout);
void w25n01g_writeEnable(void);

/**
 * ******************************************************************************
 *  Function Name: w25n01g_setTimeout (LOCAL)
 *  @brief       : Delays the operation for a specified duration.
 *  @param       : Delay - The amount of time in milliseconds to delay.
 *  @retval      : None
 * ******************************************************************************
 */
void w25n01g_setTimeout(uint32_t timeoutMillis)
{
	 HAL_Delay(timeoutMillis);
}

/**
 * ******************************************************************************
 *  Function Name: W25N01G_CSP (LOCAL)
 *  @brief       : Controls the Chip Select (CS) pin for the W25Nxx memory chip.
 *  @param       : Select - The value to set the CS pin (0 for LOW, 1 for HIGH).
 *  @retval      : None
 * ******************************************************************************
 */
void W25N01G_CSP(uint8_t Select)
{
  HAL_GPIO_WritePin(HandleW25Nxx.Gpio, HandleW25Nxx.Pin, (GPIO_PinState)Select);
  for (int i = 0; i < 10; i++);
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_transmit  (LOCAL)
 *  @brief       : Transmits a buffer of data to the W25Nxx memory device.
 *  @param       : pData - Pointer to the data buffer to be transmitted.
 *                 Size - Number of bytes to transmit.
 *                 Timeout - Timeout duration in milliseconds for the transmission.
 *  @retval      : Returns 1 if successful, non-zero if a timeout or error occurs.
 * ******************************************************************************
 */
uint8_t w25n01g_transmit(uint8_t *pData, size_t Size, uint32_t Timeout)
{
	uint8_t retVal = false;
	SYSTEM_STATUS retValHal = SPI2_Transmit(HandleW25Nxx.HSpi, pData, Size, Timeout);

	if (retValHal == STATUS_OK)
	{
		retVal = true;
	}
	else
	{
		DEBUG_PRINT(LOG_ERROR, "[SPI] Falha na transmissao SPI (Status=%d)\r\n", retValHal);
	}

	return retVal;
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_receive  (LOCAL)
 *  @brief       : Receives a buffer of data from the W25Nxx memory device.
 *  @param       : pData - Pointer to the data buffer to store the received data.
 *                 Size - Number of bytes to receive.
 *                 Timeout - Timeout duration in milliseconds for the reception.
 *  @retval      : Returns 1 if successful, non-zero if a timeout or error occurs.
 * ******************************************************************************
 */
uint8_t w25n01g_receive(uint8_t *pData, size_t Size, uint32_t Timeout)
{
	uint8_t retVal = false;
	SYSTEM_STATUS retValHal = SPI2_Receive(HandleW25Nxx.HSpi, pData, Size, Timeout);

	if (retValHal == STATUS_OK)
	{
		retVal = true;
	}
	else
	{
		DEBUG_PRINT(LOG_ERROR, "[SPI] Falha na recepcao SPI (Status=%d)\r\n", retValHal);
	}

	return retVal;
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_transmitReceive  (LOCAL)
 *  @brief     : Transmits and receives data over SPI
 *  @param     : pTxData - Pointer to data to be transmitted
 *               pRxData - Pointer to data to be received
 *               Size - Amount of data to be transmitted/received
 *               Timeout - Timeout duration for SPI operation
 *  @retval    : uint8_t - Returns true (1) if successful, false (0) otherwise
 * ******************************************************************************
 */
uint8_t w25n01g_transmitReceive(uint8_t *pTxData, uint8_t *pRxData, size_t Size, uint32_t Timeout)
{
	uint8_t retVal = false;

  if (HAL_SPI_TransmitReceive(HandleW25Nxx.HSpi, pTxData, pRxData, Size, Timeout) == HAL_OK)
  {
    retVal = true;
  }
  else
  {
	  DEBUG_PRINT(LOG_ERROR, "[SPI] Falha em TransmitReceive SPI\r\n");
  }

  return retVal;
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_performOneByteCommand  (LOCAL)
 *  @brief       : Executa um comando de unico byte no dispositivo W25Nxx
 *  @param       : command - Byte do comando a ser executado
 *  @retval      : None
 * ******************************************************************************
 */
void w25n01g_performOneByteCommand(uint8_t command)
{
	uint8_t cmd[] = {command};

	W25N01G_CSP(0);
	w25n01g_transmit(cmd, 1, 100);
	W25N01G_CSP(1);
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_performCommandWithPageAddress  (LOCAL)
 *  @brief       : Executa um comando seguido de um endereco de pagina no dispositivo
 *  @param       : command - Byte do comando a ser executado
 *                 pageAddress - Endereco da pagina de 32 bits
 *  @retval      : None
 * ******************************************************************************
 */
void w25n01g_performCommandWithPageAddress(uint8_t command, uint32_t pageAddress)
{
	uint8_t cmd[] = { command, 0, (pageAddress >> 8) & 0xff, (pageAddress >> 0) & 0xff};

	W25N01G_CSP(0);
	w25n01g_transmit(cmd, sizeof(cmd), 100);
	W25N01G_CSP(1);
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_readRegister  (LOCAL)
 *  @brief       : Le o conteudo de um registrador do dispositivo
 *  @param       : reg - Registrador a ser lido
 *  @retval      : uint8_t - Valor lido do registrador
 * ******************************************************************************
 */
uint8_t w25n01g_readRegister(uint8_t reg)
{
	W25N01G_CSP(0);
	uint8_t cmd[3] = { W25N01G_INSTRUCTION_READ_STATUS_REG, reg, 0 };
	uint8_t in[3];

	w25n01g_transmitReceive(cmd, in, sizeof(cmd), 100);
	W25N01G_CSP(1);

	#if ENABLE_SABOTAGE_FLASH
		return flash_fault_sabotage_state(in[2]);
	#else
		return in[2];
	#endif
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_writeRegister  (LOCAL)
 *  @brief       : Escreve em um registrador do dispositivo
 *  @param       : reg - Registrador a ser escrito
 *                 data - Valor a ser escrito no registrador
 *  @retval      : None
 * ******************************************************************************
 */
void w25n01g_writeRegister(uint8_t reg, uint8_t data)
{
	uint8_t cmd[3] = { W25N01G_INSTRUCTION_WRITE_STATUS_REG, reg, data };

	W25N01G_CSP(0);
	w25n01g_transmit(cmd, sizeof(cmd), 100);
	W25N01G_CSP(1);
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_deviceReset  (LOCAL)
 *  @brief       : Executa um reset completo no dispositivo
 *  @param       : None
 *  @retval      : None
 * ******************************************************************************
 */
void w25n01g_deviceReset(void)
{
    w25n01g_performOneByteCommand(W25N01G_INSTRUCTION_DEVICE_RESET);

    w25n01g_setTimeout(W25N01G_TIMEOUT_RESET_MS);
    w25n01g_waitForReady(TIMER_100ms);

    /* No protection, WP-E off, WP-E prevents use of IO2 */
    w25n01g_writeRegister(W25N01G_PROT_REG, W25N01G_PROT_CLEAR);

    /* Buffered read mode (BUF = 1), ECC enabled (ECC = 1) */
    w25n01g_writeRegister(W25N01G_CONF_REG, W25N01G_CONFIG_BUFFER_READ_MODE);
//    w25n01g_writeRegister(W25N01G_CONF_REG, W25N01G_CONFIG_ECC_ENABLE|W25N01G_CONFIG_BUFFER_READ_MODE);
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_isReady  (LOCAL)
 *  @brief       : Verifica se o dispositivo esta pronto para operacoes
 *  @param       : None
 *  @retval      : uint8_t - True (1) se pronto, False (0) se ocupado
 * ******************************************************************************
 */
uint8_t w25n01g_isReady(void)
{
    uint8_t status = w25n01g_readRegister(W25N01G_STAT_REG);

    return ((status & W25N01G_STATUS_FLAG_BUSY) == 0);
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_waitForReady  (LOCAL)
 *  @brief       : Aguarda ate o dispositivo ficar pronto ou timeout ocorrer
 *  @param       : Timeout - Tempo maximo de espera em milissegundos
 *  @retval      : uint8_t - True (1) se pronto, False (0) se timeout
 * ******************************************************************************
 */
uint8_t w25n01g_waitForReady(timer_value Timeout)
{
	nascerr_systimer_reset(W25Nxx_time);

    while (!w25n01g_isReady()) {
    	if (nascerr_systimer(Timeout, W25Nxx_time) == Set) {
    		DEBUG_PRINT(LOG_ERROR, "[SPI] Timeout aguardando flash W25N01G pronta\r\n");
            return false;
        }
    }

    return true;
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_writeEnable  (LOCAL)
 *  @brief     :
 *  @param     :
 *  @retval    :
 * ******************************************************************************
 */
void w25n01g_writeEnable(void)
{
   w25n01g_performOneByteCommand(W25N01G_INSTRUCTION_WRITE_ENABLE);
}


/**
 * ******************************************************************************
 *  Function Name: w25n01g_detect (LOCAL)
 *  @brief       : Detecta e verifica a presenca do dispositivo W25Nxx
 *  @param       : None
 *  @retval      : uint8_t - True (1) se dispositivo detectado, False (0) caso contrario
 * ******************************************************************************
 */
uint8_t w25n01g_detect(void)
{
    uint8_t cmd[] = {W25N01G_INSTRUCTION_RDID, 0};  /* > JEDEC ID command with bytes set for receiving the response */
    uint8_t rx[3];
    uint32_t id = 0;

    W25N01G_CSP(0);

    if (!w25n01g_transmit(cmd, sizeof(cmd), 1000)){
    	W25N01G_CSP(1);
        return false;
    }

    if (!w25n01g_receive(rx, 3, 1000)) {
    	W25N01G_CSP(1);
		return false;
	}

    W25N01G_CSP(1);

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
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

/**
 * ******************************************************************************
 *  Function Name: w25n01g_init
 *  @brief       : Inicializa o dispositivo e configura parametros basicos
 *  @param       : None
 *  @retval      : uint8_t - True (1) se inicializacao bem sucedida
 * ******************************************************************************
 */
uint8_t w25n01g_init(void)
{
	uint8_t retVal = false;

	do
	{
		HandleW25Nxx.HSpi = W25Nxx_SPIHANDLE;
		HandleW25Nxx.Gpio = W25Nxx_CS_PORT;
		HandleW25Nxx.Pin = W25Nxx_CS_PIN;
		HandleW25Nxx.PageCnt = W25N01G_NUM_PAGES;
		HandleW25Nxx.BlockCnt = W25N01G_NUM_BLOCKS;


		id = w25n01g_detect();
		if (id)
		{
			retVal = true;
			HandleW25Nxx.Inited = true;
		}
		else
		{
			DEBUG_PRINT(LOG_ERROR, "[SPI] Flash W25N01G nao detectada! SPI indisponivel\r\n");
		}

		w25n01g_deviceReset();

	} while (0);

  return retVal;
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_eraseSector
 *  @brief       : Apaga um bloco completo de memoria
 *  @param       : address - Endereco linear dentro do bloco a ser apagado
 *  @retval      : uint8_t - True (1) se apagou com sucesso, False (0) caso contrario
 * ******************************************************************************
 */
uint8_t w25n01g_eraseSector(uint32_t address)
{

    w25n01g_waitForReady(TIMER_100ms);

    w25n01g_writeEnable();

//    w25n01g_writeRegister(W25N01G_PROT_REG, W25N01G_PROT_CLEAR);
//    w25n01g_writeRegister(W25N01G_PROT_REG, 0);

    w25n01g_performCommandWithPageAddress(W25N01G_INSTRUCTION_BLOCK_ERASE, W25N01G_LINEAR_TO_PAGE(address));

    w25n01g_setTimeout(W25N01G_TIMEOUT_BLOCK_ERASE_MS);

    if ((w25n01g_readRegister(W25N01G_STAT_REG) & W25N01G_STATUS_ERASE_FAIL) == 0)
	{
	  return true;
	}

	DEBUG_PRINT(LOG_ERROR, "[SPI] Falha no apagamento do bloco (addr=0x%08lX)\r\n", address);
	return false;
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_eraseCompletely
 *  @brief       : Apaga toda a memoria flash
 *  @param       : None
 *  @retval      : uint8_t - True (1) se apagou com sucesso, False (0) caso contrario
 * ******************************************************************************
 */
uint8_t w25n01g_eraseCompletely(void)
{
    for (uint32_t block = 0; block < W25N01G_BLOCKS_PER_DIE; block++) {
        w25n01g_eraseSector(W25N01G_BLOCK_TO_LINEAR(block));
        i2c_slave_noBlockBus();
    }

    if ((w25n01g_readRegister(W25N01G_STAT_REG) & W25N01G_STATUS_ERASE_FAIL) == 0)
	{
	  return true;
	}

	return false;
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_programDataLoad
 *  @brief       : Carrega dados para programacao na memoria buffer do dispositivo
 *  @param       : columnAddress - Endereco de coluna inicial
 *                 data - Ponteiro para os dados a serem programados
 *                 length - Numero de bytes a serem carregados
 *  @retval      : None
 * ******************************************************************************
 */
void w25n01g_programDataLoad(uint16_t columnAddress, uint8_t *data, int length)
{

    w25n01g_waitForReady(TIMER_100ms);

	uint8_t cmd[] = { W25N01G_INSTRUCTION_PROGRAM_DATA_LOAD, columnAddress >> 8, columnAddress & 0xff };

	W25N01G_CSP(0);
	w25n01g_transmit(cmd, sizeof(cmd), 100);
	w25n01g_transmit(data, length, 1000);
	W25N01G_CSP(1);

	readReg =  w25n01g_readRegister(W25N01G_STAT_REG);

    w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_PROGRAM_MS);
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_randomProgramDataLoad
 *  @brief       : Carrega dados para programacao aleatoria na memoria buffer
 *  @param       : columnAddress - Endereco de coluna inicial
 *                 data - Ponteiro para os dados a serem programados
 *                 length - Numero de bytes a serem carregados
 *  @retval      : None
 * ******************************************************************************
 */
void w25n01g_randomProgramDataLoad(uint16_t columnAddress, uint8_t *data, int length)
{
    uint8_t cmd[] = { W25N01G_INSTRUCTION_RANDOM_PROGRAM_DATA_LOAD, columnAddress >> 8, columnAddress & 0xff };

    w25n01g_waitForReady(TIMER_100ms);

    W25N01G_CSP(0);
    w25n01g_transmit(cmd, sizeof(cmd), 100);
    w25n01g_transmit(data, length, 1000);
	W25N01G_CSP(1);

    w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_PROGRAM_MS);
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_programExecute
 *  @brief       : Executa a programacao dos dados carregados no buffer
 *  @param       : pageAddress - Endereco da pagina de destino
 *  @retval      : None
 * ******************************************************************************
 */
void w25n01g_programExecute(uint32_t pageAddress)
{
    w25n01g_waitForReady(TIMER_100ms);

    w25n01g_performCommandWithPageAddress(W25N01G_INSTRUCTION_PROGRAM_EXECUTE, pageAddress);

    w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_PROGRAM_MS);
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_pageProgramBegin
 *  @brief     : Inicia o processo de programacao de uma pagina na memoria W25N01G.
 *  @param     : address - Endereco inicial da pagina a ser programada.
 *  @retval    : Nenhum
 * ******************************************************************************
 */
void w25n01g_pageProgramBegin(uint32_t address)
{
    if (bufferDirty) {
        if (address != programLoadAddress) {
            w25n01g_waitForReady(TIMER_100ms);

            isProgramming = false;

            w25n01g_writeEnable();

            w25n01g_programExecute(W25N01G_LINEAR_TO_PAGE(programStartAddress));

            bufferDirty = false;
            isProgramming = true;
        }
    } else {
        programStartAddress = programLoadAddress = address;
    }
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_pageProgramContinue
 *  @brief     : Continua a programacao da pagina carregando os dados na memoria.
 *  @param     : data - Ponteiro para os dados a serem programados.
 *               length - Comprimento dos dados em bytes.
 *  @retval    : Nenhum
 * ******************************************************************************
 */
void w25n01g_pageProgramContinue(uint8_t *data, int length)
{
    // Check for page boundary overrun
    w25n01g_waitForReady(TIMER_100ms);

    w25n01g_writeEnable();

    isProgramming = false;
    if (!bufferDirty) {
    	w25n01g_programDataLoad(W25N01G_LINEAR_TO_COLUMN(programLoadAddress), data, length);
    } else {
        w25n01g_randomProgramDataLoad(W25N01G_LINEAR_TO_COLUMN(programLoadAddress), data, length);
    }

    w25n01g_waitForReady(TIMER_100ms);

    bufferDirty = true;
	programLoadAddress += length;

    readReg =  w25n01g_readRegister(W25N01G_STAT_REG);
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_pageProgramFinish
 *  @brief     : Finaliza o processo de programacao da pagina na memoria W25N01G.
 *  @param     : Nenhum
 *  @retval    : Nenhum
 * ******************************************************************************
 */
void w25n01g_pageProgramFinish(void)
{
    if (bufferDirty && W25N01G_LINEAR_TO_COLUMN(programLoadAddress) == 0) {

        currentPage = W25N01G_LINEAR_TO_PAGE(programStartAddress); // reset page to the page being written

        w25n01g_programExecute(W25N01G_LINEAR_TO_PAGE(programStartAddress));

        bufferDirty = false;
        isProgramming = true;

        programStartAddress = programLoadAddress;
    }
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_flush
 *  @brief     : Garante que quaisquer dados pendentes na memoria buffer sejam
 *               gravados na memoria W25N01G.
 *  @param     : Nenhum
 *  @retval    : uint8_t - True (1) se gravou com sucesso, False (0) caso contrario
 * ******************************************************************************
 */
uint8_t w25n01g_flush(void)
{
    if (bufferDirty) {
        currentPage = W25N01G_LINEAR_TO_PAGE(programStartAddress); // reset page to the page being written

        w25n01g_programExecute(W25N01G_LINEAR_TO_PAGE(programStartAddress));

        bufferDirty = false;
        isProgramming = true;
    } else {
        isProgramming = false;
    }

    if ((w25n01g_readRegister(W25N01G_STAT_REG) & W25N01G_STATUS_PROGRAM_FAIL) == 0)
	{
	  return true;
	}

	DEBUG_PRINT(LOG_ERROR, "[SPI] Falha na programacao da flash (flush)\r\n");
    return false;
}

/**
 * ******************************************************************************
 *  Function Name: w25n01g_pageProgram
 *  @brief     : Programa uma pagina completa na memoria W25N01G.
 *  @param     : address - Endereco inicial da pagina a ser programada.
 *               data - Ponteiro para os dados a serem programados.
 *               length - Comprimento dos dados em bytes.
 *  @retval    :uint8_t - True (1) se gravou com sucesso, False (0) caso contrario
 * ******************************************************************************
 */
uint8_t w25n01g_pageProgram(uint32_t address, uint8_t *data, int length)
{
    w25n01g_pageProgramBegin(address);
    w25n01g_pageProgramContinue(data, length);
    w25n01g_pageProgramFinish();

    if ((w25n01g_readRegister(W25N01G_STAT_REG) & W25N01G_STATUS_PROGRAM_FAIL) == 0)
	{
	  return true;
	}

	DEBUG_PRINT(LOG_ERROR, "[SPI] Falha na programacao da pagina (addr=0x%08lX)\r\n", address);
	return false;
}


/**
 * ******************************************************************************
 *  Function Name: w25n01g_readBytes
 *  @brief       : Le dados da memoria flash
 *  @param       : address - Endereco linear inicial de leitura
 *                 buffer - Buffer para armazenar os dados lidos
 *                 length - Numero de bytes a serem lidos
 *  @retval      : FLASHTypeDef - Flash type
 * ******************************************************************************
 */
FLASHTypeDef w25n01g_readBytes(uint32_t address, uint8_t *buffer, int length)
{
	FLASHTypeDef read_report;
	uint32_t targetPage = W25N01G_LINEAR_TO_PAGE(address);

	read_report.load_data = 0;

    if (currentPage != targetPage) {
        if (!w25n01g_waitForReady(TIMER_100ms)) {
        	read_report.error = true;
			return read_report;
        }

        currentPage = UINT32_MAX;

        w25n01g_performCommandWithPageAddress(W25N01G_INSTRUCTION_PAGE_DATA_READ, targetPage);

        w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_READ_MS);
        if (!w25n01g_waitForReady(TIMER_100ms)) {
        	read_report.error = true;
			return read_report;
        }

        currentPage = targetPage;
    }

    uint16_t column = address % W25N01G_PAGE_SIZE;
    uint16_t transferLength;

    if (length > W25N01G_PAGE_SIZE - column) {
        transferLength = W25N01G_PAGE_SIZE - column;
    } else {
        transferLength = length;
    }

	uint8_t cmd[4];
	cmd[0] = W25N01G_INSTRUCTION_READ_DATA;
	cmd[1] = (column >> 8) & 0xff;
	cmd[2] = (column >> 0) & 0xff;
	cmd[3] = 0;

	W25N01G_CSP(0);
	w25n01g_transmit(cmd, sizeof(cmd), 100);
	w25n01g_receive(buffer, length, 1000); //todo: Verificar se aqui não deveria ser transferLength invés de length
	W25N01G_CSP(1);

    w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_READ_MS);
    if (!w25n01g_waitForReady(TIMER_100ms)) {
    	read_report.error = true;
        return read_report;
    }

	#if ENABLE_SABOTAGE_FLASH
    flash_fault_corrupt_read(buffer, length);
    #endif

    /* Check ECC */
    uint8_t statReg = w25n01g_readRegister(W25N01G_STAT_REG);
    uint8_t eccCode = W25N01G_STATUS_FLAG_ECC(statReg);

    switch (eccCode) {
    case 0: // Successful read, no ECC correction
    case 1: // Successful read with ECC correction
    	read_report.corrupted = false;
		read_report.error = false;
        break;
    case 2: // Uncorrectable ECC in a single page
    case 3: // Uncorrectable ECC in multiple pages
        w25n01g_deviceReset();
        DEBUG_PRINT(LOG_ERROR, "[SPI] ECC nao corrigivel na leitura da flash (addr=0x%08lX, ecc=%d)\r\n", address, eccCode);
        read_report.corrupted = true;
        read_report.error = true;
        break;
    }

    read_report.load_data = transferLength;

    return read_report;
}
