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
/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */
static uint8_t 			g_runtime_flags = 0;			/* Runtime flags for bootloader state */
static BootloaderFSM_State current_state;
ERROS_LOGS last_error_code = ERR_NONE;

extern HASH_HandleTypeDef hhash;
extern UART_HandleTypeDef huart4;
extern SPI_HandleTypeDef hspi3;
extern I2C_HandleTypeDef hi2c1;

firmware_metadata_t *firmware_metadata_flash = (firmware_metadata_t *)METADATA_ADDRESS; 	/* Pointer to the firmware metadata in flash */

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */
static void jump_to_application(uint32_t app_address);

uint8_t nascerr_bootloader_setRuntimeError(ERROS_LOGS error_code);

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
*/

/*
 * ******************************************************************************
 * Function Name: nascerr_bootloader_FSM_Init
 * @brief      : Inicializa a Máquina de Estados Finitos (FSM) do bootloader,
 * definindo seu estado inicial.
 * @param      : Nenhum
 * @retval     : Nenhum
 * ******************************************************************************
 */
void nascerr_bootloader_FSM_Init(void)
{
    current_state = BL_STATE_CHECK_METADATA;
}

/*
 * ******************************************************************************
 * Function Name: nascerr_bootloader_FSM_Run
 * @brief       : Executa a lógica do estado atual da Máquina de Estados Finitos
 * 				: (FSM) do bootloader. Esta função gerencia o fluxo de
 * 				: inicialização, desde a verificação de metadados, passando
 * 				: pela atualização de firmware (se pendente), até o salto
 * 				: para a aplicação principal. Deve ser chamada em loop.
 * @param      : Nenhum
 * @retval     : Nenhum
 * ******************************************************************************
 */
void nascerr_bootloader_FSM_Run(void)
{
    switch (current_state)
    {
        case BL_STATE_CHECK_METADATA:
        {
            uint16_t calculated_crc = calculate_crc16(
                (uint8_t*)firmware_metadata_flash,
                sizeof(firmware_metadata_t) - sizeof(firmware_metadata_flash->crc_metadata)
            );

            if (firmware_metadata_flash->magic == METADATA_MAGIC &&
                firmware_metadata_flash->crc_metadata == calculated_crc)
            {
                BL_LOG(BL_STATE_CHECK_METADATA, LOG_INFO, "Metadados validos. Indo para INIT\r\n");
                nascerr_bootloader_initFlags();
                current_state = BL_STATE_INIT;
            }
            else
            {
            	nascerr_bootloader_setRuntimeError(ERR_BOOT_INVALID_METADATA);
                BL_LOG(BL_STATE_CHECK_METADATA, LOG_WARN, "Metadados invalidos. Tentando recuperar...\r\n");
                current_state = BL_STATE_RECOVER_METADATA;
            }
            break;
        }

        case BL_STATE_RECOVER_METADATA:
        {
            const application_signature_t* app_sig =
                (const application_signature_t*)(FIRST_APPLICATION_ADDRESS + VT_OFFSET);

            if (app_sig->magic == APP_MAGIC)
            {
                BL_LOG(BL_STATE_RECOVER_METADATA, LOG_INFO, "Assinatura valida. Criando novos metadados...\r\n");
                current_state = BL_STATE_CREATE_METADATA;
            }
            else
            {
                BL_LOG(BL_STATE_RECOVER_METADATA, LOG_ERROR, "Assinatura invalida. Falha critica!\r\n");
                current_state = BL_STATE_FAILURE;
            }
            break;
        }

        case BL_STATE_CREATE_METADATA:
        {
            const application_signature_t* app_sig =
                (const application_signature_t*)(FIRST_APPLICATION_ADDRESS + VT_OFFSET);
            firmware_metadata_t new_md;
            uint8_t calculated_sha[32];

            if (nascerr_bootloader_calculateSha256((uint8_t*)FIRST_APPLICATION_ADDRESS,
                                                   app_sig->size,
                                                   calculated_sha))
            {
                new_md.magic = METADATA_MAGIC;
                new_md.current_firmware_addr = FIRST_APPLICATION_ADDRESS;
                new_md.current_firmware_size = app_sig->size;
                new_md.current_firmware_version = app_sig->version;
                memcpy(new_md.current_firmware_sha, calculated_sha, 32);

                new_md.new_firmware_addr = SECOND_APPLICATION_ADDRESS;
                new_md.new_firmware_size = 0;
                new_md.new_firmware_version = 0;
                memset(new_md.new_firmware_sha, 0xFF, 32);

                new_md.boot_flag = 0;
                new_md.error_flag = 0;

                if (nascerr_bootloader_writeNewMetadata(new_md))
                {
                    BL_LOG(BL_STATE_CREATE_METADATA, LOG_INFO, "Metadados criados com sucesso\r\n");
                    nascerr_bootloader_initFlags();
                    current_state = BL_STATE_BACKUP_FIRMWARE;
                    break;
                }
            }
            nascerr_bootloader_setRuntimeError(ERR_BOOT_FLASH_WRITE);
            BL_LOG(BL_STATE_CREATE_METADATA, LOG_ERROR, "Falha ao criar metadados\r\n");
            current_state = BL_STATE_FAILURE;
            break;
        }

        case BL_STATE_INIT:
        {
            if (firmware_metadata_flash->boot_flag & BOOT_FLAG_UPDATE_PENDING)
            {
                BL_LOG(BL_STATE_INIT, LOG_INFO, "Update pendente. Indo verificar integridade\r\n");
                current_state = BL_STATE_VERIFY_INTEGRITY;
            }
            else
            {
                if (!nascerr_bootloader_hasValidBackup())
                {
                    BL_LOG(BL_STATE_INIT, LOG_WARN, "Sem backup valido detectado. Executando backup inicial...\r\n");
                    current_state = BL_STATE_BACKUP_FIRMWARE;
                }
                else
                {
                    BL_LOG(BL_STATE_INIT, LOG_INFO, "Nenhum update pendente e backup valido presente. Pulando para aplicacao\r\n");
                    current_state = BL_STATE_VERIFY_CURRENT;
                }

            }
            break;
        }

        case BL_STATE_VERIFY_INTEGRITY:
        {
            uint8_t calculated_sha[32];

            if (nascerr_bootloader_calculateSha256(
                    (uint8_t*)firmware_metadata_flash->new_firmware_addr,
                    firmware_metadata_flash->new_firmware_size,
                    calculated_sha))
            {
                if (memcmp(calculated_sha, firmware_metadata_flash->new_firmware_sha, 32) == 0)
                {
                    BL_LOG(BL_STATE_VERIFY_INTEGRITY, LOG_INFO, "Integridade confirmada\r\n");
                    current_state = BL_STATE_BACKUP_FIRMWARE;
                }
                else
                {
                	nascerr_bootloader_setRuntimeError(ERR_BOOT_SHA_MISMATCH);
                    BL_LOG(BL_STATE_VERIFY_INTEGRITY, LOG_ERROR, "SHA invalido. Falha na verificacao\r\n");
                    current_state = BL_STATE_DISCARD_UPDATE;
                }
            }
            else
            {
				nascerr_bootloader_setRuntimeError(ERR_BOOT_SHA_MISMATCH);
                BL_LOG(BL_STATE_VERIFY_INTEGRITY, LOG_ERROR, "Erro ao calcular SHA\r\n");
                current_state = BL_STATE_DISCARD_UPDATE;
            }
            break;
        }

        case BL_STATE_VERIFY_CURRENT: {
             uint8_t sha[32];
             if (nascerr_bootloader_calculateSha256((uint8_t*)firmware_metadata_flash->current_firmware_addr,
                              firmware_metadata_flash->current_firmware_size, sha) &&
                 memcmp(sha, firmware_metadata_flash->current_firmware_sha, 32) == 0)
             {
                 BL_LOG(BL_STATE_VERIFY_CURRENT, LOG_INFO, "SHA atual OK -> JUMP_TO_APP\r\n");
                 current_state = BL_STATE_JUMP_TO_APP;
             } else {
                 nascerr_bootloader_setRuntimeError(ERR_BOOT_SHA_MISMATCH);
                 BL_LOG(BL_STATE_VERIFY_CURRENT, LOG_ERROR, "SHA atual invalido\r\n");
                 if(nascerr_bootloader_canRollback()) {
					 BL_LOG(BL_STATE_VERIFY_CURRENT, LOG_WARN, "Rollback disponivel \r\n");
					 current_state = BL_STATE_ROLLBACK;
				 } else {
					 BL_LOG(BL_STATE_VERIFY_CURRENT, LOG_ERROR, "Rollback indisponivel \r\n");
					 current_state = BL_STATE_FAILURE;
				 }
             }
             break;
         }

        case BL_STATE_DISCARD_UPDATE:
        {
            BL_LOG(BL_STATE_DISCARD_UPDATE, LOG_WARN, "Descartando nova versao invalida. Mantendo firmware atual\r\n");

            firmware_metadata_t temp_md;
            memcpy(&temp_md, (void*)firmware_metadata_flash, sizeof(firmware_metadata_t));

            temp_md.new_firmware_addr    = SECOND_APPLICATION_ADDRESS;
            temp_md.new_firmware_size    = 0;
            temp_md.new_firmware_version = 0;
            memset(temp_md.new_firmware_sha, 0xFF, 32);

            nascerr_bootloader_resetRuntimeFlag(BOOT_FLAG_UPDATE_PENDING);
            nascerr_bootloader_resetRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
            temp_md.boot_flag = g_runtime_flags;

            if (nascerr_bootloader_writeNewMetadata(temp_md))
            {
                BL_LOG(BL_STATE_DISCARD_UPDATE, LOG_INFO, "Metadados atualizados. Saltando para aplicacao\r\n");
                current_state = BL_STATE_JUMP_TO_APP;
            }
            else
            {
            	nascerr_bootloader_setRuntimeError(ERR_DB_WRITE);
                BL_LOG(BL_STATE_DISCARD_UPDATE, LOG_ERROR, "Erro ao salvar metadados. Entrando em falha critica\r\n");
                current_state = BL_STATE_CRITICAL_FAILURE;
            }
            break;
        }

        case BL_STATE_BACKUP_FIRMWARE:
        {
            BL_LOG(BL_STATE_BACKUP_FIRMWARE, LOG_INFO, "Iniciando backup do firmware atual...\r\n");

            if (!nascerr_db_clearBootArea())
            {
            	nascerr_bootloader_setRuntimeError(ERR_DB_INIT);
            	nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
                BL_LOG(BL_STATE_BACKUP_FIRMWARE, LOG_ERROR, "Falha ao limpar area de backup\r\n");
                current_state = BL_STATE_FAILURE;
                break;
            }

            nascerr_db_updateBootVersion(firmware_metadata_flash->current_firmware_version);
            nascerr_db_updateBootSha(firmware_metadata_flash->current_firmware_sha);

            FLASHTypeDef write_result = nascerr_db_writeBootData(
                (uint8_t *)0x8021000,
                firmware_metadata_flash->current_firmware_size
            );

            if (write_result.error)
            {
            	nascerr_bootloader_setRuntimeError(ERR_DB_WRITE);
            	nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
                BL_LOG(BL_STATE_BACKUP_FIRMWARE, LOG_ERROR, "Erro ao escrever backup na flash externa\r\n");
                current_state = BL_STATE_FAILURE;
                break;
            }

            if (!nascerr_db_commitHeader(write_result.end_addr))
            {
            	nascerr_bootloader_setRuntimeError(ERR_DB_WRITE);
            	nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
                BL_LOG(BL_STATE_BACKUP_FIRMWARE, LOG_ERROR, "Erro ao fazer commit do header do backup\r\n");
                current_state = BL_STATE_FAILURE;
                break;
            }

            BL_LOG(BL_STATE_BACKUP_FIRMWARE, LOG_INFO, "Backup concluido com sucesso\r\n");
            nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ROLLBACK_AVAILABLE);
            nascerr_bootloader_updateBootFlags();
            if (firmware_metadata_flash->new_firmware_size == 0)
            {
                BL_LOG(BL_STATE_BACKUP_FIRMWARE, LOG_INFO, "Nenhum firmware novo. Pulando para aplicacao\r\n");
                current_state = BL_STATE_JUMP_TO_APP;
                break;
            }

            current_state = BL_STATE_ERASE_TARGET_BANK;
            break;
        }

        case BL_STATE_ERASE_TARGET_BANK:
        {
            if (nascerr_bootloader_eraseFlash(firmware_metadata_flash->new_firmware_size,
                                              firmware_metadata_flash->current_firmware_addr))
            {
                BL_LOG(BL_STATE_ERASE_TARGET_BANK, LOG_INFO, "Banco de destino apagado\r\n");
                current_state = BL_STATE_COPY_FIRMWARE;
            }
            else
            {
            	nascerr_bootloader_setRuntimeError(ERR_BOOT_FLASH_ERASE);
                BL_LOG(BL_STATE_ERASE_TARGET_BANK, LOG_ERROR, "Erro ao apagar banco de destino\r\n");
                current_state = BL_STATE_ROLLBACK;
            }
            break;
        }

        case BL_STATE_COPY_FIRMWARE:
        {
            uint32_t source_addr = firmware_metadata_flash->new_firmware_addr;
            uint32_t dest_addr   = firmware_metadata_flash->current_firmware_addr;
            uint32_t total_size  = firmware_metadata_flash->new_firmware_size;
            uint8_t buffer[256];
            uint8_t success = 1;

            for (uint32_t offset = 0; offset < total_size; offset += sizeof(buffer))
            {
                uint32_t chunk_size = (total_size - offset < sizeof(buffer)) ? (total_size - offset) : sizeof(buffer);
                memcpy(buffer, (void*)(source_addr + offset), chunk_size);

                if (!nascerr_bootloader_writeToFlash(dest_addr + offset, buffer, chunk_size))
                {
                	nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
                    success = 0;
                    break;
                }
            }

            if (success)
            {
                BL_LOG(BL_STATE_COPY_FIRMWARE, LOG_INFO, "Copia do novo firmware concluida\r\n");
                current_state = BL_STATE_VERIFY_COPY;
            }
            else
            {
            	nascerr_bootloader_setRuntimeError(ERR_BOOT_FLASH_WRITE);
                BL_LOG(BL_STATE_COPY_FIRMWARE, LOG_ERROR, "Erro ao copiar firmware\r\n");
                current_state = BL_STATE_ROLLBACK;
            }
            break;
        }

        case BL_STATE_VERIFY_COPY:
        {
            uint8_t calculated_sha[32];
            if (nascerr_bootloader_calculateSha256(
                    (uint8_t*)firmware_metadata_flash->current_firmware_addr,
                    firmware_metadata_flash->new_firmware_size,
                    calculated_sha))
            {
                if (memcmp(calculated_sha, firmware_metadata_flash->new_firmware_sha, 32) == 0)
                {
                	nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_IMAGE_VALID);
                    BL_LOG(BL_STATE_VERIFY_COPY, LOG_INFO, "Novo firmware validado com sucesso\r\n");
                    current_state = BL_STATE_FINALIZE_SUCCESS;
                }
                else
                {
                	nascerr_bootloader_setRuntimeError(ERR_BOOT_SHA_MISMATCH);
                    BL_LOG(BL_STATE_VERIFY_COPY, LOG_ERROR, "Validacao falhou. SHA nao bate\r\n");
                    current_state = BL_STATE_ROLLBACK;
                }
            }
            else
            {
            	nascerr_bootloader_setRuntimeError(ERR_BOOT_SHA_MISMATCH);
                BL_LOG(BL_STATE_VERIFY_COPY, LOG_ERROR, "Erro ao calcular SHA para validacao\r\n");
                current_state = BL_STATE_ROLLBACK;
            }
            break;
        }

        case BL_STATE_FINALIZE_SUCCESS:
        {
            firmware_metadata_t temp_md;
            memcpy(&temp_md, (void*)firmware_metadata_flash, sizeof(firmware_metadata_t));

            temp_md.current_firmware_addr    = FIRST_APPLICATION_ADDRESS;
            temp_md.current_firmware_size    = temp_md.new_firmware_size;
            temp_md.current_firmware_version = temp_md.new_firmware_version;
            memcpy(temp_md.current_firmware_sha, temp_md.new_firmware_sha, 32);

            temp_md.new_firmware_addr    = SECOND_APPLICATION_ADDRESS;
            temp_md.new_firmware_size    = 0;
            temp_md.new_firmware_version = 0;
            memset(temp_md.new_firmware_sha, 0xFF, 32);

            nascerr_bootloader_resetRuntimeFlag(BOOT_FLAG_UPDATE_PENDING);
            nascerr_bootloader_resetRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
            temp_md.boot_flag = g_runtime_flags;

            if (nascerr_bootloader_writeNewMetadata(temp_md))
            {
                BL_LOG(BL_STATE_FINALIZE_SUCCESS, LOG_INFO, "Atualizacao concluida. Reiniciando sistema\r\n");
                nascerr_bootloader_updateBootFlags();
                HAL_NVIC_SystemReset();
            }
            else
            {
            	nascerr_bootloader_setRuntimeError(ERR_BOOT_FLASH_WRITE);
                BL_LOG(BL_STATE_FINALIZE_SUCCESS, LOG_ERROR, "Erro ao salvar metadados finais\r\n");
                current_state = BL_STATE_ROLLBACK;
            }
            break;
        }

        case BL_STATE_ROLLBACK:
        {
            BL_LOG(BL_STATE_ROLLBACK, LOG_ERROR, "Iniciando rollback...\r\n");

            if (!nascerr_db_readHeader())
            {
                BL_LOG(BL_STATE_ROLLBACK, LOG_ERROR, "Nao foi possivel ler header de backup\r\n");
                current_state = BL_STATE_CRITICAL_FAILURE;
                break;
            }

            uint32_t backup_size = experimentHeader.boot_data.header.data_load;
            uint32_t old_version = experimentHeader.boot_data.version;
            uint8_t old_sha[32];
            memcpy(old_sha, experimentHeader.boot_data.hash_256, 32);

            if (!nascerr_bootloader_eraseFlash(backup_size, FIRST_APPLICATION_ADDRESS))
            {
                nascerr_bootloader_setRuntimeError(ERR_BOOT_FLASH_ERASE);
                BL_LOG(BL_STATE_ROLLBACK, LOG_ERROR, "Falha ao apagar banco durante rollback\r\n");
                current_state = BL_STATE_CRITICAL_FAILURE;
                break;
            }

            uint8_t buffer[256];
            uint8_t success = 1;
            for (uint32_t offset = 0; offset < backup_size; offset += sizeof(buffer))
            {
                uint32_t chunk_size = (backup_size - offset < sizeof(buffer)) ? (backup_size - offset) : sizeof(buffer);

                if (!nascerr_db_readBootData(offset, buffer, chunk_size) ||
                    !nascerr_bootloader_writeToFlash(FIRST_APPLICATION_ADDRESS + offset, buffer, chunk_size))
                {
                    success = 0;
                    break;
                }
            }

            if (success)
            {
                BL_LOG(BL_STATE_ROLLBACK, LOG_INFO, "Rollback concluido. Atualizando metadados...\r\n");

                firmware_metadata_t temp_md;
                memcpy(&temp_md, (void*)firmware_metadata_flash, sizeof(firmware_metadata_t));

                temp_md.current_firmware_addr    = FIRST_APPLICATION_ADDRESS;
                temp_md.current_firmware_size    = backup_size;
                temp_md.current_firmware_version = old_version;
                memcpy(temp_md.current_firmware_sha, old_sha, 32);

                // Limpa os campos de firmware novo (já foi descartado)
                temp_md.new_firmware_addr    = SECOND_APPLICATION_ADDRESS;
                temp_md.new_firmware_size    = 0;
                temp_md.new_firmware_version = 0;
                memset(temp_md.new_firmware_sha, 0xFF, 32);

                // Flags: indica rollback feito com sucesso
                nascerr_bootloader_resetRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
                nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ROLLBACK_AVAILABLE);
                nascerr_bootloader_resetRuntimeFlag(BOOT_FLAG_UPDATE_PENDING);
                temp_md.boot_flag = g_runtime_flags;

                // === Salva metadados corrigidos ===
                if (!nascerr_bootloader_writeNewMetadata(temp_md))
                {
                    nascerr_bootloader_setRuntimeError(ERR_DB_WRITE);
                    BL_LOG(BL_STATE_ROLLBACK, LOG_ERROR, "Erro ao salvar metadados apos rollback\r\n");
                    current_state = BL_STATE_CRITICAL_FAILURE;
                    break;
                }

                BL_LOG(BL_STATE_ROLLBACK, LOG_INFO, "Metadados atualizados com SHA da versao restaurada\r\n");
                nascerr_bootloader_updateBootFlags();
                HAL_NVIC_SystemReset();
            }
            else
            {
                nascerr_bootloader_setRuntimeError(ERR_BOOT_FLASH_WRITE);
                BL_LOG(BL_STATE_ROLLBACK, LOG_ERROR, "Falha ao copiar dados no rollback\r\n");
                current_state = BL_STATE_CRITICAL_FAILURE;
            }
            break;
        }

        case BL_STATE_JUMP_TO_APP:
        {
        	nascerr_bootloader_updateBootFlags();
            BL_LOG(BL_STATE_JUMP_TO_APP, LOG_INFO, "Saltando para aplicacao\r\n");
            jump_to_application(firmware_metadata_flash->current_firmware_addr);
            break;
        }

        case BL_STATE_FAILURE:
        {
            nascerr_bootloader_resetRuntimeFlag(BOOT_ALL_FLAGS);
            BL_LOG(BL_STATE_FAILURE, LOG_ERROR, "Estado de Falha critica\r\n");

            if (nascerr_bootloader_canRollback())
            {
                BL_LOG(BL_STATE_FAILURE, LOG_WARN, "Backup valido encontrado. Tentando rollback...\r\n");
                current_state = BL_STATE_ROLLBACK;
            }
            else
            {
                nascerr_bootloader_setRuntimeError(ERR_BOOT_UNRECOVERABLE);
                nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
                BL_LOG(BL_STATE_FAILURE, LOG_ERROR, "Erro Irrecuperavel\r\n");
                BL_LOG(BL_STATE_FAILURE, LOG_ERROR, "Sem backup disponivel. Entrando em falha critica\r\n");
                current_state = BL_STATE_CRITICAL_FAILURE;
            }
            break;
        }

        case BL_STATE_CRITICAL_FAILURE:
        {
        	nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);

            BL_LOG(BL_STATE_CRITICAL_FAILURE, LOG_ERROR, "Falha critica. Entrando em loop de seguranca\r\n");
            while (1)
            {
                blink_led(1000);
            }
            break;
        }
    }
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_hasValidBackup
 *  @brief       : Verifica se existe um backup valido do firmware na flash externa.
 *  @param       : Nenhum
 *  @retval      : Nenhum
 * ******************************************************************************
 */
uint8_t nascerr_bootloader_hasValidBackup(void)
{
    if (!nascerr_db_readHeader()) return false;

    if (experimentHeader.boot_data.version == 0) return false;
    if (experimentHeader.boot_data.header.data_load == 0) return false;

    return true;
}

/*
 * ******************************************************************************
 * Function Name: nascerr_bootloader_canRollback
 * @brief       : Verifica se o sistema pode realizar um rollback para o firmware
 * 			   de backup.
 * @param       : Nenhum
 * @retval      : 1 se o rollback for possivel.
 */
uint8_t nascerr_bootloader_canRollback(void)
{

    if (!nascerr_bootloader_hasValidBackup())
        return false;

    return true;
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
	uint32_t pagesErase = (sizeApp / PAGE_SIZE);

	if(sizeApp % PAGE_SIZE) pagesErase+=1;

	return nascerr_flash_erase_pages(addrApp, pagesErase);;
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
    	return false;
    }

	if(!nascerr_flash_write_bytes(address, data, pkgSize))
		return false;
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
    nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_IN_BOOTLOADER);
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
	g_runtime_flags &= ~(uint8_t)f;
	nascerr_bootloader_setRuntimeFlag(BOOT_FLAG_IN_BOOTLOADER);
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
uint8_t nascerr_bootloader_buildFlagsFromMetadata(void)
{
    return g_runtime_flags;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bootloader_setCurrentVersion
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
uint8_t nascerr_bootloader_setCurrentVersion(uint32_t new_version, uint32_t new_version_size)
{
	firmware_metadata_t buffer;

	memcpy(&buffer, (void *)firmware_metadata_flash, sizeof(firmware_metadata_t));

	if(buffer.magic != firmware_metadata_flash->magic)
		return false;

	buffer.current_firmware_version = new_version;
	buffer.current_firmware_size = new_version_size;

	if(!nascerr_bootloader_writeNewMetadata(buffer))
		return false;

	return true;
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
    if (g_runtime_flags == 0 || g_runtime_flags == 0xFF)
        return true;

    firmware_metadata_t tmp;
    memcpy(&tmp, (void*)firmware_metadata_flash, sizeof(tmp));

    tmp.boot_flag = g_runtime_flags;

    if (!nascerr_bootloader_writeNewMetadata(tmp))
        return false;

    g_runtime_flags = firmware_metadata_flash->boot_flag;
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
    st.flags            = nascerr_bootloader_buildFlagsFromMetadata();   									/* agora 1 byte completo */
    st.last_error       = last_error_code;        															/* mantido em RAM       */
    st.last_packet_id   = 0xFFFF;           																/* esta em modo bootloader      */
    st.current_region   = firmware_metadata_flash->current_firmware_addr == FIRST_APPLICATION_ADDRESS ? 0xAA : 0xBB;
    st.reserved         = 0;

    return nascerr_bootloader_changeBootStatus(&st);
}

//--------------------------Local Functions----------------------------//

/*
 * ******************************************************************************
 * Function Name: jump_to_application
 * @brief       : Desinicializa os periféricos e interrupções do sistema atual,
 * 				  reconfigura o ponteiro de pilha (MSP) e o vetor de
 * 				  interrupções (VTOR) e, por fim, transfere o controle de
 * 				  execução para uma nova aplicação em um endereço específico.
 * @param       : app_address - Endereço de memória inicial da aplicação para a
 * 				  qual o controle será transferido.
 * @retval      : Nenhum. A função não retorna; em caso de sucesso, o controle
 * 				  é transferido. Em caso de endereço inválido, o sistema entra
 * 				  em loop infinito.
 * ******************************************************************************
 */
static void jump_to_application(uint32_t app_address)
{
    if (app_address < FIRST_APPLICATION_ADDRESS || app_address > (SECOND_APPLICATION_ADDRESS + 0x000E0000))
    {
        while(1);
    }

    HAL_HASH_DeInit(&hhash);
    HAL_UART_DeInit(&huart4);
    HAL_SPI_DeInit(&hspi3);
    HAL_I2C_DeInit(&hi2c1);
    HAL_RCC_DeInit();
    HAL_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;

    for (uint32_t i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }

    SCB->VTOR = app_address;

    uint32_t jump_address = *(volatile uint32_t*)(app_address + 4);
    void (*app_reset_handler)(void) = (void*)jump_address;

    __set_MSP(*(volatile uint32_t*)app_address);

    app_reset_handler();
}

uint8_t nascerr_bootloader_setRuntimeError(ERROS_LOGS error_code){
	last_error_code = error_code;
	return last_error_code;
}

