/**
 * ******************************************************************************
 *  @file    : nascerr_db.h
 *  @brief   : Header for nascerr_db.c file.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

#ifndef INC_NASCERR_DB_H_
#define INC_NASCERR_DB_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "main.h"
#include "NASCERR_System.h"
#include "NASCERR_Util.h"
#include "NASCERR_Flash.h"
#include "NASCERR_Control.h"
#include "NASCERR_Log.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

#define EXPERIMENTS_PER_PAGE        (W25N01G_PAGES_PER_BLOCK*FLASH_EXPERIMENT_SIZE)
#define LOGS_PER_PAGE       		(W25N01G_PAGES_PER_BLOCK*FLASH_LOGS_SIZE)
#define SCHEDULER_PER_PAGE        	(W25N01G_PAGES_PER_BLOCK*FLASH_SCHEDULER_SIZE)
#define IMAGE_AREA_SIZE				(FLASH_IMAGE_SIZE*W25N01G_PAGES_PER_BLOCK*W25N01G_PAGE_SIZE)

#define FLASHEXT_PAGE_SIZE        	(W25N01G_PAGE_SIZE)

#define CRC_AREA_SIZE             	(2)
#define CRC_AREA_OFFSET           	(FLASHEXT_PAGE_SIZE - CRC_AREA_SIZE)

#define MAX_EXPERIMENTS_PER_PAGE  	(23)    /* Numero maximo de experimentos por pagina de 2112 bytes (88B por experimento + 2B CRC) */
#define MAX_LOGS_PER_PAGE         	(301)   /* Numero maximo de logs por pagina de 2112 bytes (7B por log + 2B CRC) */
#define MAX_SCHEDULER             	(W25N01G_PAGES_PER_BLOCK) /* Um bloco para separado para scheduler = 64 */
#define MAX_ID_DB             		(0xFFFFFFFF) /* Identificador maximo de banco de dados (32 bits) */

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

#pragma pack(push, 1)

/* > Estados possiveis para os dados armazenados */
typedef enum
{
	DB_EMPTY        = 0x00,
	DB_NOT_READ     = 0x01,
    DB_READ         = 0x02,
    DB_CANCELED     = 0x03,
	DB_NEW_COMMAND  = 0x04,
	DB_EXECUTED     = 0x05,
} DBStatus;

/* > Estrutura de um item de experimento ou agendamento */
typedef struct
{
	id_db    		id;       		/* > Identificador do banco  */
	nascerr_nblock 	num_block;     	/* > Numero de blocos        */
    DBStatus 		status;   		/* > Estado do banco         */
} DBItem;

/* > Bootload dados */
typedef struct
{
    CHeaderTypeDef header;   	    /* > Cabecalho comum     	*/
    uint32_t       version;  		/* > Versao do firmware  	*/
    uint16_t       crc;      		/* > Checksum CRC        	*/
    uint8_t        hash_256[32]; 	/* > Hash SHA-256 do boot  */
} BootData;

typedef struct {
    CHeaderTypeDef header;      	/* start_addr, current_addr, data_load   */
    uint32_t       image_size;  	/* tamanho real da imagem em bytes       */
    uint32_t       crc16;       	/* CRC16 da imagem completa              */
    uint8_t        valid;       	/* 0xAA = válida, 0x00 = vazia           */
    uint8_t        reserved[3];
} ImageData;

/* > Scheduler dados */
typedef struct
{
    CHeaderTypeDef header;                            /* > Cabecalho comum             */
    uint8_t        qtd_scheduling;                    /* > Quantidade de agendamentos  */
    DBItem         scheduled[SCHEDULER_PER_PAGE];     /* > Vetor de agendamentos       */
} SchedulerData;

/* > Experimentos dados */
typedef struct
{
    CHeaderTypeDef header;                            /* > Cabecalho comum             */
    uint8_t        qtd_experiments;                   /* > Quantidade de experimentos  */
    DBItem         experiments[EXPERIMENTS_PER_PAGE]; /* > Vetor de experimentos       */
} ExpData;

/* > Logs de operacao dados */
typedef struct
{
    CHeaderTypeDef header;                            /* > Cabecalho comum             */
    uint16_t       qtd_logs;                          /* > Quantidade de pages de logs */
    DBItem         logs[LOGS_PER_PAGE];   		      /* > Logs com qtd and status     */
} LogData;

/* > Header Flash NASCERR */
typedef struct
{
	LogData        logs_data;    					  /* > Cabecalho principal     	   */
    SchedulerData  scheduler_data; 					  /* > Dados de agendamento        */
    ExpData        experiment_data;					  /* > Dados dos experimentos      */
    ImageData	   image_data;						  /* > Dados da imagem guardada    */
    BootData       boot_data;     			     	  /* > Dados do boot               */
    uint16_t       crc;            					  /* > Checksum CRC                */
} HeaderTypeDef;

#pragma pack(pop)

/* > Exportando header experiment */
extern HeaderTypeDef experimentHeader;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

/* Funcoes para atualizar Cabecalho */
SYSTEM_STATUS nascerr_db_init(void);
uint8_t nascerr_db_readHeader(void);
uint8_t nascerr_db_defaultHeader(void);
uint8_t nascerr_db_updateHeader(void);

/* Funcoes para atualizar BootArea */
void nascerr_db_updateBootSha(uint8_t* sha);
void nascerr_db_updateBootVersion(uint32_t new_version);
FLASHTypeDef nascerr_db_writeBootData(const uint8_t *data, uint32_t size);
uint8_t nascerr_db_readBootData(uint32_t offset, uint8_t *buffer, uint32_t size);
uint8_t nascerr_db_clearBootArea(void);
uint8_t nascerr_db_commitHeader(uint32_t last_addr);

/* Funcoes para atualizar ExperimentArea */
uint8_t nascerr_db_beginExperimentPage(id_db id);
uint8_t nascerr_db_addExperiment(const NASCERR_EXPERIMENT *exp);
uint8_t nascerr_db_commitExperimentPage(void);

const DBItem* nascerr_db_listAllExperiments(uint8_t *count);
uint8_t nascerr_db_listUnreadExperimentIDs(id_db *buffer, uint8_t max, uint8_t offset);
uint8_t nascerr_db_readExperimentPage(uint8_t index, NASCERR_EXPERIMENT *buffer);
uint8_t nascerr_db_setReadExperimentPage(uint8_t index);
uint8_t nascerr_db_readExperimentRange(uint8_t index, uint8_t offset, uint8_t qtd, NASCERR_EXPERIMENT *buffer);
uint8_t nascerr_db_clearExperimentArea(void);

/* Funcoes para atualizar Logs do Sistema */
uint8_t nascerr_db_beginLogPage(void);
uint8_t nascerr_db_addLog(const NASCERR_LOG *log);
uint8_t nascerr_db_commitLogPage(void);
uint8_t nascerr_db_readLogPage(uint8_t index, NASCERR_LOG *buffer, uint16_t *count);
uint8_t nascerr_db_clearLogArea(void);

/* Funcoes para scheduler do Sistema */
uint8_t nascerr_db_writeScheduler(const NASCERR_SCHEDULER *sched);
uint8_t nascerr_db_updateScheduler(const NASCERR_SCHEDULER *sched);
uint8_t nascerr_db_readScheduler(uint8_t index, NASCERR_SCHEDULER *dest);
uint8_t nascerr_db_clearSchedulerArea(void);

/* Funcoes para armazenamento de imagem */
uint8_t nascerr_db_writeImage(const uint8_t *data, uint32_t size);
uint8_t nascerr_db_writeImageChunk(uint32_t offset, const uint8_t *data, uint32_t size);
uint8_t nascerr_db_readImageChunk(uint32_t offset, uint8_t *buffer, uint32_t size);
uint8_t nascerr_db_validateImage(uint32_t size, uint16_t crc16_expected);
uint8_t nascerr_db_clearImageArea(void);
uint8_t nascerr_db_setImageSize(uint32_t size);
#endif /* INC_NASCERR_DB_H_ */
