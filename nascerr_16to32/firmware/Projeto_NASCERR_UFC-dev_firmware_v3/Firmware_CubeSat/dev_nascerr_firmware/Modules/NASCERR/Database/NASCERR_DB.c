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

/* Experiment Variables */
static uint8_t experiment_page_buffer[FLASHEXT_PAGE_SIZE];
static uint32_t experiment_page_index = 0;
static uint8_t current_experiment_count = 0;
static id_db current_experiment_id = MAX_ID_DB;
static uint8_t page_active = 0;
static uint32_t page_start_address = 0;

/* Logs Variables */
static uint8_t log_page_buffer[FLASHEXT_PAGE_SIZE];
static uint16_t log_page_index = 0;
static uint16_t log_count = 0;
static uint8_t log_page_active = 0;
static uint32_t log_page_start_address = 0;

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
 *  Function Name: nascerr_db_beginExperimentPage
 *  @brief     : Inicia uma nova pagina para insercao de experimentos com um ID
 *               especifico
 *  @param     : id - identificador comum para os experimentos a serem inseridos
 *  @retval    : true se a pagina foi alocada com sucesso, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_beginExperimentPage(id_db id)
{
    if (page_active) return false; // Ja em uso

    memset(experiment_page_buffer, 0xFF, FLASHEXT_PAGE_SIZE);
    experiment_page_index = 0;
    current_experiment_count = 0;
    current_experiment_id = id;
    page_active = 1;

    page_start_address = experimentHeader.experiment_data.header.current_addr;

    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_addExperiment
 *  @brief     : Adiciona um novo experimento a pagina atual
 *  @param     : exp - ponteiro para estrutura do experimento
 *  @retval    : true se inserido com sucesso, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_addExperiment(const NASCERR_EXPERIMENT *exp)
{
    if (!page_active || exp == NULL) return false;
    if (current_experiment_count >= MAX_EXPERIMENTS_PER_PAGE) return false;
    if (exp->id != current_experiment_id) return false;

    memcpy(&experiment_page_buffer[experiment_page_index], exp, sizeof(NASCERR_EXPERIMENT));
    experiment_page_index += sizeof(NASCERR_EXPERIMENT);
    current_experiment_count++;

    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_commitExperimentPage
 *  @brief     : Finaliza a pagina e grava metadados
 *  @param     : None
 *  @retval    : true se a pagina foi finalizada e gravada com sucesso,
 *  			 false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_commitExperimentPage(void)
{
    if (!page_active || current_experiment_count == 0) return false;

    /* Preenche o restante da pagina com 0xFF se for incompleta (exceto CRC) */
    if (experiment_page_index < CRC_AREA_OFFSET) {
        memset(&experiment_page_buffer[experiment_page_index], 0xFF, CRC_AREA_OFFSET - experiment_page_index);
    }

    /* Calcula CRC da pagina ate o offset reservado para o CRC */
    uint16_t crc = calculate_crc16(experiment_page_buffer, CRC_AREA_OFFSET);
    experiment_page_buffer[CRC_AREA_OFFSET]     = (uint8_t)(crc >> 8);
    experiment_page_buffer[CRC_AREA_OFFSET + 1] = (uint8_t)(crc & 0xFF);

    FLASHTypeDef result = nascerr_flash_writeAddress(page_start_address, experiment_page_buffer, FLASHEXT_PAGE_SIZE);
    if (result.error) return false;

    /* Atualiza dados do header */
    experimentHeader.experiment_data.experiments[experimentHeader.experiment_data.qtd_experiments].id = current_experiment_id;
    experimentHeader.experiment_data.experiments[experimentHeader.experiment_data.qtd_experiments].num_block = current_experiment_count;
    experimentHeader.experiment_data.experiments[experimentHeader.experiment_data.qtd_experiments].status = DB_NOT_READ;
    experimentHeader.experiment_data.qtd_experiments++;

    experimentHeader.experiment_data.header.current_addr += FLASHEXT_PAGE_SIZE;
    experimentHeader.experiment_data.header.data_load += FLASHEXT_PAGE_SIZE;

    /* Finaliza */
    page_active = 0;
    return nascerr_db_updateHeader();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_listAllExperiments
 *  @brief     : Lista todos os experimentos gravados na memoria
 *  @retval    : Ponteiro para lista estatica de DBItem
 * ******************************************************************************
 */
const DBItem* nascerr_db_listAllExperiments(uint8_t *count)
{
    if (count) *count = experimentHeader.experiment_data.qtd_experiments;
    return experimentHeader.experiment_data.experiments;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_listUnreadExperimentIDs
 *  @brief     : Retorna apenas os IDs dos experimentos nao lidos a partir de um offset
 *  @param     : buffer - ponteiro para armazenamento dos IDs
 *               max - numero maximo de IDs a retornar
 *               offset - posicao inicial para varredura
 *  @retval    : Quantidade de IDs preenchidos no buffer fornecido
 * ******************************************************************************
 */
uint8_t nascerr_db_listUnreadExperimentIDs(id_db *buffer, uint8_t max, uint8_t offset)
{
    uint8_t count = 0;
    for (uint8_t i = offset; i < experimentHeader.experiment_data.qtd_experiments; i++) {
        if (experimentHeader.experiment_data.experiments[i].status == DB_NOT_READ) {
            if (count < max) {
                buffer[count++] = experimentHeader.experiment_data.experiments[i].id;
            } else {
                break;
            }
        }
    }
    return count;
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
 *  Function Name: nascerr_db_clearExperimentArea
 *  @brief     : Limpa toda a area de experimentos na flash e zera estrutura em RAM
 *  @retval    : 1 se sucesso, 0 em caso de erro
 * ******************************************************************************
 */
uint8_t nascerr_db_clearExperimentArea(void)
{
    if (nascerr_flash_eraseArea(FLASH_EXPERIMENT) != STATUS_SUCCESS) {
        return false;
    }

    /* Zera os campos da estrutura experiment_data em RAM */
    experimentHeader.experiment_data.header.current_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_EXPERIMENT_BLOCK);
    experimentHeader.experiment_data.header.data_load = 0;
    experimentHeader.experiment_data.qtd_experiments = 0;

    for (int i = 0; i < EXPERIMENTS_PER_PAGE; i++) {
    	experimentHeader.experiment_data.experiments[i].id = MAX_ID_DB;
    	experimentHeader.experiment_data.experiments[i].num_block = 0;
    	experimentHeader.experiment_data.experiments[i].status = DB_EMPTY;
	}

    /* Atualiza o header completo na flash */
    return nascerr_db_updateHeader();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_readExperimentPage
 *  @brief     : Le uma pagina de experimentos da memoria e valida CRC
 *  @param     : index - indice no vetor de experimentos
 *               buffer - ponteiro para armazenamento dos experimentos lidos
 *  @retval    : true se leitura e validacao bem-sucedidas, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_readExperimentPage(uint8_t index, NASCERR_EXPERIMENT *buffer)
{
    if (index >= experimentHeader.experiment_data.qtd_experiments) return false;

    DBItem *item = &experimentHeader.experiment_data.experiments[index];
    uint32_t addr = experimentHeader.experiment_data.header.start_addr + (index * FLASHEXT_PAGE_SIZE);

    FLASHTypeDef result = nascerr_flash_readAddress(addr, experiment_page_buffer, FLASHEXT_PAGE_SIZE);
    if (result.error) return false;

    uint16_t crc_stored = ((uint16_t)experiment_page_buffer[CRC_AREA_OFFSET] << 8) |
                           (uint16_t)experiment_page_buffer[CRC_AREA_OFFSET + 1];

    uint16_t crc_calc = calculate_crc16(experiment_page_buffer, CRC_AREA_OFFSET);
    if (crc_calc != crc_stored) return false;

    memcpy(buffer, experiment_page_buffer, item->num_block * sizeof(NASCERR_EXPERIMENT));
    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_setReadExperimentPage
 *  @brief     : Marca experimento como lido no banco e atualiza o header
 *  @param[in] : index - indice do experimento a ser marcado como lido
 *  @retval    : true se operacao bem-sucedida, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_setReadExperimentPage(uint8_t index)
{
    if (index >= experimentHeader.experiment_data.qtd_experiments) return false;

    experimentHeader.experiment_data.experiments[index].status = DB_READ;
    nascerr_db_updateHeader();
    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_readExperimentRange
 *  @brief     : Le um intervalo de experimentos de uma pagina valida
 *  @param     : index - indice da pagina
 *               offset - deslocamento dentro da pagina (0 a MAX_EXPERIMENTS_PER_PAGE-1)
 *               qtd - quantidade de experimentos desejados
 *               buffer - destino dos experimentos lidos
 *  @retval    : true se leitura bem-sucedida, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_readExperimentRange(uint8_t index, uint8_t offset, uint8_t qtd, NASCERR_EXPERIMENT *buffer)
{
    if (index >= experimentHeader.experiment_data.qtd_experiments) return false;
    if (offset >= MAX_EXPERIMENTS_PER_PAGE || qtd == 0) return false;

    DBItem *item = &experimentHeader.experiment_data.experiments[index];
    if (offset + qtd > item->num_block) return false;

    uint32_t addr = experimentHeader.experiment_data.header.start_addr + (index * FLASHEXT_PAGE_SIZE);
    FLASHTypeDef result = nascerr_flash_readAddress(addr, experiment_page_buffer, FLASHEXT_PAGE_SIZE);
    if (result.error) return false;

    uint16_t crc_stored = ((uint16_t)experiment_page_buffer[CRC_AREA_OFFSET] << 8) |
                           (uint16_t)experiment_page_buffer[CRC_AREA_OFFSET + 1];

    uint16_t crc_calc = calculate_crc16(experiment_page_buffer, CRC_AREA_OFFSET);
    if (crc_calc != crc_stored) return false;

    memcpy(buffer,
           &experiment_page_buffer[offset * sizeof(NASCERR_EXPERIMENT)],
           qtd * sizeof(NASCERR_EXPERIMENT));

    item->status = DB_READ;
    nascerr_db_updateHeader();

    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_beginLogPage
 *  @brief     : Inicia uma nova pagina de logs para insercao de operacoes
 *  @param     : Nenhum
 *  @retval    : true se a pagina foi iniciada com sucesso, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_beginLogPage(void)
{
    if (log_page_active) {
        return false;  // Ja ha uma pagina ativa
    }

    memset(log_page_buffer, 0xFF, FLASHEXT_PAGE_SIZE);  // Limpa o buffer com 0xFF
    log_page_index = 0;
    log_count = 0;
    log_page_active = 1;

    log_page_start_address = experimentHeader.logs_data.header.current_addr;

    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_addLog
 *  @brief     : Adiciona um novo log na pagina de logs em RAM
 *  @param     : log - Ponteiro para a estrutura NASCERR_LOG a ser inserida
 *  @retval    : true se inserido com sucesso, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_addLog(const NASCERR_LOG *log)
{
    if (!log_page_active || log == NULL) {
        return false;
    }

    if (log_count >= MAX_LOGS_PER_PAGE) {
        return false; // Limite de logs por pagina atingido
    }

    /* Verifica se ainda ha espaco na pagina */
    if ((log_page_index + sizeof(NASCERR_LOG) + CRC_AREA_SIZE) > FLASHEXT_PAGE_SIZE) {
        return false;  // Nao ha espaco suficiente
    }

    /* Copia o log para o buffer */
    memcpy(&log_page_buffer[log_page_index], log, sizeof(NASCERR_LOG));
    log_page_index += sizeof(NASCERR_LOG);
    log_count++;

    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_commitLogPage
 *  @brief     : Finaliza a pagina de logs, grava na flash e atualiza o header
 *  @param     : Nenhum
 *  @retval    : true se gravado com sucesso, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_commitLogPage(void)
{
    if (!log_page_active || log_count == 0) {
        return false;
    }

    /* Preenche com 0xFF ate a posicao do CRC se necessario */
    if (log_page_index < CRC_AREA_OFFSET) {
        memset(&log_page_buffer[log_page_index], 0xFF, CRC_AREA_OFFSET - log_page_index);
    }

    /* Calcula e insere o CRC no final da pagina */
    uint16_t crc = calculate_crc16(log_page_buffer, CRC_AREA_OFFSET);
    log_page_buffer[CRC_AREA_OFFSET]     = (uint8_t)(crc >> 8);
    log_page_buffer[CRC_AREA_OFFSET + 1] = (uint8_t)(crc & 0xFF);

    /* Grava a pagina na flash */
    FLASHTypeDef result = nascerr_flash_writeAddress(log_page_start_address, log_page_buffer, FLASHEXT_PAGE_SIZE);
    if (result.error) {
        return false;
    }

    if (experimentHeader.logs_data.qtd_logs >= MAX_LOGS_PER_PAGE) {
        return false;
    }

    /* Atualiza estrutura logs_data no header */
    experimentHeader.logs_data.logs[experimentHeader.logs_data.qtd_logs].id = g_register_data[LOG_SET_PAGE].value[0];  // id como numero de logs
    experimentHeader.logs_data.logs[experimentHeader.logs_data.qtd_logs].num_block = log_count;
    experimentHeader.logs_data.logs[experimentHeader.logs_data.qtd_logs].status = DB_NOT_READ;
    experimentHeader.logs_data.qtd_logs++;

    experimentHeader.logs_data.header.current_addr += FLASHEXT_PAGE_SIZE;
    experimentHeader.logs_data.header.data_load += FLASHEXT_PAGE_SIZE;

    log_page_active = 0;

    return nascerr_db_updateHeader();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_readLogPage
 *  @brief     : Le uma pagina de logs da flash, valida o CRC e copia os logs
 *  @param     : index  - Indice da pagina (logs_data.logs[index])
 *               buffer - Ponteiro para o buffer de destino (array de LogOperation)
 *               count  - Ponteiro para variavel onde sera armazenado o numero de logs lidos
 *  @retval    : true se leitura e validacao do CRC foram bem-sucedidas, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_readLogPage(uint8_t index, NASCERR_LOG *buffer, uint16_t *count)
{
    if (index >= experimentHeader.logs_data.qtd_logs || buffer == NULL || count == NULL) {
        return false;
    }

    DBItem *item = &experimentHeader.logs_data.logs[index];
    uint32_t addr = experimentHeader.logs_data.header.start_addr + (index * FLASHEXT_PAGE_SIZE);


    /* Le a pagina completa da flash */
    FLASHTypeDef result = nascerr_flash_readAddress(addr, log_page_buffer, FLASHEXT_PAGE_SIZE);
    if (result.error) {
        return false;
    }

    /* Verifica o CRC */
    uint16_t crc_stored = ((uint16_t)log_page_buffer[CRC_AREA_OFFSET] << 8) |
                           (uint16_t)log_page_buffer[CRC_AREA_OFFSET + 1];

    uint16_t crc_calc = calculate_crc16(log_page_buffer, CRC_AREA_OFFSET);
    if (crc_calc != crc_stored) {
        return false;
    }

    /* Copia os logs para o buffer de destino */
    uint16_t qtd = item->num_block;
    if (qtd > MAX_LOGS_PER_PAGE||(qtd * sizeof(NASCERR_LOG)) > CRC_AREA_OFFSET) {
        return false;
    }

    memcpy(buffer, log_page_buffer, qtd * sizeof(NASCERR_LOG));
    *count = qtd;

//    item->status = DB_READ;
//    return nascerr_db_updateHeader();
    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_clearLogArea
 *  @brief     : Apaga a regiao de logs da flash e reinicializa a estrutura logs_data em RAM
 *  @param     : Nenhum
 *  @retval    : true se a operacao foi bem-sucedida, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_clearLogArea(void)
{
    if (nascerr_flash_eraseArea(FLASH_LOGS) != STATUS_SUCCESS) {
        return false;
    }

    /* Zera os campos da estrutura logs_data em RAM */
    experimentHeader.logs_data.header.current_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_LOGS_BLOCK);
    experimentHeader.logs_data.header.data_load = 0;
    experimentHeader.logs_data.qtd_logs = 0;

    for (int i = 0; i < LOGS_PER_PAGE; i++) {
        experimentHeader.logs_data.logs[i].id = MAX_ID_DB;
        experimentHeader.logs_data.logs[i].num_block = 0;
        experimentHeader.logs_data.logs[i].status = DB_EMPTY;
    }

    /* Atualiza o header completo na flash */
    return nascerr_db_updateHeader();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_writeScheduler
 *  @brief     : Grava um novo comando de agendamento em uma pagina da flash
 *  @param     : sched - Ponteiro para a estrutura NASCERR_SCHEDULER a ser gravada
 *  @retval    : true se a operacao foi bem-sucedida, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_writeScheduler(const NASCERR_SCHEDULER *sched)
{
    if (sched == NULL) {
        return false;
    }

    if (experimentHeader.scheduler_data.qtd_scheduling >= MAX_SCHEDULER) {
        return false; // Todas as paginas foram utilizadas
    }

    uint8_t page_buffer[FLASHEXT_PAGE_SIZE];
    memset(page_buffer, 0xFF, sizeof(page_buffer));

    /* Copia o scheduler para o inicio da pagina */
    memcpy(page_buffer, sched, sizeof(NASCERR_SCHEDULER));

    /* Calcula e insere o CRC no final da pagina */
    uint16_t crc = calculate_crc16(page_buffer, CRC_AREA_OFFSET);
    page_buffer[CRC_AREA_OFFSET]     = (uint8_t)(crc >> 8);
    page_buffer[CRC_AREA_OFFSET + 1] = (uint8_t)(crc & 0xFF);

    /* Calcula endereco da nova pagina */
    uint32_t addr = W25N01G_BLOCK_TO_LINEAR(FLASH_SCHEDULER_BLOCK) +
                    (experimentHeader.scheduler_data.qtd_scheduling * FLASHEXT_PAGE_SIZE);

    /* Grava na flash */
    FLASHTypeDef result = nascerr_flash_writeAddress(addr, page_buffer, FLASHEXT_PAGE_SIZE);
    if (result.error) {
        return false;
    }

    /* Atualiza metadados */
    DBItem *item = &experimentHeader.scheduler_data.scheduled[experimentHeader.scheduler_data.qtd_scheduling];
    item->id = sched->id;
    item->num_block = 0;
    item->status = DB_NEW_COMMAND;

    experimentHeader.scheduler_data.qtd_scheduling++;

    return nascerr_db_updateHeader();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_updateScheduler
 *  @brief     : Grava comando de agendamento atualizado em uma pagina da flash
 *  @param     : sched - Ponteiro para a estrutura NASCERR_SCHEDULER a ser re-gravada
 *  @retval    : true se a operacao foi bem-sucedida, false caso contrario
 * ******************************************************************************
 */

uint8_t nascerr_db_updateScheduler(const NASCERR_SCHEDULER *sched)
{
    if (sched == NULL) {
        return false;
    }

    uint8_t page_buffer[FLASHEXT_PAGE_SIZE];
    memset(page_buffer, 0xFF, sizeof(page_buffer));

    /* Copia o scheduler para o inicio da pagina */
    memcpy(page_buffer, sched, sizeof(NASCERR_SCHEDULER));

    /* Calcula e insere o CRC no final da pagina */
    uint16_t crc = calculate_crc16(page_buffer, CRC_AREA_OFFSET);
    page_buffer[CRC_AREA_OFFSET]     = (uint8_t)(crc >> 8);
    page_buffer[CRC_AREA_OFFSET + 1] = (uint8_t)(crc & 0xFF);

    /* Calcula endereco da nova pagina */
    uint32_t addr = W25N01G_BLOCK_TO_LINEAR(FLASH_SCHEDULER_BLOCK) +
                    (experimentHeader.scheduler_data.qtd_scheduling * FLASHEXT_PAGE_SIZE);

    /* Grava na flash */
    FLASHTypeDef result = nascerr_flash_writeAddress(addr, page_buffer, FLASHEXT_PAGE_SIZE);
    if (result.error) {
        return false;
    }

    return true;
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_db_readScheduler
 *  @brief     : Le um comando de agendamento armazenado em uma pagina da flash
 *  @param     : index - Indice da pagina de agendamento (0 a MAX_SCHEDULER - 1)
 *               dest  - Ponteiro para a estrutura de destino (saida)
 *  @retval    : true se leitura e validacao foram bem-sucedidas, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_readScheduler(uint8_t index, NASCERR_SCHEDULER *dest)
{
    if (index >= experimentHeader.scheduler_data.qtd_scheduling || dest == NULL) {
        return false;
    }

    uint8_t page_buffer[FLASHEXT_PAGE_SIZE];

    /* Calcula endereco da pagina */
    uint32_t addr = W25N01G_BLOCK_TO_LINEAR(FLASH_SCHEDULER_BLOCK) +
                    (index * FLASHEXT_PAGE_SIZE);

    /* Le a pagina da flash */
    FLASHTypeDef result = nascerr_flash_readAddress(addr, page_buffer, FLASHEXT_PAGE_SIZE);
    if (result.error) {
        return false;
    }

    /* Valida o CRC */
    uint16_t crc_stored = ((uint16_t)page_buffer[CRC_AREA_OFFSET] << 8) |
                           (uint16_t)page_buffer[CRC_AREA_OFFSET + 1];
    uint16_t crc_calc = calculate_crc16(page_buffer, CRC_AREA_OFFSET);

    if (crc_stored != crc_calc) {
        return false;
    }

    /* Copia o conteudo para a estrutura de destino */
    memcpy(dest, page_buffer, sizeof(NASCERR_SCHEDULER));

    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_clearSchedulerArea
 *  @brief     : Apaga a regiao de agendamento da flash e reinicializa o header
 *  @param     : Nenhum
 *  @retval    : true se a operacao foi bem-sucedida, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_db_clearSchedulerArea(void)
{
    if (nascerr_flash_eraseArea(FLASH_SCHEDULER) != STATUS_SUCCESS) {
        return false;
    }

    /* Zera os campos do scheduler em RAM */
    experimentHeader.scheduler_data.qtd_scheduling = 0;

    for (uint16_t i = 0; i < SCHEDULER_PER_PAGE; i++) {
        experimentHeader.scheduler_data.scheduled[i].id = MAX_ID_DB;
        experimentHeader.scheduler_data.scheduled[i].num_block = 0;
        experimentHeader.scheduler_data.scheduled[i].status = DB_EMPTY;
    }

    /* Atualiza o header na flash */
    return nascerr_db_updateHeader();
}

/* ---------------------------------------------------------------
 * nascerr_db_writeImage
 * Grava uma imagem na região reservada da flash.
 * data: ponteiro para os bytes da imagem
 * size: tamanho em bytes (máx FLASH_IMAGE_MAX_BYTES)
 * --------------------------------------------------------------- */
uint8_t nascerr_db_writeImage(const uint8_t *data, uint32_t size)
{
    if (data == NULL || size == 0 || size > FLASH_IMAGE_MAX_BYTES)
        return false;

    /* Apaga a região antes de gravar */
    if (nascerr_flash_eraseArea(FLASH_IMAGE) != STATUS_SUCCESS)
        return false;

    uint32_t start = W25N01G_BLOCK_TO_LINEAR(FLASH_IMAGE_BLOCK);
    FLASHTypeDef res = nascerr_flash_writeAddress(start, data, size);
    if (res.error) return false;

    if (nascerr_flash_flush(res.end_addr).error) return false;

    /* Atualiza metadados no header em RAM */
    experimentHeader.image_data.header.current_addr = res.end_addr;
    experimentHeader.image_data.header.data_load    = size;
    experimentHeader.image_data.image_size          = size;
    experimentHeader.image_data.crc16               = calculate_crc16(data, size);
    experimentHeader.image_data.valid               = 0xAAu;

    return nascerr_db_updateHeader();
}

uint8_t nascerr_db_writeImageChunk(uint32_t offset, const uint8_t *data, uint32_t size)
{
    if (data == NULL || size == 0) return false;
    if (experimentHeader.image_data.valid != 0x00u) return false;
    if (offset + size >= IMAGE_AREA_SIZE) return false;

//    uint32_t chunk_size = IMG_CHUNK_SIZE;
//    if (offset + chunk_size >= size)
//        chunk_size = size - offset;

    uint32_t addr = experimentHeader.image_data.header.start_addr + offset;
    FLASHTypeDef res = nascerr_flash_writeAddress(addr, data, size);

    if (!res.error) nascerr_flash_flush(res.end_addr);;

//    DEBUG_PRINT(LOG_DEBUG, "write: offset=%lu addr=0x%08lX end=0x%08lX\n\r",
//                offset, addr, res.end_addr);


//    res = nascerr_flash_flush(res.end_addr);
    if (res.error) return false;

//    DEBUG_PRINT(LOG_DEBUG, "write: offset=%lu addr=0x%08lX end=0x%08lX\n\r",
//                offset, addr, res.end_addr);

    experimentHeader.image_data.header.current_addr = res.end_addr;
    experimentHeader.image_data.header.data_load    += size;
//    experimentHeader.image_data.image_size          = size;

    experimentHeader.image_data.crc16 = update_crc16(
        experimentHeader.image_data.crc16, data, size
    );

    if (offset + size == experimentHeader.image_data.image_size)
        experimentHeader.image_data.valid = 0xAAu;

    return nascerr_db_updateHeader();
}

uint8_t nascerr_db_validateImage(uint32_t size, uint16_t crc16_expected)
{
	DEBUG_PRINT(LOG_WARN, "entrou na funcao de validacao \r\n");
    if (experimentHeader.image_data.valid != 0xAAu) {
    	DEBUG_PRINT(LOG_WARN, "faltou aa\r\n");
    	return false;
    }
    if (size == 0u || size > FLASH_IMAGE_MAX_BYTES)  {
    	DEBUG_PRINT(LOG_WARN, "tamanhos incorretos/formacao \r\n");
    	return false;
    }

    uint8_t  chunk[W25N01G_PAGE_SIZE];
    uint32_t done    = 0u;
    uint16_t crc_acc = 0xFFFFu;   /* valor inicial CRC16 */

    while (done < size) {
        uint32_t to_read = (size - done < sizeof(chunk))
                         ? (size - done)
                         : sizeof(chunk);

        if (!nascerr_db_readImageChunk(done, chunk, to_read)){
        	DEBUG_PRINT(LOG_WARN, "falha na leitura da flash \r\n");
            return false;
        }

        /* Acumula CRC16 sobre os bytes lidos */
        crc_acc = update_crc16(crc_acc, chunk, to_read);

        done += to_read;
//        i2c_slave_noBlockBus();
    }
    DEBUG_PRINT(LOG_WARN, "crc_lido %d, crc_esperado %d \r\n", crc16_expected, crc_acc);
    return (crc_acc == crc16_expected);
}

uint8_t nascerr_db_clearImageArea(void)
{
    if (nascerr_flash_eraseArea(FLASH_IMAGE) != STATUS_SUCCESS)
        return false;

    experimentHeader.image_data.header.start_addr   = W25N01G_BLOCK_TO_LINEAR(FLASH_IMAGE_BLOCK);
    experimentHeader.image_data.header.current_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_IMAGE_BLOCK);
    experimentHeader.image_data.header.data_load = 0;
    experimentHeader.image_data.image_size       = 0;
    experimentHeader.image_data.crc16            = 0xFFFFu;
    experimentHeader.image_data.valid            = 0x00u;

    return nascerr_db_updateHeader();
}

/* ---------------------------------------------------------------
 * nascerr_db_readImageChunk
 * Lê um trecho da imagem a partir de um offset.
 * Usado para carregar pedaços na SRAM bloco a bloco.
 * --------------------------------------------------------------- */
uint8_t nascerr_db_readImageChunk(uint32_t offset,
                                   uint8_t *buffer,
                                   uint32_t size)
{
    if (buffer == NULL || size == 0) return false;

    if (experimentHeader.image_data.valid != 0xAAu) return false;

    uint32_t img_size = experimentHeader.image_data.image_size;
    if (offset >= img_size) return false;

    /* Clipa o tamanho ao fim da imagem */
    if (offset + size > img_size)
        size = img_size - offset;

    uint32_t addr = experimentHeader.image_data.header.start_addr + offset;
    FLASHTypeDef res = nascerr_flash_readAddress(addr, buffer, size);
    return !res.error;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_db_setImageSize
 *  @brief     : Define o tamanho total da imagem que será recebida.
 *               Deve ser chamada após nascerr_db_clearImageArea(), antes de
 *               qualquer nascerr_db_writeImageChunk().
 *               O campo image_size é usado por writeImageChunk para saber
 *               quando a imagem está completa e setar valid=0xAA.
 *  @param     : size — tamanho total da imagem em bytes
 *  @retval    : true se OK, false se parâmetro inválido ou imagem já válida
 * ******************************************************************************
 */
uint8_t nascerr_db_setImageSize(uint32_t size)
{
    if (size == 0u || size > FLASH_IMAGE_MAX_BYTES)
        return false;

    /* Não permite sobrescrever uma imagem já validada */
    if (experimentHeader.image_data.valid == 0xAAu)
        return false;

    experimentHeader.image_data.image_size = size;

    return nascerr_db_updateHeader();
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
