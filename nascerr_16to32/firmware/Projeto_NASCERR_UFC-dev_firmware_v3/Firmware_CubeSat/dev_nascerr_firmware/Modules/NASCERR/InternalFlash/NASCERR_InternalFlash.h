/**
 * ******************************************************************************
 *  @file    : NASCERR_InternalFlash.h
 *  @brief   : Biblioteca para gravar dados na Flash interna
 *  @author  : Joao Manoel
 *
 * ******************************************************************************
 */
#ifndef STM_FLASH_MEMORY_H
#define STM_FLASH_MEMORY_H

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "main.h"
#include "flash_w25n01g.h"
#include <stdint.h>
#include <string.h>

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */
#define FLASH_START_ADDRESS   			0x08000000U       		/* > Endereco de inicio da Flash interna                  */
#define FLASH_END_ADDRESS     			0x081FFFFFU				/* > Endereco de fim da Flash interna                     */

#define FLASH_BANK1_BASE				FLASH_START_ADDRESS		/* > Endereco base do banco 1 da Flash interna            */
#define FLASH_BANK2_BASE				0x8100000U				/* > Endereco base do banco 2 da Flash interna            */
#define PAGE_SIZE						PAGESIZE

#define EXTERNAL_FLASH_PAGE_SIZE 		W25N01G_PAGE_SIZE       /* > Tamanho de pagina da Flash externa W25N01G           */

#define FLASH_APP1_START_ADDRESS		0x8031000U			   	/* > Endereco de inicio da aplicacao 1 na Flash interna   */
//#define FLASH_APP1_START_ADDRESS		0x8021000U			   	/* > Endereco de inicio da aplicacao 1 na Flash interna   */
#define FLASH_APP2_START_ADDRESS		0x8101000U				/* > Endereco de inicio da aplicacao 2 na Flash interna   */

#define FLASH_LOG_START_ADDRESS			FLASH_BANK2_BASE		/* > Endereco de inicio do buffer de log na Flash interna */
//#define FLASH_METADATA_START_ADDRESS	0x8020000U				/* > Endereco de inicio da metadata na Flash interna      */ //TODO: AJUSTE TÉCNICO PARA AUMETAR ÁREA DE BOOT E CONSEGUIR DEBUGAR COM A APLICAÇÃO ATUAL
#define FLASH_METADATA_START_ADDRESS	0x8030000U				/* > Endereco de inicio da metadata na Flash interna      */


#define BUFFER_BLOCK_SIZE				64						/* > Tamanho do buffer de log em bytes                    */

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

/* ------------------------------------------------------------------------- */
/* Funcoes publicas                                         				 */
/* ------------------------------------------------------------------------- */

uint8_t nascerr_flash_log_append(uint8_t *data, uint32_t size);

uint8_t nascerr_flash_reset_log_buffer(void);

uint16_t nascerr_flash_get_log_occupied_size(void);

uint8_t *nascerr_flash_get_log_address(void);

uint8_t nascerr_flash_write_words(uint32_t address, uint32_t *data, uint32_t size);

uint8_t nascerr_flash_write_bytes(uint32_t address, uint8_t *data, uint32_t size);

uint8_t nascerr_flash_write_doubleword(uint32_t address, uint64_t data);

void nascerr_flash_read(uint32_t address, uint32_t *data, uint32_t size);

uint8_t nascerr_flash_erase_pages(uint32_t address, uint32_t pages);

uint8_t nascerr_flash_init_log_buffer(void);

#endif /* NASCERR_INTERNALFLASH_H */
