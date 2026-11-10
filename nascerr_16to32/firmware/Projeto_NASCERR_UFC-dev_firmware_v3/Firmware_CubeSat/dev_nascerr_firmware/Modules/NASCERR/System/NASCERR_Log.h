/**
 * ******************************************************************************
 *  @file    : NASCERR_Log.h
 *  @brief   : Header for NASCERR_Log.c file.
 *  @author  : Wendell
 * ******************************************************************************
 */

#ifndef NASCERR_INC_NASCERR_LOG_H_
#define NASCERR_INC_NASCERR_LOG_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include <stdint.h>
#include "NASCERR_Util.h"
#include "NASCERR_System.h"
#include "NASCERR_InternalFlash.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

/* Buffer estático */
#define LOG_BUFFER_MAX_SIZE  301 //301

/* Definições de status */
#define LOG_STATUS_SUCCESS 1
#define LOG_STATUS_WARN    2
#define LOG_STATUS_ERROR   3

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */


typedef enum {
    LOG_SUCCESS          = 0x01, /* Operacao bem-sucedida 							 */
	LOG_RECEIVE_COMMAND		   , /* Comando recebido e em processamento 			 */
	LOG_RECEIVE_HW_STAT_REQ	   , /* Comando de recebibo de algum aspecto do hardware */
	LOG_TRANSMIT_HW_RESPONSE   , /* Resposta do HW pronta para ser transmitida 		 */
	LOG_TRANSMIT_HW_FINISH     , /* Comando de hardware trasmitido 					 */
	LOG_HW_WARN				   , /* Warm de Hardware 								 */
	LOG_HW_ERROR			   , /* Erro de Hardware 								 */
	LOG_TRANSMIT_RESPONSE      , /* Resposta em transmissao 						 */
    LOG_TRANSMIT_FINISH        , /* Transmissao finalizada com sucesso 				 */
	LOG_EXP_EXECUT			   , /* Experimento em execucao 						 */
	LOG_EXP_FINNISH			   , /* Experimento em finalizado						 */
	LOG_SYSTEM_BUSY			   , /* Sistema ocupado, nao pode aceitar novo comando   */
	LOG_FAIL				   , /* Operacao falhou (erro generico) 				 */
	LOG_MAX					   , /* Valor maximo 									 */
	LOG_BOOT_INITIALIZED	   , /* Bootloader inicializado 						 */
	LOG_BOOT_RECEIVE_COMMAND   , /* Comando de bootloader recebido 		      		 */
	LOG_BOOT_OPERATION		   , /* Operacao de bootloader em andamento 		     */
	LOG_ROBUST_MEM_HEALTH	   , /* Operacao na memoria robusta em andamento 		 */
	LOG_DB					   , /* Opreações no Banco de dados (SRAM)				 */
	LOG_EXP_RECEIVED		   , /* Experimento recebido						 	 */
	LOG_BOOT				   , /* Operacao de Bootloader							 */
	LOG_CRC					   , /* Operacao de CRC*/
} LOG_TYPE;

typedef enum {
	ERR_NONE 			 = 0x01		, /* Sem erro 													  */
	WARN_COMM_DECODE     	   		, /* Falha na decodificação do comando 							  */
	WARM_HW_STATUS_CODE		   		, /* Tentativa de acesso de status de hardware inexistente  	  */
	ERR_COMM_SENDPKT           		, /* Falha no envio do pacote 									  */
	ERR_EXP_INVALID_DATALEN	   		, /* Experimento interrompido: dado de tamanho invalido     	  */
	ERR_EXP_MEM_OVERFLOW	   		, /* Experimento interrompido: estouro de regiao de memoria 	  */
	ERR_EXP_INVALID_REPETITIONS		, /* Experimento interrompido: numero de repeticoes invalido	  */
	ERR_EXP_INVALID_DELAY	   		, /* Experimento interrompido: valor de delay invalido 			  */
	ERR_EXP_CONFLICT	       		, /* Experimento em conflito							 		  */
	ERR_EXP_SCH_FULL	       		, /* Agendador Full      							 			  */
	ERR_EXP_READ_OUT_OF_RANGE  		, /* Tentativa de leitura fora do intervalo de experimentos 	  */
	ERR_I2C_INIT_FAILED		   		, /* Erro na inicialização da comunicacao I2C 					  */
	ERR_I2C_BUSY               		, /* Comunicacao I2C ocupada 									  */
	ERR_I2C_TRANSFER		   		, /* Erro na transmissao/recepcao I2C 							  */
	ERR_I2C_TIMEOUT			   		, /* Transmissao finalizada por atingir timeout 				  */
	ERR_FLASH_OPERATION		   		, /* Erro de operacao na flash 									  */
	ERR_FLASH_ERASE_FAILED	   		, /* Erro ao apagar bloco na flash 								  */
	ERR_PROTO_CMD_DECODE	   		, /* Falha na decodificacao do comando 							  */
	ERR_PROTO_SEND_PACKET	   		, /* Falha no envio do pacote 									  */
	ERR_COMM_GENERAL		   		, /* Erro de comunicacao generico 								  */
	ERR_CMD_INVALID            		, /* Comando invalido ou desconhecido 							  */
	ERR_CMD_OVERFLOW           		, /* Estouro na fila de Comando 								  */
	ERR_CMD_CRC                		, /* Erro no crc do comando  									  */
	ERR_DB_INIT                		, /* Erro de inicializacao do banco de dados					  */
	ERR_DB_WRITE               		, /* Erro de escrita no banco de dados							  */
	ERR_BOOT_CMD_SIZE		   		, /* Erro de tamanho de comando de bootloader 					  */
	ERR_BOOT_CMD_INIT		   		, /* Erro de inicializacao de comando de bootloader 			  */
	ERR_BOOT_CMD_DECODE		   		, /* Erro de decodificacao de comando de bootloader 			  */
	ERR_BOOT_FLASH_WRITE	   		, /* Erro de escrita na flash do bootloader 					  */
	ERR_BOOT_FLASH_ERASE	   		, /* Erro de apagamento na flash do bootloader 					  */
	ERR_BOOT_FLASH_WRITE_SIZE  		, /* Erro de tamanho de escrita na flash do bootloader 			  */
	ERR_BOOT_FLASH_CRC		   		, /* Erro de CRC na flash do bootloader 						  */
	ERR_DB_READ                		, /* Erro de leitura no banco de dados							  */
	ERR_BOOT_PKG_OUT_OF_SEQ    		, /* Erro de pacote fora de sequencia no bootloader 			  */
	ERR_BOOT_PKG_INVALID_VER   		, /* Erro de versao invalida no bootloader 						  */
	ERR_BOOT_TIMEOUT		   		, /* Timeout no bootloader 										  */
	WARN_HW_CURRENT_THRESHOLD  		, /* Corrente superou o limiar estipulado						  */
    ERR_BOOT_INVALID_METADATA  		, /* Metadados do firmware invalidos							  */
    ERR_BOOT_SHA_MISMATCH	   		, /* SHA-256 do firmware nao confere							  */
    ERR_BOOT_UNRECOVERABLE	   		, /* Erro critico no bootloader									  */
    WARN_HW_TEMPERATURE_THRESHOLD 	, /* Temperatura superior o limite estipulado					  */
	WARN_ROBUST_MEM_SRAM_1			, /* Indica que houve corrupção de um novo bloco na SRAM 1		  */
	WARN_ROBUST_MEM_SRAM_2			, /* Indica que houve corrupção de um novo bloco na SRAM 2		  */
	WARN_ROBUST_MEM_ENC_DEC			, /* Indica que houve um erro nos encoders/decoders				  */
	ERR_ROBUST_MEM_ENC_DEC			, /* Indica que houve a saturação nos erros nos encoders/decoders */
	WARM_IWDG_RESET					, /* Reset provocado pelo estouro do Independent Watchdog 		  */
	ERR_SPI_TIMEOUT					, /* SPI: timeout aguardando flash pronta (waitForReady) 		  */
	ERR_SPI_TRANSMIT				, /* SPI: falha na transmissao ou recepcao de dados 			  */
	ERR_SPI_ECC_UNCORRECTABLE		, /* SPI NAND: erro ECC nao corrigivel na leitura de pagina 	  */
	ERR_ADC_CALIBRATION				, /* ADC: falha na auto-calibracao 							  */
	ERR_ADC_START					, /* ADC: falha ao iniciar conversao DMA 					  */
	ERR_ADC_STOP					, /* ADC: falha ao parar conversao DMA 						  */
	ERR_TIM_START					, /* Timer: falha ao iniciar 								  */
	ERR_TIM_STOP					, /* Timer: falha ao parar 									  */
	ERR_FMC_OVERFLOW				, /* FMC/SRAM: acesso alem do limite da regiao de memoria 	  */
	ERR_UART_DMA					, /* UART: erro de hardware DMA (FE/NE/ORE) 				  */
} ERROS_LOGS;


typedef struct __attribute__((packed)) {
    uint32_t timestamp;     /* Tempo em ms desde o boot 				 			 */
    LOG_TYPE type; 			/* Tipo de operacao 						 			 */
    uint8_t status;         /* LOG_STATUS_SUCCESS, LOG_STATUS_WARN, LOG_STATUS_ERROR */
    ERROS_LOGS error_code;  /* Código de erro ou resultado 				 			 */
} NASCERR_LOG;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

void nascerr_log_add(LOG_TYPE type, uint8_t status, ERROS_LOGS code);
uint16_t nascerr_log_get_count(void);
uint16_t nascerr_log_get_buffer_size(void);
NASCERR_LOG* nascerr_log_get(void);
void nascerr_log_clear(void);

#endif /* NASCERR_INC_NASCERR_LOG_H_ */
