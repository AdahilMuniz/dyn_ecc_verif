/**
 * ******************************************************************************
 *  @file    : nascerr_scheduler.h
 *  @brief   : Header for nascerr_scheduler.c file.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

#ifndef INC_NASCERR_SCHEDULER_H_
#define INC_NASCERR_SCHEDULER_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "main.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "queue.h"
#include "NASCERR_System.h"
#include "NASCERR_Log.h"
#include "NASCERR_Protocol.h"
#include "NASCERR_DB.h"
#include "NASCERR_SchedulerDB.h"
#include "NASCERR_Experiment.h"
#include "NASCERR_Bootloader.h"
#include "NASCERR_Log.h"
#include "NASCERR_UartImage.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

/* > Quantidade de Theads Temporais			*/
#define SCHEDULER_THREADS_COUNT  4u

/* > Mascara de flags para cada evento 		*/
#define FLAG_EXP_WRITE     			(1u << 0)
#define FLAG_EXP_READ      			(1u << 1)
#define FLAG_HW_CHECK      			(1u << 2)
#define FLAG_MEMORY_CHECK			(1u << 3)
#define FLAG_BOOTLOADER_TRIGGER   	(1u << 0)
#define FLAG_IMG_DMA_RX_DONE		(1U << 0)

/* > Prioridades CMSIS (mapa p/ freeRTOS) 	*/
#define PRIO_COMM      (osPriorityHigh)
#define PRIO_IO_HIGH   (osPriorityAboveNormal)
#define PRIO_MED       (osPriorityNormal)
#define PRIO_LOW       (osPriorityBelowNormal)
#define PRIO_BG        (osPriorityLow)

/* > Mutex DB defines 						*/
#define DB_LOCK()    osMutexAcquire(dbMutex, osWaitForever)
#define DB_UNLOCK()  osMutexRelease(dbMutex)

/* > Mutex SRAM defines 	*/
#define SRAM_LOCK()    osMutexAcquire(sramMutex, osWaitForever)
#define SRAM_UNLOCK()  osMutexRelease(sramMutex)

/* > Profundidade da fila  					*/
#define CMD_PAYLOAD_MAX     32u  /* > bytes maximos no campo payload 		*/
#define CMD_QUEUE_LENGTH   	MAX_SCHEDULER  /* > suporta ate MAX_SCHEDULER requisicoes para CMD 	*/

/* > Quantidade de execucoes necessaria para que o teste completo da sram ocorra */
#define FULL_SRAMs_CHECK_CYCLE_TRIGGER 720


/* > Parametros para o funcionamento do mock 					*/
#if MOCK_COMMAND

/* > Defines necessarios		*/
	#define NASCERR_CMD_PAYLOAD_LEN  27u   /* cmd_type(1) + scheduler(24) + crc(2) */
	#define NASCERR_CMD_FRAME_SIZE   29u   /* bytes_msg(2) + 27                    */

/* > Macros necessarios		*/
	#define PUT_U16_BE(p, val) \
    do { (p)[0] = (uint8_t)((val) >> 8);         \
         (p)[1] = (uint8_t)((val) & 0xFF); } while(0)

	#define PUT_U32_BE(p, val) \
		do { (p)[0] = (uint8_t)((val) >> 24);        \
			 (p)[1] = (uint8_t)((val) >> 16);        \
			 (p)[2] = (uint8_t)((val) >>  8);        \
			 (p)[3] = (uint8_t)((val) & 0xFF); } while(0)
#endif

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Enum Name : CommandType_t
 *  @brief    : Identificacao dos comandos enviados pela Master
 * ******************************************************************************
 */
typedef enum
{
    CMD_SET_WRITE_SCH         = 0,    /* > Solicita escrita de experimento na SRAM 			 */
    CMD_READ_MEMORY           = 1,    /* > Solicita leitura  de experimento na SRAM          */
    CMD_TO_BOOTLOADER         = 3,    /* > Inicia checagem / atualizacao de firmware         */
    CMD_SET_HW_CHECK_PERIOD   = 4,    /* > Ajusta periodo (ms) da verificacao de hardware    */
	CMD_PREPARE_LOG_PAGE 	  = 5,    /* > Prepara página de logs							 */
	CMD_MARK_LOG_PAGE 	  	  = 6,    /* > Marca a página de logs como lida					 */
    CMD_UNKNOWN               = 0xFF  /* > Comando invalido / nao suportado                  */
} CommandType_t;

/*
 * ******************************************************************************
 *  Struct Name: CommandMessage_t
 *  @brief     : Mensagem para o escalonador (tipo + carga generica)
 * ******************************************************************************
 */
typedef struct
{
    CommandType_t type;                     /* > qual comando               	*/
    uint32_t      param_u32;                /* > uso generico               	*/
    uint8_t       payload[CMD_PAYLOAD_MAX]; /* > dados crus                 	*/
    uint8_t       payload_len;              /* > bytes validos em payload   	*/
    uint16_t      payload_crc;              /* > CRC16 da carga             	*/
} CommandMessage_t;


/*
 * ******************************************************************************
 *  Struct Name: EventType_t
 *  @brief     : Define o comportamento do evento
 * ******************************************************************************
 */

typedef enum {
    EVT_ONESHOT,
    EVT_PERIODIC
} EventType_t;

/*
 * ******************************************************************************
 *  Struct Name: EventScheduler_t
 *  @brief     : Entrada usada pelo scheduler para eventos periodicos
 * ******************************************************************************
 */
typedef struct {
    uint32_t     periodTicks;             	/* > Periodo entre execucoes (ticks) */
    uint32_t     nextWakeTick;            	/* > Tick proximo disparo            */
    osThreadId_t targetId;                	/* > Thread a ser sinalizada         */
    uint32_t     flagMask;                	/* > Mascara da flag enviada         */
    EventType_t type;						/* > O tipo do evento				 */

} EventScheduler_t;

/*
 * ******************************************************************************
 *  Enum Name: LoggerOp_t
 *  @brief     : Entrada usada pelo scheduler para controle de gravação de logs
 * ******************************************************************************
 */
typedef enum {
	LOG_OP_FLUSH_RAM_TO_FLASH = 0,      	/* > Gravar o buffer de RAM na área de logs em flash 						*/
    LOG_OP_BUILD_PAGE_SUMMARY,          	/* > montar LOG_READ_PAGE + LOG_SUM da página X 	 						*/
    LOG_OP_MARK_READ_AND_REFRESH,       	/* > marcar página X como DB_READ + atualizar sumário; opcional: auto-clear */
    LOG_OP_CLEAR_LOGS                		/* > apagar a região de logs										        */
} LoggerOp_t;

/*
 * ******************************************************************************
 *  Struct Name: LoggerMst_t
 *  @brief     : Mensagem usada pelo scheduler para controle de gravação de logs
 * ******************************************************************************
 */
typedef struct {
	LoggerOp_t op;
	uint8_t page;							/* > Usado por BUILD_PAGE_SUMMARY / MARK_READ_AND_REFRESH */
	uint8_t auto_clear;						/* > Usado por MARK_READ_AND_REFRESH (0/1) */
}LoggerMsg_t;

/* Fila global do logger */
extern osMessageQueueId_t g_loggerQ;
/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

void nascerr_sch_initScheduler(void);

void nascerr_sch_masterCommTask(void);
void nascerr_sch_bootloaderTask(void *arg);
void nascerr_sch_imgUploadTask(void *arg);
void nascerr_sch_schedulerTask(void *arg);
void nascerr_sch_expWriteTask(void *arg);
void nascerr_sch_expReadTask(void *arg);
void nascerr_sch_hwCheckTask(void *arg);
void nascerr_sch_loggerTask(void *arg);
void nascerr_sch_memCheckTask(void *arg);

uint8_t nascerr_sch_processCommand(CommandMessage_t *cmd);
void nascerr_sch_handleCommand(const CommandMessage_t *cmd);

void nascerr_sch_signal_loggerTask(void);
uint8_t nascerr_sch_recording_memory_external(void);
#endif /* INC_NASCERR_SCHEDULER_H_ */
