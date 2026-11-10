/**
 * ******************************************************************************
 *  @file    : nascerr_db.c
 *  @brief   : Biblioteca de databank do NASCERR
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

#include "NASCERR_DB.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

HeaderTypeDef experimentHeader, newHeader;
uint32_t in_use[MAX_FLASH_REGION - 1];

///* Experiment Variables */
//static uint8_t experiment_page_buffer[FLASHEXT_PAGE_SIZE];
//static uint32_t experiment_page_index = 0;
//static uint8_t current_experiment_count = 0;
//static id_db current_experiment_id = MAX_ID_DB;
//static uint8_t page_active = 0;
//static uint32_t page_start_address = 0;
//
///* Logs Variables */
//static uint8_t log_page_buffer[FLASHEXT_PAGE_SIZE];
//static uint16_t log_page_index = 0;
//static uint16_t log_count = 0;
//static uint8_t log_page_active = 0;
//static uint32_t log_page_start_address = 0;

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

void nascerr_db_loadDefaultHeader(HeaderTypeDef *header);

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_init
 *  @brief       : Inicializa a flash e verifica a integridade do header
 *                 Caso o header não esteja configurado corretamente, inicializa com valores padrão.
 *  @param       : Nenhum
 *  @retval      : SYSTEM_STATUS - STATUS_SUCCESS se sucesso, ERR_INIT caso falhe
 * ******************************************************************************
 */
SYSTEM_STATUS nascerr_db_init(void)
{
    SYSTEM_STATUS retVal = ERR_INIT;

    /* Inicializa a flash externa */
    retVal = nascerr_flash_init();
    if (retVal != STATUS_SUCCESS) {
        DEBUG_PRINT(LOG_ERROR, "Erro ao inicializar a flash externa.\r\n");
        return retVal;
    }

    /* Verifica a integridade do header na flash */
    if (!nascerr_db_readHeader()) {
        DEBUG_PRINT(LOG_WARN, "Header nao configurado ou corrompido, inicializando valores padrao.\r\n");

        /* Inicializa com valores padrao caso o header nao seja valido */
        if (!nascerr_db_defaultHeader()) {
            DEBUG_PRINT(LOG_ERROR, "Falha ao inicializar o header padrao na flash.\r\n");
            return ERR_INIT;
        }
        nascerr_db_readHeader();
        DEBUG_PRINT(LOG_INFO, "Header inicializado com valores padrao com sucesso.\r\n");
    } else {
        DEBUG_PRINT(LOG_INFO, "Header lido com sucesso.\r\n");
    }

    return STATUS_SUCCESS;
}

/*
 * *******************************************************************************
 *  Function Name: nascerr_db_readHeader
 *  @brief       : Reads and updates the header from flash memory.
 *  @param       : None
 *  @retval      : uint8_t - Returns true if the header was successfully read,
 *                 false otherwise.
 * *******************************************************************************
 */
uint8_t nascerr_db_readHeader(void)
{
	uint16_t sizeHeader = sizeof(HeaderTypeDef);

	/* Le o header inteiro da flash */
	nascerr_flash_readAddress(W25N01G_BLOCK_TO_LINEAR(FLASH_HEADER_BLOCK), (uint8_t *)&experimentHeader, sizeHeader);

	/* Verifica os campos "in_use" de cada secao */
	if (experimentHeader.logs_data.header.in_use != FLASH_IN_USE_BYTES) {
		return false;
	}
	if (experimentHeader.scheduler_data.header.in_use != FLASH_IN_USE_BYTES) {
		return false;
	}
	if (experimentHeader.experiment_data.header.in_use != FLASH_IN_USE_BYTES) {
		return false;
	}
	if (experimentHeader.boot_data.header.in_use != FLASH_IN_USE_BYTES) {
		return false;
	}
	if (experimentHeader.image_data.header.in_use != FLASH_IN_USE_BYTES) {
		return false;
	}

	/* Verifica o CRC */
	uint16_t crc_size = sizeof(HeaderTypeDef) - sizeof(uint16_t);
	uint16_t crc_calc = calculate_crc16((uint8_t *)&experimentHeader, crc_size);

	if (crc_calc != experimentHeader.crc) return false;

	return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_defaultHeader
 *  @brief     : Carrega e grava um novo header na memoria flash.
 *  @param     : Nenhum
 *  @retval    : true se bem sucedido, false caso contrario.
 * ******************************************************************************
 */
uint8_t nascerr_db_defaultHeader(void)
{
	FLASHTypeDef retWrite;
	uint16_t sizeHeader = sizeof(HeaderTypeDef);

	if (nascerr_flash_eraseAll() != STATUS_SUCCESS) return false;

	nascerr_db_loadDefaultHeader(&newHeader);
	retWrite = nascerr_flash_writeAddress(W25N01G_BLOCK_TO_LINEAR(FLASH_HEADER_BLOCK), (uint8_t *)&newHeader, sizeHeader);

	return !nascerr_flash_flush(retWrite.end_addr).error;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_updateHeader
 *  @brief     : Atualiza a estrutura HeaderTypeDef na memoria flash, recalculando o CRC.
 *  @param     : Nenhum
 *  @retval    : uint8_t - Retorna true se a gravacao foi bem-sucedida, false caso contrario.
 * ******************************************************************************
 */
uint8_t nascerr_db_updateHeader(void)
{
    uint16_t sizeHeader = sizeof(HeaderTypeDef);

    if (nascerr_flash_eraseArea(FLASH_HEADER) != STATUS_SUCCESS) { return false; }

    /* Atualiza CRC antes de salvar */
    experimentHeader.crc = calculate_crc16((uint8_t *)&experimentHeader, sizeHeader - sizeof(uint16_t));

    /* Grava estrutura completa na flash  */
    FLASHTypeDef result = nascerr_flash_writeAddress(W25N01G_BLOCK_TO_LINEAR(FLASH_HEADER_BLOCK),
                                                     (uint8_t *)&experimentHeader,
                                                     sizeHeader);
    return !nascerr_flash_flush(result.end_addr).error;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_updateBootVersion
 *  @brief     : Atualiza o campo de versao do boot_data na estrutura HeaderTypeDef em RAM.
 *  @param     : new_version - Nova versao do firmware.
 *  @retval    : Nenhum
 * ******************************************************************************
 */
void nascerr_db_updateBootVersion(uint32_t new_version)
{
	experimentHeader.boot_data.version = new_version;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_writeBootData
 *  @brief     : Escreve dados na regiao de backup do firmware. Nao realiza flush
 *               nem atualiza o header.
 *  @param     : data - Ponteiro para os dados a serem gravados.
 *  @param     : size - Tamanho dos dados em bytes.
 *  @retval    : FLASHTypeDef - Estrutura com resultado da escrita
 *               (endereco final, erro, etc).
 * ******************************************************************************
 */
FLASHTypeDef nascerr_db_writeBootData(const uint8_t *data, uint32_t size)
{
    FLASHTypeDef result = {0};

    if (data == NULL || size == 0) {
        result.error = true;
        return result;
    }

    uint32_t start_addr = experimentHeader.boot_data.header.start_addr;
    result = nascerr_flash_writeAddress(start_addr, data, size);

    /* Atualiza a estrutura de boot_data apenas em RAM */
    experimentHeader.boot_data.header.current_addr = result.end_addr;
    experimentHeader.boot_data.header.data_load = size;
    experimentHeader.boot_data.crc = calculate_crc16(data, size);

    return result;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_readBootData
 *  @brief     : Le dados da regiao de boot com offset e tamanho definidos,
 *  			 validando limites.
 *  @param     : offset - Deslocamento em bytes a partir do endereco base de boot_data.
 *  @param     : buffer - Ponteiro para o buffer onde os dados serao armazenados.
 *  @param     : size   - Quantidade de bytes a serem lidos.
 *  @retval    : uint8_t - Retorna true se leitura for valida, false caso contrario.
 * ******************************************************************************
 */
uint8_t nascerr_db_readBootData(uint32_t offset, uint8_t *buffer, uint32_t size)
{
    if (buffer == NULL) {
        return false;
    }

    uint32_t start_addr = experimentHeader.boot_data.header.start_addr;
    uint32_t data_size  = experimentHeader.boot_data.header.data_load;

    if (offset >= data_size) {
        return false;
    }

    if ((offset + size) > data_size) {
        return false;
    }

    /* Realiza a leitura da flash e verifica erro */
    FLASHTypeDef result = nascerr_flash_readAddress(start_addr + offset, buffer, size);
    if (result.error) {
        return false;
    }

    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_clearBootArea
 *  @brief     : Apaga a regiao de boot/firmware da flash e atualiza o cabecalho
 *  			 correspondente.
 *  @param     : Nenhum
 *  @retval    : uint8_t - Retorna true se a operacao foi bem-sucedida,
 *  			 false caso contrario.
 * ******************************************************************************
 */
uint8_t nascerr_db_clearBootArea(void)
{
    if (nascerr_flash_eraseArea(FLASH_BACKUP_FIRMWARE) != STATUS_SUCCESS) {
        return false;
    }

    /* Zera os campos da estrutura boot_data em RAM */
    experimentHeader.boot_data.header.start_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_BACKUP_FIRMWARE_BLOCK);
    experimentHeader.boot_data.header.current_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_BACKUP_FIRMWARE_BLOCK);
    experimentHeader.boot_data.header.data_load = 0;
    experimentHeader.boot_data.version = 0;
    experimentHeader.boot_data.crc = 0;

    /* Atualiza o header completo na flash */
    return nascerr_db_updateHeader();
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_db_commitHeader
 *  @brief     : Recalcula o CRC do HeaderTypeDef e grava na flash (com flush).
 *  @param     : last_addr - Ultimo endereco escrito que deve ser usado no flush.
 *  @retval    : uint8_t - Retorna true se a operacao foi bem-sucedida,
 *                         false caso contrario.
 * ******************************************************************************
 */
uint8_t nascerr_db_commitHeader(uint32_t last_addr)
{
	FLASHTypeDef retVal = nascerr_flash_flush(last_addr);

    /* Realiza o flush com base no endereco final de escrita */
    if (retVal.error) {
        return false;
    }

    experimentHeader.boot_data.header.current_addr = retVal.end_addr;

    /* Atualiza o CRC e grava o header */
    return nascerr_db_updateHeader();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_updateBootSha
 *  @brief     : Atualiza o campo SHA-256 do boot_data na estrutura HeaderTypeDef em RAM.
 *  @param     : sha - Ponteiro para o buffer de 32 bytes contendo o hash SHA-256.
 *  @retval    : Nenhum
 * ******************************************************************************
 */
void nascerr_db_updateBootSha(uint8_t* sha)
{
	memcpy(experimentHeader.boot_data.hash_256, sha, 32);
}


/*
 * ******************************************************************************
 *  Local Functions Definitions
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_loadDefaultHeader
 *  @brief     : Carrega valores padrao na estrutura HeaderTypeDef.
 *  @param     : header - Ponteiro para a estrutura HeaderTypeDef a ser inicializada.
 *  @retval    : Nenhum
 * ******************************************************************************
 */
void nascerr_db_loadDefaultHeader(HeaderTypeDef *header)
{
    if (header == NULL) {
        return;
    }

    /* Inicializa os dados de logs */
    header->logs_data.header.in_use = FLASH_IN_USE_BYTES;
    header->logs_data.header.start_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_LOGS_BLOCK);
    header->logs_data.header.current_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_LOGS_BLOCK);
    header->logs_data.header.data_load = 0;
    header->logs_data.qtd_logs = 0;

    for (int i = 0; i < LOGS_PER_PAGE; i++) {
        header->logs_data.logs[i].id = MAX_ID_DB;
        header->logs_data.logs[i].num_block = 0;
        header->logs_data.logs[i].status = DB_EMPTY;
    }

    /* Inicializa os dados de agendamento (scheduler) */
    header->scheduler_data.header.in_use = FLASH_IN_USE_BYTES;
    header->scheduler_data.header.start_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_SCHEDULER_BLOCK);
    header->scheduler_data.header.current_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_SCHEDULER_BLOCK);
    header->scheduler_data.header.data_load = 0;
    header->scheduler_data.qtd_scheduling = 0;

    for (int i = 0; i < SCHEDULER_PER_PAGE; i++) {
        header->scheduler_data.scheduled[i].id = MAX_ID_DB;
        header->scheduler_data.scheduled[i].num_block = 0;
        header->scheduler_data.scheduled[i].status = DB_EMPTY;
    }

    /* Inicializa os dados de experimentos */
    header->experiment_data.header.in_use = FLASH_IN_USE_BYTES;
    header->experiment_data.header.start_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_EXPERIMENT_BLOCK);
    header->experiment_data.header.current_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_EXPERIMENT_BLOCK);
    header->experiment_data.header.data_load = 0;
    header->experiment_data.qtd_experiments = 0;

    for (int i = 0; i < EXPERIMENTS_PER_PAGE; i++) {
        header->experiment_data.experiments[i].id = MAX_ID_DB;
        header->experiment_data.experiments[i].num_block = 0;
        header->experiment_data.experiments[i].status = DB_EMPTY;
    }

    /* Inicializa os dados da imagem gravada */
    header->image_data.header.in_use = FLASH_IN_USE_BYTES;
    header->image_data.header.start_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_IMAGE_BLOCK);
    header->image_data.header.current_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_IMAGE_BLOCK);
    header->image_data.header.data_load = 0;
    header->image_data.image_size = 0;
    header->image_data.valid = 0;
    header->image_data.crc16 = 0;

    /* Inicializa os dados de boot/firmware backup */
    header->boot_data.header.in_use = FLASH_IN_USE_BYTES;
    header->boot_data.header.start_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_BACKUP_FIRMWARE_BLOCK);
    header->boot_data.header.current_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_BACKUP_FIRMWARE_BLOCK);
    header->boot_data.header.data_load = 0;
    header->boot_data.version = 0;
    header->boot_data.crc = 0;

    /* Calcula o CRC do header */
    uint16_t sizeHeader = sizeof(HeaderTypeDef) - sizeof(uint16_t);
    header->crc = calculate_crc16((uint8_t *)header, sizeHeader);
}
