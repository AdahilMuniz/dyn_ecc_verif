/**
 * ******************************************************************************
 *  @file    : nascerr_system.h
 *  @brief   : Header for system settings.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

#ifndef INC_NASCERR_SYSTEM_H_
#define INC_NASCERR_SYSTEM_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include <stdint.h>

/*
 * ******************************************************************************
 *  SYSTEM Params
 * ******************************************************************************
 */
typedef enum {
	ERR_NONE 			 = 0x01, /* Sem erro 												*/
	WARN_COMM_DECODE     	   , /* Falha na decodificação do comando 						*/
	WARM_HW_STATUS_CODE		   , /* Tentativa de acesso de status de hardware inexistente   */
	ERR_COMM_SENDPKT           , /* Falha no envio do pacote 								*/
	ERR_EXP_INVALID_DATALEN	   , /* Experimento interrompido: dado de tamanho invalido      */
	ERR_EXP_MEM_OVERFLOW	   , /* Experimento interrompido: estouro de regiao de memoria 	*/
	ERR_EXP_INVALID_REPETITIONS, /* Experimento interrompido: numero de repeticoes invalido */
	ERR_EXP_INVALID_DELAY	   , /* Experimento interrompido: valor de delay invalido 		*/
	ERR_EXP_CONFLICT	       , /* Experimento em conflito							 		*/
	ERR_EXP_SCH_FULL	       , /* Agendador Full      							 		*/
	ERR_EXP_READ_OUT_OF_RANGE  , /* Tentativa de leitura fora do intervalo de experimentos 	*/
	ERR_I2C_INIT_FAILED		   , /* Erro na inicialização da comunicacao I2C 				*/
	ERR_I2C_BUSY               , /* Comunicacao I2C ocupada 								*/
	ERR_I2C_TRANSFER		   , /* Erro na transmissao/recepcao I2C 						*/
	ERR_I2C_TIMEOUT			   , /* Transmissao finalizada por atingir timeout 				*/
	ERR_FLASH_OPERATION		   , /* Erro de operacao na flash 								*/
	ERR_FLASH_ERASE_FAILED	   , /* Erro ao apagar bloco na flash 							*/
	ERR_PROTO_CMD_DECODE	   , /* Falha na decodificacao do comando 						*/
	ERR_PROTO_SEND_PACKET	   , /* Falha no envio do pacote 								*/
	ERR_COMM_GENERAL		   , /* Erro de comunicacao generico 							*/
	ERR_CMD_INVALID            , /* Comando invalido ou desconhecido 						*/
	ERR_CMD_OVERFLOW           , /* Estouro na fila de Comando 								*/
	ERR_CMD_CRC                , /* Erro no crc do comando  								*/
	ERR_DB_INIT                , /* Erro de inicializacao do banco de dados				    */
	ERR_DB_WRITE               , /* Erro de escrita no banco de dados						*/
	ERR_BOOT_CMD_SIZE		   , /* Erro de tamanho de comando de bootloader 				*/
	ERR_BOOT_CMD_INIT		   , /* Erro de inicializacao de comando de bootloader 			*/
	ERR_BOOT_CMD_DECODE		   , /* Erro de decodificacao de comando de bootloader 			*/
	ERR_BOOT_FLASH_WRITE	   , /* Erro de escrita na flash do bootloader 					*/
	ERR_BOOT_FLASH_ERASE	   , /* Erro de apagamento na flash do bootloader 				*/
	ERR_BOOT_FLASH_WRITE_SIZE  , /* Erro de tamanho de escrita na flash do bootloader 		*/
	ERR_BOOT_FLASH_CRC		   , /* Erro de CRC na flash do bootloader 						*/
	ERR_DB_READ                , /* Erro de leitura no banco de dados						*/
	ERR_BOOT_PKG_OUT_OF_SEQ    , /* Erro de pacote fora de sequencia no bootloader 			*/
	ERR_BOOT_PKG_INVALID_VER   , /* Erro de versao invalida no bootloader 					*/
	ERR_BOOT_TIMEOUT		   , /* Timeout no bootloader 									*/
	WARN_HW_CURRENT_THRESHOLD  , /* Corrente superou o limiar estipulado					*/
	ERR_BOOT_INVALID_METADATA  , /* Metadata invalido no bootloader 						*/
	ERR_BOOT_SHA_MISMATCH	   , /* SHA do firmware nao confere com o esperado				*/
	ERR_BOOT_UNRECOVERABLE     , /* Erro critico nao recuperavel no bootloader				*/
} ERROS_LOGS;

/* Parameter Standard */
typedef enum
{
    Clear = 0,
    Set = 1,
	Open = 0,
    Close = 1,
	Min = 0,
	Max = 1,
    Turn_off = 0,
    Turn_on = 1,
	Action_Failed = 0,
	Action_Succeeded = 1,
} PARAM_STANDARD;

/* System Status */
typedef enum
{
	ERR_UNKNOWN = 0,
	STATUS_OK,
	STATUS_SUCCESS,
	ERR_INIT,
	ERR_BUSY,
	ERR_COMM,
	ERR_IO,
	ERR_TIMEOUT,
	ERR_FLASH,
	ERR_FLASH_ERASE
} SYSTEM_STATUS;

/*
 * ******************************************************************************
 *  NASCERR Params
 * ******************************************************************************
 */



/* Operating Logs */
typedef enum
{
	OP_EXP_OK = 0xA0,
	OP_EXP_BUSY,
	OP_EXP_RECEIVE_COMMAND,
	OP_EXP_TRANSMIT_RESPONSE,
	OP_EXP_TRANSMIT_FINISH,
	OP_EXP_FINISH,
	OP_EXP_FAIL
} OP_EXPERIMENT;

typedef struct
{
	uint16_t id;
	uint16_t page;
	OP_EXPERIMENT stat;
	uint8_t error_flag;
}OP_EXP_STRUCT;

typedef enum
{
	OP_HW_OK = 0xB0,
	OP_HW_SLEEP,
	OP_HW_RECEIVED_COMMAND,
	OP_HW_TRANSMIT_RESPONSE
} OP_HARDWARE;


typedef enum
{
	OP_LOG_OK = 0xC0,
	OP_lOG_FAIL
} OP_LOG;


/* Request Logs */
typedef enum
{
	REQ_EXP_NOREQUEST = 0xA0,
	REQ_EXP_TXRESULT
} REQ_EXPERIMENT;

typedef enum
{
	REQ_HW_NOREQUEST = 0xB0,
	REQ_HW_TRANSMIT_TEMPERATURE,
	REQ_HW_TRANSMIT_INDIVIDUAL_CURRENTS,
	REQ_HW_TRANSMIT_INDIVIDUAL_POWER,
	REQ_HW_TRANSMIT_TOTAL_POWER
} REQ_HARDWARE;

typedef enum
{
	REQ_LOG_NOREQUEST = 0xC0,
	REQ_LOG_TXRESULT
} REQ_LOG;



typedef struct
{
	REQ_EXPERIMENT 	EXP;
	REQ_HARDWARE 	HW;
	REQ_LOG		LOG;
} REQ;


typedef struct
{
	OP_EXP_STRUCT 	EXP;
	OP_HARDWARE		HW;
	OP_LOG	 		LOG;
} OP;

typedef struct
{
	OP	OP;
	REQ REQ;
} NASCERR_STATUS;

typedef struct __attribute__((packed)) {
    uint32_t firmware_version;          		/*< versão do firmware ativo				*/
    uint32_t other_firmware_version;    		/*< versão do firmware no outro bank 		*/
    uint8_t  flags;                    			/*< BootFlags 								*/
    uint8_t  last_error;                		/*< código de erro 							*/
    uint16_t last_packet_id;            		/*< último pacote 							*/
    uint8_t  current_region;            		/*< 0xAA=app1 | 0xBB=app2					*/
    uint8_t  reserved;                  		/*< alinhamento/futuro 						*/
} NASCERR_BOOT_STATUS;

/* BOOL definitions */
#define false       (0)
#define true        (1)
#define undefined   (2)
#ifndef NULL
#define NULL        (0)
#endif /* Define of NULL */

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

#endif /* INC_NASCERR_SYSTEM_H_ */
