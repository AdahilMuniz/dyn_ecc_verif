/**
 * ******************************************************************************
 *  @file    : NASCERR_InternalFlash.c
 *  @brief   : Biblioteca para gravar dados na Flash interna do STM
 *  @author  : Joao Manoel
 *  @note    :
 *
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */
#include "NASCERR_InternalFlash.h"
#include "NASCERR_Bootloader.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

extern firmware_metadata_t *firmware_metadata_flash; 	// Ponteiro para a estrutura de metadados na flash
uint32_t log_address; 									// Endereco inicial para logs
uint8_t log_buffer[BUFFER_BLOCK_SIZE] = {0}; 			// Buffer para logs
uint8_t log_buffer_index = 0; 							// Indice do buffer de logs

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

uint8_t nascerr_flash_write_logs(uint8_t *data, uint32_t size);

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_log_append
 *  @brief       : Anexa dados ao buffer de log e grava na memoria flash externa se necessario.
 *  @param       : data – Ponteiro para os dados a serem anexados.
 *  @param       : size – Tamanho dos dados em bytes.
 *  @retval      : -1 se o buffer estiver cheio; exige gravação na flash externa.
 *                 1 se a operação de escrita for bem-sucedida.
 *                 0 se o buffer estiver cheio ou ocorrer falha na escrita.
 * ******************************************************************************
 */
uint8_t nascerr_flash_log_append(uint8_t *data, uint32_t size)
{
    uint32_t occupied = nascerr_flash_get_log_occupied_size();
    uint32_t free_space = EXTERNAL_FLASH_PAGE_SIZE - occupied;
    if (free_space < BUFFER_BLOCK_SIZE) {
        return -1;
    }

    uint32_t copied = 0;
    while (copied < size) {
        uint32_t space   = BUFFER_BLOCK_SIZE - log_buffer_index;
        uint32_t to_copy = (size - copied < space)
                            ? (size - copied)
                            : space;

        memcpy(&log_buffer[log_buffer_index], &data[copied], to_copy);
        log_buffer_index += to_copy;
        copied          += to_copy;

        if (log_buffer_index == BUFFER_BLOCK_SIZE) {
        	vTaskSuspendAll();
            uint8_t ret = nascerr_flash_write_logs(log_buffer, BUFFER_BLOCK_SIZE);
            xTaskResumeAll();
            if (ret != true)
            {
                return ret;
            }
            log_buffer_index = 0;
        }
    }

    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_init_log_buffer
 *  @brief       : Reseta o buffer de log na memoria flash.
 *                 Esta funcao apaga a regiao de flash onde os logs
 *                 sao armazenados e redefine o endereco de gravacao
 *                 para o inicio do buffer.
 *  @param       : none
 *  @retval      : 1 se o buffer de log for iniciado com sucesso.
 *                 0 se o apagamento da flash falhar.
 * ******************************************************************************
 */
uint8_t nascerr_flash_init_log_buffer(void)
{
	log_address = firmware_metadata_flash->end_address_log;
	return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_reset_log_buffer
 *  @brief       : Reseta o buffer de log na memoria flash.
 *                 Esta funcao apaga a regiao de flash onde os logs
 *                 sao armazenados e redefine o endereco de gravacao
 *                 para o inicio do buffer.
 *  @param       : none
 *  @retval      : 1 se o buffer de log for reiniciado com sucesso.
 *                 0 se o apagamento da flash falhar.
 * ******************************************************************************
 */
uint8_t nascerr_flash_reset_log_buffer(void)
{
	log_address = FLASH_LOG_START_ADDRESS;
	return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_get_log_occupied_size
 *  @brief       : Obtem o tamanho atual do buffer de log usado na memaria flash.
 *                 Esta funcao calcula quantos bytes do buffer de log
 *                 foram ocupados na memoria flash interna desde o endereco inicial.
 *  @param       : none
 *  @retval      : uint16_t – Tamanho do buffer de log ocupado em bytes.
 * ******************************************************************************
 */
uint16_t nascerr_flash_get_log_occupied_size(void)
{
	return log_address - FLASH_LOG_START_ADDRESS;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_get_log_address
 *  @brief       : Retorna o ponteiro para o inicio do buffer de log na memoria flash.
 *                 Esta funcao fornece o endereco base onde os logs
 *                 começam a ser armazenados na flash.
 *  @param       : none
 *  @retval      : uint8_t* – Ponteiro para o endereco inicial do buffer de log.
 * ******************************************************************************
 */
uint8_t *nascerr_flash_get_log_address(void)
{
	uint8_t *log_address_ptr = (uint8_t *)FLASH_LOG_START_ADDRESS;
	return log_address_ptr;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_write_words
 *  @brief       : Escreve multiplas palavras de 32 bits na memoria flash do STM32.
 *                 Garante que o endereco esta dentro dos limites validos da flash
 *                 e que esteja alinhado a 32 bits.
 *  @param       : address – Endereco inicial para escrita (deve ser multiplo de 4).
 *  @param       : data    – Ponteiro para o buffer de dados (palavras de 32 bits).
 *  @param       : size    – Número de palavras de 32 bits a serem escritas.
 *  @retval      : 1       – Operacao de escrita bem-sucedida.
 *  @retval      : 0       – Endereco invalido ou falha na escrita.
 *  @note        : A regiao de flash deve estar previamente apagada antes de escrever.
 * ******************************************************************************
 */
uint8_t nascerr_flash_write_words(uint32_t address, uint32_t *data, uint32_t size)
{
	if (address < FLASH_START_ADDRESS || (address + (size * 4)) > FLASH_END_ADDRESS)
	{
		return false;
	}

	uint64_t data_buf[(size + 1) / 2]; // Vetor de 64 bits
	for (uint32_t i = 0; i < size / 2; i++)
	{
		data_buf[i] = ((uint64_t)data[2 * i + 1] << 32) | data[2 * i]; // Junta duas palavras de 32 bits em 64 bits
	}

	// Caso o numero de palavras seja impar, processa a ultima palavra
	if (size % 2 != 0)
	{
		data_buf[size / 2] = ((uint64_t)0xFFFFFFFF << 32) | data[size - 1];
	}

	memcpy(data_buf, data, sizeof(data_buf)/8);

	for(uint32_t i = 0; i<(sizeof(data_buf)/8); i++)
	{
		if(nascerr_flash_write_doubleword(address, data_buf[i])!=HAL_OK)
		{
			break;
		}
		address += 8;
	}

	return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_write_bytes
 *  @brief       : Escreve um bloco de bytes na memoria flash do STM32.
 *                 Esta funcao grava um numero especificado de bytes na memoria flash
 *                 no endereco informado. Trata alinhamento e padding para garantir
 *                 operacoes corretas de escrita na flash.
 *  @param       : address – Endereco inicial para gravacao (deve estar alinhado a 64 bits).
 *  @param       : data    – Ponteiro para o buffer de dados.
 *  @param       : size    – Tamanho dos dados em bytes.
 *  @retval      : 1       – Operacao de escrita bem-sucedida.
 *  @retval      : 0       – Endereco invalido ou falha na escrita.
 * ******************************************************************************
 */
uint8_t nascerr_flash_write_bytes(uint32_t address, uint8_t *data, uint32_t size)
{
    if (address < FLASH_START_ADDRESS || (address + size) > FLASH_END_ADDRESS)
    {
        return false;
    }

    uint32_t word32_buf[(size + 3) / 4];

    for (uint32_t i = 0; i < size; i++)
    {
        ((uint8_t *)word32_buf)[i] = data[i];
    }

    for (uint32_t i = size; i < ((size + 3) / 4) * 4; i++)
    {
        ((uint8_t *)word32_buf)[i] = 0xFF;
    }

    uint64_t word64_buf[(size + 7) / 8];

    for (uint32_t i = 0; i < ((size + 3) / 4) / 2; i++)
    {
        word64_buf[i] = ((uint64_t)word32_buf[2 * i + 1] << 32) | word32_buf[2 * i];
    }

    if (((size + 3) / 4) % 2 != 0)
    {
        word64_buf[((size + 3) / 4) / 2] = ((uint64_t)0xFFFFFFFF << 32) | word32_buf[((size + 3) / 4) - 1];
    }

    for (uint32_t i = 0; i < ((size + 7) / 8); i++)
    {
        if (!nascerr_flash_write_doubleword(address, word64_buf[i]))
        {
            return false;
        }
        address += 8;
    }

    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_write_doubleword
 *  @brief       : Escreve uma double word (64 bits) na memoria flash do STM32.
 *                 Esta funcao grava um valor de 64 bits no endereco especificado
 *                 da memoria flash. Desbloqueia a flash antes da escrita e
 *                 bloqueia novamente apos a operacao.
 *  @param       : address – Endereco inicial para gravacao (deve estar alinhado a 64 bits).
 *  @param       : data    – Valor de 64 bits a ser escrito na flash.
 *  @retval      : 1       – Operacao de escrita bem-sucedida.
 *  @retval      : 0       – Falha na escrita.
 * ******************************************************************************
 */
uint8_t nascerr_flash_write_doubleword(uint32_t address, uint64_t data) {
    HAL_StatusTypeDef status;

    if (address & 0x7) {
        return 0;
    }


    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS | FLASH_FLAG_EOP);

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);

    HAL_FLASH_Lock();
    HAL_FLASH_OB_Lock();

    return (status == HAL_OK) ? 1 : 0;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_read
 *  @brief       : Le um bloco de dados da memoria flash do STM32.
 *                 Esta funcao le um numero especificado de palavras de 32 bits
 *                 a partir do endereco informado na memoria flash.
 *  @param       : address – Endereco inicial para leitura (deve estar alinhado a 32 bits).
 *  @param       : data    – Ponteiro para o buffer onde os dados lidos serao armazenados.
 *  @param       : size    – Numero de palavras de 32 bits a serem lidas.
 *  @retval      : none
 * ******************************************************************************
 */
void nascerr_flash_read(uint32_t address, uint32_t *data, uint32_t size){

	for(uint32_t i=0; i<size; i++)
	{
		data[i] = *(uint32_t*)(address + (i*4));
	}

}

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_erase_pages
 *  @brief       : Apaga um numero especifico de paginas de flash a partir de um endereco.
 *                 Esta funcao apaga paginas de memoria flash do STM32, tratando ambos os bancos
 *                 e assegurando alinhamento adequado e contagem de paginas.
 *  @param       : address – Endereco inicial para apagamento (deve estar alinhado ao tamanho de pagina).
 *  @param       : pages   – Numero de paginas a apagar.
 *  @retval      : 1       – Operacao de apagamento bem sucedida.
 *  @retval      : 0       – Falha no apagamento.
 * ******************************************************************************
 */
uint8_t nascerr_flash_erase_pages(uint32_t address, uint32_t pages) {
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError;
    uint32_t current_address = address;
    uint32_t remaining_pages = pages;

    const uint32_t MAX_PAGES_PER_ERASE = 32;

    while (remaining_pages > 0) {
        uint32_t bank, first_page, pages_in_bank;

        uint32_t pages_to_erase = (remaining_pages > MAX_PAGES_PER_ERASE) ? MAX_PAGES_PER_ERASE : remaining_pages;

        if (current_address >= FLASH_BANK2_BASE) {
            bank = FLASH_BANK_2;
            first_page = (current_address - FLASH_BANK2_BASE) / PAGE_SIZE;
            uint32_t max_pages_bank = (FLASH_END - FLASH_BANK2_BASE + 1) / PAGE_SIZE;
            pages_in_bank = MIN(pages_to_erase, max_pages_bank - first_page);
        } else {
            bank = FLASH_BANK_1;
            first_page = (current_address - FLASH_BANK1_BASE) / PAGE_SIZE;
            uint32_t max_pages_bank = (FLASH_BANK2_BASE - FLASH_BANK1_BASE) / PAGE_SIZE;
            pages_in_bank = MIN(pages_to_erase, max_pages_bank - first_page);
        }

        EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
        EraseInitStruct.Banks = bank;
        EraseInitStruct.Page = first_page;
        EraseInitStruct.NbPages = pages_in_bank;

//        printf("Erasing %lu pages at address 0x%08lX (Bank %lu, First Page %lu)...\r\n",
//			   pages_in_bank, current_address, bank, first_page);
        HAL_FLASH_Unlock();
        HAL_FLASH_OB_Unlock();
        __disable_irq();
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

        if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
            __enable_irq();
            HAL_FLASH_Lock();
            HAL_FLASH_OB_Lock();
            return false;
        }
//        printf("Erase completed successfully.\r\n");
        i2c_slave_noBlockBus();
        __enable_irq();
        HAL_FLASH_Lock();
        HAL_FLASH_OB_Lock();

        current_address += pages_in_bank * PAGE_SIZE;
        remaining_pages -= pages_in_bank;
    }

    return true;
}

/*
 * ******************************************************************************
 *  Local Functions Definitions
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: nascerr_flash_write_logs
 *  @brief       : Escreve logs na memoria flash interna.
 *                 Esta funcao grava um bloco de dados na memoria flash interna
 *                 para fins de log. Verifica se ha espaco no buffer de log e grava
 *                 os dados se possivel. Se o buffer estiver cheio, retorna codigo de erro.
 *  @param       : data – Ponteiro para os dados a serem escritos.
 *  @param       : size – Tamanho dos dados em bytes.
 *  @retval      : -1 – Buffer cheio; gravar na flash externa em vez disso.
 *                 1  – Operacao de escrita bem sucedida.
 *                 0  – Buffer cheio ou falha na escrita.
 * ******************************************************************************
 */
uint8_t nascerr_flash_write_logs(uint8_t *data, uint32_t size)
{
	if(log_address + size > EXTERNAL_FLASH_PAGE_SIZE + FLASH_LOG_START_ADDRESS)
	{
		return -1;
	}

	if(log_address == FLASH_LOG_START_ADDRESS)
	{
		if (!nascerr_flash_erase_pages(FLASH_LOG_START_ADDRESS, 1))
		{
			return false;
		}
	}

	if(!nascerr_flash_write_bytes(log_address, data, size))
	{
		return false;
	}

	log_address += size;
	nascerr_bootloader_setLogEndAddress(log_address);
	return true;
}


