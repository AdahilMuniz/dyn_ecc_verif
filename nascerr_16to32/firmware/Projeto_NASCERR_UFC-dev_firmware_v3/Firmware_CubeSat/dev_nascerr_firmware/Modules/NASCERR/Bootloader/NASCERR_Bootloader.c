/**
 * ******************************************************************************
 *  @file    : NASCERR_Bootloader.c
 *  @brief   : NASCERR bootloader system.
 *  @author  : Joao Manoel
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */
#include "NASCERR_Bootloader.h"

/* Global instance of the bootloader state */
BootloaderState         bootState;						/* Bootloader state structure */

static BootSM_State 	sm_state;						/* State machine state */
static BOOT_DATA_PACKET sm_packet;						/* Packet structure for state machine */

uint8_t                 boot_in_process = false;  		/* Flag controlling the state machine loop*/
static uint16_t			g_last_pkg_id = 0;				/* Last package ID received */
static uint8_t 			g_runtime_flags = 0;			/* Runtime flags for bootloader state */
static uint16_t			boot_number_pkgs = 0;			/* Number of packages expected in the download process */

extern HASH_HandleTypeDef hhash;
uint32_t boot_timeout_timer[3] = {0, 0, 0};

firmware_metadata_t *firmware_metadata_flash = (firmware_metadata_t*)METADATA_ADDRESS; 	/* Pointer to the firmware metadata in flash */

/* Local function prototypes */

/*
 * ******************************************************************************
 * Function Name: nascerr_bootloader_setRuntimeFlag
 * @brief       : Sets a runtime flag in the bootloader state.
 * @param flag  : The flag to set.
 * @retval      : The updated runtime flags.
 * ******************************************************************************
 */
uint8_t nascerr_bootloader_setRuntimeError(ERROS_LOGS error_code, uint8_t status);

/* global functions */

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_init
 *  @brief       : Inicializa o processo de bootloader para recepcao de novo firmware.
 *                 Valida o comando recebido, interpreta metadados do pacote de
 *                 inicializacao, atualiza registros de versao e apaga a area de
 *                 memoria destinada ao novo firmware.
 *  @param       : ctrl – Ponteiro para a estrutura BOOT_DATA_PACKET contendo
 *                 comando, dados e parametros de inicializacao.
 *  @retval      : 1 se o processo foi iniciado com sucesso.
 *                 0 se ocorrer erro na validacao do comando ou nas operacoes
 *                   de gravacao/apagamento.
 * ******************************************************************************
 */
uint8_t nascerr_bootloader_init(BOOT_DATA_PACKET *ctrl)
{
    uint32_t firmwareVersion;
    uint32_t appSize;
    uint8_t  retorno = false;

    nascerr_boot_comm_init(COMM_I2C);

    vTaskSuspendAll();

    if ((BootCommand_t)ctrl->cmd != PROTOCOL_CMD_DOWNLOAD_INIT) {
        DEBUG_PRINT(LOG_ERROR,
            "BOOT ERROR: comando invalido 0x%02X (esperado 0x%02X)\r\n",
            ctrl->cmd,
            PROTOCOL_CMD_DOWNLOAD_INIT
        );
        nascerr_bootloader_setRuntimeError(ERR_BOOT_CMD_INIT, LOG_STATUS_ERROR);
        nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
        goto cleanup;
    }
    const uint8_t *p = ctrl->data;

    appSize =  GET_U32_BE(p); p += 4;

    firmwareVersion = GET_U32_BE(p); p += 4;

    boot_number_pkgs = GET_U32_BE(p);
    DEBUG_PRINT(LOG_INFO,
        "BOOT: init ver=%lu, size=%lu, pacotes=%u\r\n",
        firmwareVersion,
        appSize,
        boot_number_pkgs
    );

    if (!nascerr_bootloader_setNewVersion(firmwareVersion, appSize)) {
        DEBUG_PRINT(LOG_ERROR,
            "BOOT ERROR: falha ao gravar metadata new versao=%lu size=%lu\r\n",
            firmwareVersion,
            appSize
        );
        nascerr_bootloader_setRuntimeError(ERR_BOOT_FLASH_ERASE, LOG_STATUS_ERROR);
        nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
        goto cleanup;
    }

    if (!nascerr_bootloader_eraseFlash(
            appSize,
            firmware_metadata_flash->new_firmware_addr))
    {
        DEBUG_PRINT(LOG_ERROR,
            "BOOT ERROR: falha ao apagar flash em 0x%08lX, size=%lu\r\n",
            firmware_metadata_flash->new_firmware_addr,
            appSize
        );
        nascerr_bootloader_setRuntimeError(ERR_BOOT_FLASH_WRITE, LOG_STATUS_ERROR);
        nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
        goto cleanup;
    }

    nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_IN_DOWNLOAD);
    DEBUG_PRINT(LOG_INFO,
        "BOOT: entrando em modo download (versao %lu, %lu bytes)\r\n",
        firmwareVersion,
        appSize
    );

    retorno = true;

cleanup:
    xTaskResumeAll();
    return retorno;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_fsmInit
 *  @brief       : Inicializa a maquina de estados do bootloader, configurando
 *                 variaveis globais para o estado inicial de recepcao de pacotes.
 *                 Define o endereco inicial de escrita, zera o ID do ultimo
 *                 pacote recebido e ajusta as flags de execucao para o modo
 *                 bootloader.
 *  @param       : Nenhum.
 *  @retval      : Nenhum.
 * ******************************************************************************
 */
void nascerr_bootloader_fsmInit(void)
{
  sm_state 						= BS_WAIT_PKT;
  boot_in_process				= true;
  nascerr_systimer((timer_value)BOOT_TIMEOUT_MS, boot_timeout_timer);
  bootState.currentAddress 		= FLASH_APP2_START_ADDRESS;
  bootState.lastPacketID		= 0;
  bootState.errorCode 			= ERR_NONE;
  nascerr_bootloader_resetRuntimeFlag(BOOT_ALL_FLAGS);
  nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_IN_DOWNLOAD);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_fsmStep
 *  @brief       : Executa um passo da maquina de estados do bootloader, processando
 *                 o estado atual e realizando transicoes conforme o comando recebido.
 *                 Trata comandos de transferencia de dados, finalizacao do download
 *                 ou erros de protocolo.
 *  @param       : Nenhum.
 *  @retval      : Nenhum.
 *
 *  @note        : Estados processados:
 *                   - BS_WAIT_PKT       : Aguarda pacote; avanca conforme o comando.
 *                   - BS_DATA_TRANSFER  : Processa bloco de dados; retorna a espera
 *                                          ou vai para erro.
 *                   - BS_DOWNLOAD_FINISH: Processa finalizacao do download; vai para
 *                                          BS_DONE ou BS_ERROR.
 *                   - BS_ERROR          : Encerra processo de bootloader por erro.
 *                   - BS_DONE           : Estado final ate reinicializacao da FSM.
 * ******************************************************************************
 */
void nascerr_bootloader_fsmStep(void)
{
  if (boot_in_process) {
	  if (nascerr_systimer(BOOT_TIMEOUT_MS, boot_timeout_timer) == Set) {
		  DEBUG_PRINT(LOG_ERROR, "BOOT: timeout atingido\r\n");
		  nascerr_bootloader_setRuntimeError(ERR_BOOT_TIMEOUT, LOG_STATUS_ERROR);
		  nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
		  sm_state = BS_ERROR;
	  }
  }
  switch (sm_state) {
    case BS_WAIT_PKT:
    	if (nascerr_boot_comm_receive(&sm_packet)) {
    		nascerr_systimer_reset(boot_timeout_timer);
    		nascerr_systimer(BOOT_TIMEOUT_MS, boot_timeout_timer);

    	    if (sm_packet.cmd == PROTOCOL_CMD_DATA_TRANSFER) {
    	        sm_state = BS_DATA_TRANSFER;
    	    }
    	    else if (sm_packet.cmd == PROTOCOL_CMD_DOWNLOAD_FINISH) {
    	        sm_state = BS_DOWNLOAD_FINISH;
    	    }
    	    else {
    	        sm_state = BS_ERROR;
    	    }
    	}
      break;

    case BS_DATA_TRANSFER:
		if (nascerr_bootloader_processDataTransfer(&sm_packet)) {
		  sm_state = BS_WAIT_PKT;
		}
		else {
		  sm_state = BS_ERROR;
		}
		break;

    case BS_DOWNLOAD_FINISH:
        if (nascerr_bootloader_processDownloadFinish(&sm_packet)) {
          sm_state = BS_DONE;
        }
        else {
          sm_state = BS_ERROR;
        }
        break;

    case BS_ERROR:
    	nascerr_bootloader_resetRuntimeFlag(BOOT_ALL_FLAGS);
		nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
		boot_in_process = false;
		sm_state = BS_DONE;
		break;

    case BS_DONE:

    default:
      break;
  }
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_processDataTransfer
 *  @brief       : Processa um pacote de transferencia de dados do firmware no
 *                 processo de bootloader. Valida tamanho, ordem e integridade
 *                 do pacote antes de gravar o conteudo na memoria flash.
 *  @param       : pkt – Ponteiro constante para a estrutura BOOT_DATA_PACKET
 *                 contendo o pacote de dados recebido.
 *  @retval      : 1 se o pacote foi validado e gravado com sucesso na flash.
 *                 0 se ocorrer qualquer erro de validacao ou gravacao.
 *
 *  @note        : Validacoes realizadas:
 *                   - Tamanho do header >= BOOT_PAYLOAD_TRANSFER_HEADER_SIZE.
 *                   - ID do pacote sequencial ao ultimo recebido.
 *                   - ID do pacote <= numero total de pacotes esperados.
 *                   - Tamanho total do pacote <= 263 bytes.
 *                 Em caso de erro, a flag BOOT_FLAG_ERROR_DETECTED e acionada.
 * ******************************************************************************
 */
uint8_t nascerr_bootloader_processDataTransfer(const BOOT_DATA_PACKET *pkt)
{
    if (pkt->dataSize < BOOT_PAYLOAD_TRANSFER_HEADER_SIZE) {
        nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
        DEBUG_PRINT(LOG_ERROR, "BOOT: tamanho de header invalido (%u)\r\n", pkt->dataSize);
        nascerr_bootloader_setRuntimeError(ERR_BOOT_CMD_SIZE, LOG_STATUS_ERROR);
        return false;
    }

    uint16_t pkg_id = GET_U16_BE(pkt->data);
    if (pkg_id != (bootState.lastPacketID + 1)) {
        nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
        DEBUG_PRINT(LOG_ERROR, "BOOT: pacote fora de ordem (esperado %u, recebido %u)\r\n",
                    bootState.lastPacketID + 1, pkg_id);
        nascerr_bootloader_setRuntimeError(ERR_BOOT_PKG_OUT_OF_SEQ, LOG_STATUS_ERROR);
        return false;
    }

    if (pkg_id > boot_number_pkgs) {
        nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
        DEBUG_PRINT(LOG_ERROR, "BOOT: pacote %u > numero total %u\r\n",
                    pkg_id, boot_number_pkgs);
        nascerr_bootloader_setRuntimeError(ERR_BOOT_PKG_OUT_OF_SEQ, LOG_STATUS_ERROR);
        return false;
    }

    /* BOOT_PAYLOAD_TRANSFER_HEADER_SIZE + 256) */
    if (pkt->dataSize > REG_DATA_BOOT_SIZE) {
        nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
        DEBUG_PRINT(LOG_ERROR, "BOOT: dataSize %u > 263\r\n", pkt->dataSize);
        nascerr_bootloader_setRuntimeError(ERR_BOOT_FLASH_WRITE_SIZE, LOG_STATUS_ERROR);
        return false;
    }

    uint8_t 	*user_data 	= &pkt->data[2];
    uint32_t    user_len  	= pkt->dataSize - BOOT_PAYLOAD_TRANSFER_HEADER_SIZE;

    uint8_t res = nascerr_bootloader_writeToFlash(bootState.currentAddress, user_data, user_len);
    if (!res)
    {
        nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
        DEBUG_PRINT(LOG_ERROR, "BOOT: falha ao gravar flash em 0x%08X\r\n",
                    bootState.currentAddress);
        nascerr_bootloader_setRuntimeError(ERR_BOOT_FLASH_WRITE, LOG_STATUS_ERROR);
        return false;
    }

    bootState.lastPacketID    = pkg_id;
    nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_DOWNLOAD_SUCCESS);
    nascerr_bootloader_setPkgID(bootState.lastPacketID);

    DEBUG_PRINT(LOG_INFO, "BOOT: gravado pacote %u (%u bytes)\r\n", pkg_id, user_len);

    return true;
}

/**
 * ******************************************************************************
 * Function Name: nascerr_bootloader_processDownloadFinish
 * @brief       : Processa o pacote de finalizacao do download. Valida a versao,
 * extrai e salva o hash SHA-256 e, se valido, prepara para
 * o salto para o bootloader (Estágio 2).
 * @param       : pkt – Ponteiro constante para a estrutura BOOT_DATA_PACKET.
 * @retval      : 1 se o processo for bem-sucedido.
 * 0 se ocorrer qualquer erro.
 *
 * @note        : O payload esperado é [ version (4B) | SHA-256 (32B) ].
 * ******************************************************************************
 */
uint8_t nascerr_bootloader_processDownloadFinish(const BOOT_DATA_PACKET *pkt)
{
    if (pkt->dataSize != BOOT_PAYLOAD_TRASNFER_FINISH_SIZE) {
        nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
        DEBUG_PRINT(LOG_ERROR, "BOOT: finish packet tamanho invalido %u\r\n", pkt->dataSize);
        nascerr_bootloader_setRuntimeError(ERR_BOOT_CMD_SIZE, LOG_STATUS_ERROR);
        return false;
    }

    uint32_t new_ver = GET_U32_BE(pkt->data);

    if (new_ver == 0 || new_ver != firmware_metadata_flash->new_firmware_version) {
        nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
        DEBUG_PRINT(LOG_ERROR, "BOOT: versao recebida %u invalida ou diferente da esperada %u\r\n",
                    new_ver, firmware_metadata_flash->new_firmware_version);
        nascerr_bootloader_setRuntimeError(ERR_BOOT_PKG_INVALID_VER, LOG_STATUS_ERROR);
        return false;
    }

    const uint8_t* received_sha = &pkt->data[4];
    DEBUG_PRINT(LOG_INFO, "BOOT: SHA-256 recebido, gravando no metadata...\r\n");

    if (!nascerr_bootloader_writeNewFirmwareSha(received_sha)) {
        nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
        DEBUG_PRINT(LOG_ERROR, "BOOT: falha ao gravar o SHA-256 no metadata!\r\n");
        nascerr_bootloader_setRuntimeError(ERR_BOOT_FLASH_WRITE, LOG_STATUS_ERROR);
        return false;
    }

    DEBUG_PRINT(LOG_INFO, "BOOT: download concluido, pulando para bootloader v%u\r\n", new_ver);

    nascerr_bootloader_resetRuntimeFlag(BOOT_ALL_FLAGS);
    nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_UPDATE_PENDING);
    nascerr_bootloader_updateBootFlags();
    nascerr_bootloader_jumpToBootloader();

    return true;
}

/**
 * ******************************************************************************
 * Function Name: nascerr_bootloader_jumpToBootloader
 * @brief       : Forca um reset de software no sistema para que o bootloader
 * (Estágio 2) possa assumir e realizar a troca do firmware.
 * @param       : Nenhum.
 * @retval      : Nenhum (a funcao nunca retorna).
 * ******************************************************************************
 */
void nascerr_bootloader_jumpToBootloader(void)
{
	HAL_NVIC_SystemReset();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_eraseFlash
 *  @brief       : Configura os parametros de estado do bootloader e executa o
 *                 apagamento da area de memoria flash onde o novo firmware sera
 *                 gravado.
 *  @param       : sizeApp – Tamanho total do novo firmware em bytes.
 *                 addrApp – Endereco inicial na flash para gravacao do firmware.
 *  @retval      : 1 se o apagamento das paginas foi bem-sucedido.
 *                 0 se ocorrer falha no processo de apagamento.
 *
 *  @note        : Calcula a quantidade de paginas a serem apagadas com base no
 *                 tamanho do firmware e no tamanho de pagina (PAGE_SIZE). Se
 *                 houver bytes restantes apos a divisao, e apagada uma pagina
 *                 adicional. As variaveis de estado do bootloader sao inicializadas
 *                 antes do apagamento.
 * ******************************************************************************
 */

uint8_t nascerr_bootloader_eraseFlash(uint32_t sizeApp, uint32_t addrApp)
{
    bootState.currentAddress 	= addrApp;
    bootState.appSize 			= sizeApp;
    bootState.bytesWritten 		= 0;
    bootState.status 			= true;
    bootState.errorCode 		= 0;

	uint32_t pagesErase = (sizeApp / PAGE_SIZE);

	if(sizeApp % PAGE_SIZE) pagesErase+=1;

	bootState.status = nascerr_flash_erase_pages(addrApp, pagesErase);

	return bootState.status;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_writeToFlash
 *  @brief       : Grava um bloco de dados na memoria flash durante o processo
 *                 de bootloader, validando o alinhamento do endereco.
 *  @param       : address – Endereco de memoria flash onde os dados serao gravados.
 *                 data    – Ponteiro para o buffer de dados a gravar.
 *                 pkgSize – Tamanho do bloco de dados em bytes.
 *  @retval      : 1 se a gravacao for bem-sucedida.
 *                 0 se ocorrer erro de alinhamento ou falha na escrita.
 *
 *  @note        : O endereco deve estar alinhado a 8 bytes. Em caso de
 *                 desalinhamento, a funcao retorna erro e aciona a flag
 *                 BOOT_FLAG_ERROR_DETECTED. Em caso de sucesso, atualiza
 *                 bootState.currentAddress para apontar para a proxima
 *                 posicao livre na flash.
 * ******************************************************************************
 */
uint8_t nascerr_bootloader_writeToFlash(uint32_t address, uint8_t *data, uint32_t pkgSize)
{
    if (address % 8 != 0) {
    	bootState.status = false;
    	return bootState.status;
    }

	if(!nascerr_flash_write_bytes(address, data, pkgSize))
		return false;
	bootState.currentAddress += pkgSize;
	return true;
}

/**
 * ******************************************************************************
 * Function Name: nascerr_bootloader_calculateSha256
 * @brief       : Calcula o hash SHA-256 de um buffer de dados usando o
 * periférico de hardware do STM32.
 * @param       : data - Ponteiro para o início dos dados a serem processados.
 * size - Tamanho dos dados em bytes.
 * output_sha - Ponteiro para um buffer de 32 bytes onde o
 * hash calculado será armazenado.
 * @retval      : 1 (true) se o cálculo for bem-sucedido.
 * 				: 0 (false) se ocorrer um erro na HAL.
 * ******************************************************************************
 */
uint8_t nascerr_bootloader_calculateSha256(const uint8_t* data, uint32_t size, uint8_t* output_sha)
{

    HAL_StatusTypeDef status = HASH_Start(&hhash, (uint8_t*)data, size, output_sha, 1000, HASH_ALGOSELECTION_SHA256);

    if (status == HAL_OK) {
        return true;
    }
    return false;
}

/*-------------------------------METADATA----------------------------------*/

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_initMetadata
 *  @brief       : Inicializa ou valida a estrutura de metadata do firmware na
 *                 memoria flash. Caso os dados sejam invalidos ou o CRC do log
 *                 nao corresponda, reconfigura o metadata com valores padrao.
 *  @param       : Nenhum.
 *  @retval      : Nenhum.
 *
 *  @note        : Operacoes realizadas:
 *                   - Le o metadata atual do endereco METADATA_ADDRESS.
 *                   - Calcula o CRC16 do log armazenado, se o tamanho for valido.
 *                   - Verifica se o campo magic (METADATA_MAGIC) e o CRC do log
 *                     sao validos.
 *                   - Em caso de falha, preenche todos os campos do metadata com
 *                     valores padrao, incluindo informacoes do firmware atual e
 *                     espaco para novo firmware.
 *                   - Zera todos os campos de flags e redefine o log como vazio.
 *                   - Escreve o novo metadata na flash.
 * ******************************************************************************
 */
void nascerr_bootloader_initMetadata(void)
{
    firmware_metadata_t md;
    uint16_t crc_log_calculated = 0;

    memcpy(&md,
           (void*)METADATA_ADDRESS,
           sizeof(firmware_metadata_t));

    uint16_t crc_metadata_calculated = calculate_crc16((uint8_t*)&md, sizeof(firmware_metadata_t) - sizeof(md.crc_metadata));

    uint32_t start = FLASH_LOG_START_ADDRESS;
    uint32_t end   = md.end_address_log;
    uint32_t length = end - start;

    if (length > 0 && length <= EXTERNAL_FLASH_PAGE_SIZE) {
        crc_log_calculated = calculate_crc16((uint8_t*)start, length);
    } else {
		crc_log_calculated = 0;
		nascerr_flash_reset_log_buffer();
		md.end_address_log = FLASH_LOG_START_ADDRESS;
		md.crc_log = 0;
	}

    if (md.crc_log != crc_log_calculated) {
    			DEBUG_PRINT(LOG_WARN, "CRC de estrutrua de LOG inválido\r\n",
					md.crc_log, crc_log_calculated);
    			nascerr_flash_reset_log_buffer();

    			md.end_address_log = FLASH_LOG_START_ADDRESS;
    			md.crc_log = 0;
	}

    if (md.magic != METADATA_MAGIC || md.crc_metadata != crc_metadata_calculated || md.current_firmware_version != APP_VERSION) {
        md.magic                   = METADATA_MAGIC;
        md.current_firmware_addr   = FIRST_APPLICATION_ADDRESS;
        md.current_firmware_size   = APP_SIZE;
        md.current_firmware_version= APP_VERSION;

        DEBUG_PRINT(LOG_WARN, "Estrutura de Metadata invalida ou corrompida! Inicializando Default\r\n",
                    md.current_firmware_addr, md.current_firmware_size);

        if (nascerr_bootloader_calculateSha256((uint8_t*)_app_start, APP_SIZE, md.current_firmware_sha))
        {
            DEBUG_PRINT(LOG_DEBUG, "Codigo SHA-256 da aplicacao calculado com sucesso\r\n");
        }
        else
        {
            memset(md.current_firmware_sha, 0, sizeof(md.current_firmware_sha));
        }

        md.new_firmware_addr       = SECOND_APPLICATION_ADDRESS;
        md.new_firmware_size       = 0;
        md.new_firmware_version    = 0;
        memset(md.new_firmware_sha, 0xFF, sizeof(md.new_firmware_sha));

        // Zera flags
        md.boot_flag               = 0;
        md.execution_mode_flag     = 0;
        md.error_flag              = 0;
        md.rollback_flag           = 0;

        md.end_address_log         = FLASH_LOG_START_ADDRESS;
        md.crc_log                 = 0;
    }

    nascerr_bootloader_initFlags();
    nascerr_bootloader_writeNewMetadata(md);
    DEBUG_PRINT(LOG_INFO, "Firmware v%02u.%02u.%02u iniciando\r\n",
                (md.current_firmware_version >> 16) & 0xFF,
                (md.current_firmware_version >>  8) & 0xFF,
                 md.current_firmware_version        & 0xFF);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_writeNewMetadata
 *  @brief       : Grava uma nova estrutura de metadata do firmware na memoria
 *                 flash, substituindo completamente o valor anterior.
 *  @param       : metadata – Estrutura firmware_metadata_t contendo os dados
 *                 que serao gravados no endereco METADATA_ADDRESS.
 *  @retval      : 1 se a gravacao for bem-sucedida.
 *                 0 se ocorrer falha no apagamento ou escrita da flash.
 *
 *  @note        : Procedimento:
 *                   - Apaga a pagina de memoria onde o metadata esta localizado.
 *                   - Converte a estrutura para ponteiro de 64 bits.
 *                   - Grava os dados na flash em blocos de 8 bytes
 *                     (doubleword write).
 *                   - Interrompe e retorna erro se qualquer escrita falhar.
 * ******************************************************************************
 */
uint8_t nascerr_bootloader_writeNewMetadata(firmware_metadata_t metadata)
{
    if (!nascerr_flash_erase_pages(METADATA_ADDRESS, 1))
        return false;

    uint16_t crc_metadata = calculate_crc16((uint8_t*)&metadata, sizeof(firmware_metadata_t) - sizeof(metadata.crc_metadata));
    metadata.crc_metadata = crc_metadata;
    uint64_t *pData = (uint64_t *)&metadata;

    for (uint32_t i = 0; i < sizeof(firmware_metadata_t) / 8; i++)
    {
        if (!nascerr_flash_write_doubleword(METADATA_ADDRESS + i * 8, pData[i]))
        {
            return false;
        }

    }
    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_initFlags
 *  @brief       : Inicializa as flags de execucao em tempo de execucao do bootloader
 *                 com base no valor armazenado no campo boot_flag do metadata
 *                 presente na memoria flash.
 *  @param       : Nenhum.
 *  @retval      : Nenhum.
 * ******************************************************************************
 */
void nascerr_bootloader_initFlags(void)
{
    g_runtime_flags = firmware_metadata_flash->boot_flag;
    nascerr_bootloader_resetRuntimeFlag(BOOT_FLAG_IN_BOOTLOADER);
    nascerr_bootloader_updateBootFlags();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_setRuntimeFlag
 *  @brief       : Ativa (seta) uma flag especifica no conjunto de flags de
 *                 execucao do bootloader.
 *  @param       : f – Flag do tipo BOOT_FLAG a ser ativada.
 *  @retval      : Nenhum.
 *
 *  @note        : A flag informada e combinada ao valor atual de g_runtime_flags
 *                 por operacao OR bit a bit, preservando as demais flags ja
 *                 ativadas.
 * ******************************************************************************
 */
void nascerr_bootloader_setRuntimeFlag(BOOT_FLAG f)
{
	g_runtime_flags |=  (uint8_t)f;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_resetRuntimeFlag
 *  @brief       : Desativa (reseta) uma flag especifica no conjunto de flags
 *                 de execucao do bootloader.
 *  @param       : f – Flag do tipo BOOT_FLAG a ser desativada.
 *  @retval      : Nenhum.
 *
 *  @note        : A flag informada e removida do valor atual de g_runtime_flags
 *                 por operacao AND com o inverso (bitwise NOT) da flag, mantendo
 *                 inalteradas as demais flags.
 * ******************************************************************************
 */
void nascerr_bootloader_resetRuntimeFlag(BOOT_FLAG f)
{
	uint8_t roolback_flag = g_runtime_flags & BOOT_FLAG_ROLLBACK_AVAILABLE;
	g_runtime_flags &= ~(uint8_t)f;
	if(roolback_flag)
		g_runtime_flags |= BOOT_FLAG_ROLLBACK_AVAILABLE;      				 /* Mantem a flag de rollback se ja estava setada */
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_buildFlagsFromMetadata
 *  @brief       : Retorna o valor atual das flags de execucao do bootloader.
 *  @param       : Nenhum.
 *  @retval      : Valor de g_runtime_flags contendo o conjunto de flags ativas.
 *
 *  @note        : Esta funcao e estatica (escopo restrito ao arquivo) e serve
 *                 como utilitario para obter as flags correntes, tipicamente
 *                 para uso em operacoes que precisam gerar uma representacao
 *                 baseada no metadata do firmware.
 * ******************************************************************************
 */
static uint8_t nascerr_bootloader_buildFlagsFromMetadata(void)
{
    return g_runtime_flags;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_setPkgID
 *  @brief       : Atualiza o identificador do ultimo pacote recebido durante
 *                 o processo de transferencia de firmware.
 *  @param       : new_id – Novo valor para o identificador do pacote.
 *  @retval      : Nenhum.
 *
 *  @note        : O valor e armazenado na variavel global g_last_pkg_id, que
 *                 pode ser utilizada para controle de sequencia de pacotes
 *                 e verificacao de integridade da transmissao.
 * ******************************************************************************
 */
void nascerr_bootloader_setPkgID(uint16_t new_id)
{
	g_last_pkg_id = new_id;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_getPkgID
 *  @brief       : Retorna o identificador do ultimo pacote recebido durante
 *                 o processo de transferencia de firmware.
 *  @param       : Nenhum.
 *  @retval      : Valor de g_last_pkg_id representando o ID do ultimo pacote.
 *
 *  @note        : Usado principalmente para verificacao de sequencia de pacotes
 *                 no processo de download do firmware via bootloader.
 * ******************************************************************************
 */
uint16_t nascerr_bootloader_getPkgID(void)
{
	return g_last_pkg_id;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_setNewVersion
 *  @brief       : Atualiza no metadata as informacoes da nova versao de firmware
 *                 a ser gravada, incluindo versao, tamanho e endereco de destino.
 *  @param       : new_version      – Numero da nova versao do firmware.
 *                 new_version_size – Tamanho do novo firmware em bytes.
 *  @retval      : 1 se a atualizacao e gravacao do metadata forem bem-sucedidas.
 *                 0 se ocorrer falha de validacao ou gravacao.
 *
 *  @note        : Procedimento:
 *                   - Copia o metadata atual para um buffer temporario.
 *                   - Verifica consistencia do campo magic.
 *                   - Atualiza versao e tamanho do novo firmware.
 *                   - Alterna o endereco de destino entre FIRST_APPLICATION_ADDRESS
 *                     e SECOND_APPLICATION_ADDRESS para implementar esquema de
 *                     dual slot.
 *                   - Grava o metadata atualizado na flash.
 * ******************************************************************************
 */
uint8_t nascerr_bootloader_setNewVersion(uint32_t new_version, uint32_t new_version_size)
{
	firmware_metadata_t buffer;

	memcpy(&buffer, (void *)firmware_metadata_flash, sizeof(firmware_metadata_t));

	if(buffer.magic != firmware_metadata_flash->magic || buffer.crc_metadata != firmware_metadata_flash->crc_metadata)
		return false;

	buffer.new_firmware_version = new_version;
	buffer.new_firmware_size = new_version_size;

	if(buffer.current_firmware_addr == FIRST_APPLICATION_ADDRESS)
	{
		buffer.new_firmware_addr = SECOND_APPLICATION_ADDRESS;
	} else if(buffer.current_firmware_addr == SECOND_APPLICATION_ADDRESS)
	{
		buffer.new_firmware_addr = FIRST_APPLICATION_ADDRESS;
	}

	if(!nascerr_bootloader_writeNewMetadata(buffer))
		return false;

	return true;
}

/**
 * ******************************************************************************
 * Function Name: nascerr_bootloader_writeNewFirmwareSha
 * @brief       : Atualiza o campo SHA-256 do novo firmware no metadata.
 * @param       : sha - Ponteiro para o buffer de 32 bytes contendo o hash
 * SHA-256 a ser gravado.
 * @retval      : 1 se a atualização for bem-sucedida.
 * 				: 0 se ocorrer falha na leitura ou escrita do metadata.
 * ******************************************************************************
 */
uint8_t nascerr_bootloader_writeNewFirmwareSha(const uint8_t* sha)
{
    firmware_metadata_t temp_metadata;

    memcpy(&temp_metadata, (void*)METADATA_ADDRESS, sizeof(firmware_metadata_t));

    memcpy(temp_metadata.new_firmware_sha, sha, 32);

    return nascerr_bootloader_writeNewMetadata(temp_metadata);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_updateBootFlags
 *  @brief       : Atualiza o campo boot_flag no metadata do firmware com as
 *                 flags de execucao em tempo de execucao atualmente ativas.
 *  @param       : mask   – Mascara de bits para selecao de flags (nao utilizada
 *                           na implementacao atual).
 *                 enable – Valor para habilitar/desabilitar flags (nao utilizado
 *                           na implementacao atual).
 *  @retval      : 1 se a atualizacao e gravacao do metadata forem bem-sucedidas
 *                   ou se nao houver flags ativas para atualizar.
 *                 0 se ocorrer falha na gravacao do metadata.
 *
 *  @note        : Procedimento:
 *                   - Se nao houver flags ativas em g_runtime_flags, retorna
 *                     sucesso imediatamente.
 *                   - Copia o metadata atual para um buffer temporario.
 *                   - Aplica as flags ativas de g_runtime_flags no campo boot_flag.
 *                   - Grava o metadata atualizado na flash.
 *                   - Zera g_runtime_flags apos a gravacao.
 * ******************************************************************************
 */
uint8_t nascerr_bootloader_updateBootFlags(void)
{
    if (g_runtime_flags == 0)
        return true;

    firmware_metadata_t tmp;
    memcpy(&tmp, (void*)firmware_metadata_flash, sizeof(tmp));

    tmp.boot_flag = g_runtime_flags;

    if (!nascerr_bootloader_writeNewMetadata(tmp))
        return false;

    g_runtime_flags = 0;
    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_changeBootStatus
 *  @brief       : Atualiza o buffer de dados do registrador BOOT_STAT com as
 *                 informacoes fornecidas na estrutura NASCERR_BOOT_STATUS.
 *  @param       : boot_status – Ponteiro para estrutura NASCERR_BOOT_STATUS
 *                 contendo os campos a serem escritos no buffer.
 *  @retval      : Estrutura NASCERR_BOOT_STATUS recebida como parametro (retorno
 *                 por valor).
 *
 *  @note        : Campos gravados no buffer g_register_data[BOOT_STAT].value:
 *                   - Bytes [0..3]   : firmware_version (little-endian)
 *                   - Bytes [4..7]   : other_firmware_version (little-endian)
 *                   - Byte  [8]      : flags
 *                   - Byte  [9]      : last_error
 *                   - Bytes [10..11] : last_packet_id (little-endian)
 *                   - Byte  [12]     : current_region
 *                   - Byte  [13]     : reserved
 * ******************************************************************************
 */
uint8_t *nascerr_bootloader_changeBootStatus(NASCERR_BOOT_STATUS *boot_status)
{
    uint8_t *buf = g_register_data[BOOT_STAT].value;

    /* firmware_version (big-endian) */
    buf[0] = (uint8_t)((boot_status->firmware_version >> 24) & 0xFF);
    buf[1] = (uint8_t)((boot_status->firmware_version >> 16) & 0xFF);
    buf[2] = (uint8_t)((boot_status->firmware_version >>  8) & 0xFF);
    buf[3] = (uint8_t)( boot_status->firmware_version        & 0xFF);

    /* other_firmware_version (big-endian) */
    buf[4] = (uint8_t)((boot_status->other_firmware_version >> 24) & 0xFF);
    buf[5] = (uint8_t)((boot_status->other_firmware_version >> 16) & 0xFF);
    buf[6] = (uint8_t)((boot_status->other_firmware_version >>  8) & 0xFF);
    buf[7] = (uint8_t)( boot_status->other_firmware_version        & 0xFF);

    /* flags e erro */
    buf[8] = boot_status->flags;
    buf[9] = boot_status->last_error;

    /* last_packet_id (big-endian) */
    buf[10] = (uint8_t)((boot_status->last_packet_id >> 8) & 0xFF);
    buf[11] = (uint8_t)( boot_status->last_packet_id       & 0xFF);

    /* region e reservado (1 byte cada, endian não importa) */
    buf[12] = (uint8_t)(boot_status->current_region & 0xFF);
    buf[13] = (uint8_t)(boot_status->reserved       & 0xFF);

    return buf;          /* devolve cópia por valor */
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_getBootStatus
 *  @brief       : Monta e retorna a estrutura NASCERR_BOOT_STATUS contendo
 *                 informacoes atuais do estado do bootloader e atualiza o
 *                 registrador BOOT_STAT com esses valores.
 *  @param       : Nenhum.
 *  @retval      : Estrutura NASCERR_BOOT_STATUS preenchida com:
 *                   - Versao atual do firmware.
 *                   - Versao do novo firmware (se houver).
 *                   - Flags de execucao atuais.
 *                   - Codigo do ultimo erro.
 *                   - ID do ultimo pacote recebido.
 *                   - Regiao atual de execucao (0xAA = APP1, 0xBB = APP2).
 *                   - Campo reservado = 0.
 *
 *  @note        : Os valores sao obtidos do metadata em flash e de variaveis
 *                 de controle em RAM. A funcao chama internamente
 *                 nascerr_bootloader_changeBootStatus() para refletir as
 *                 informacoes no buffer g_register_data[BOOT_STAT].
 * ******************************************************************************
 */
uint8_t *nascerr_bootloader_getBootStatus(void)
{
    NASCERR_BOOT_STATUS st;

    st.firmware_version = firmware_metadata_flash->current_firmware_version;
    st.other_firmware_version = firmware_metadata_flash->new_firmware_version;
    st.flags            = nascerr_bootloader_buildFlagsFromMetadata();   /* agora 1 byte completo */
    st.last_error       = bootState.errorCode;        /* mantido em RAM       */
    st.last_packet_id   = g_last_pkg_id;           	/* mantido em RAM       */
    st.current_region   = firmware_metadata_flash->current_firmware_addr == FIRST_APPLICATION_ADDRESS ? 0xAA : 0xBB;
    st.reserved         = 0;

    return nascerr_bootloader_changeBootStatus(&st);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_setLogEndAddress
 *  @brief       : Atualiza no metadata o endereco final da regiao de log e o
 *                 respectivo CRC16 calculado a partir do conteudo da regiao.
 *  @param       : new_end_address – Novo endereco final da regiao de log.
 *  @retval      : 1 se a atualizacao for bem-sucedida.
 *                 0 se ocorrer falha de validacao ou gravacao do metadata.
 *
 *  @note        : Procedimento:
 *                   - Verifica se o campo magic do metadata e valido.
 *                   - Valida se o novo endereco e maior ou igual ao endereco
 *                     inicial da regiao de log.
 *                   - Calcula o tamanho da regiao (new_end_address - FLASH_LOG_START_ADDRESS).
 *                   - Calcula o CRC16 do conteudo dessa regiao.
 *                   - Atualiza os campos end_address_log e crc_log no metadata.
 *                   - Grava o metadata atualizado na flash.
 * ******************************************************************************
 */
uint8_t nascerr_bootloader_setLogEndAddress(uint32_t new_end_address)
{
    firmware_metadata_t md = *firmware_metadata_flash;
    uint32_t            length;
    uint16_t            crc;

    if (firmware_metadata_flash->magic != METADATA_MAGIC) {
        DEBUG_PRINT(LOG_ERROR,
            "BOOT ERROR: metadata corrompida (magic=0x%08lX)\r\n",
            (unsigned long)firmware_metadata_flash->magic
        );
        nascerr_bootloader_setRuntimeError(ERR_BOOT_FLASH_WRITE, LOG_STATUS_ERROR);
        return false;
    }

    if (new_end_address < FLASH_LOG_START_ADDRESS) {
        DEBUG_PRINT(LOG_ERROR,
            "BOOT ERROR: end_address_log invalido 0x%08lX < start 0x%08lX\r\n",
            (unsigned long)new_end_address,
            (unsigned long)FLASH_LOG_START_ADDRESS
        );
        return false;
    }

    length = new_end_address - FLASH_LOG_START_ADDRESS;
    crc = calculate_crc16((uint8_t*)FLASH_LOG_START_ADDRESS, length);

    md.end_address_log = new_end_address;
    md.crc_log         = crc;

    if (!nascerr_bootloader_writeNewMetadata(md)) {
        DEBUG_PRINT(LOG_ERROR,
            "BOOT ERROR: falha ao escrever metadata (log_end=0x%08lX, crc=0x%04X)\r\n",
            (unsigned long)new_end_address,
            (unsigned int)crc
        );
        return false;
    }
#if ENABLE_SERIAL_SYS
    DEBUG_PRINT(LOG_DEBUG,
        "BOOT: log_end_address=0x%08lX, crc_log=0x%04X atualizado\r\n",
        (unsigned long)new_end_address,
        (unsigned int)crc
    );
#endif
    return true;
}

/* local functions implementation */
uint8_t nascerr_bootloader_setRuntimeError(ERROS_LOGS error_code, uint8_t status){
	bootState.errorCode = error_code;
//	nascerr_log_add(LOG_BOOT_OPERATION, status, error_code);
	LOG_AND_NOTIFY(LOG_BOOT_OPERATION, status, error_code);
	return bootState.errorCode;
}

