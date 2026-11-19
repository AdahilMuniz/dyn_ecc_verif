/*
 * NASCERR_Log.c
 *
 *  Created on: May 7, 2025
 *      Author: Wendell
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "NASCERR_Scheduler.h"
#include "NASCERR_Log.h"
#include "NASCERR_DB.h"
#include <stdio.h> // Para printf (caso use debug via serial)


/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

static NASCERR_LOG log_buffer;
static uint32_t log_count = 0;


/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: nascerr_log_add
 *  @brief        : Registra uma nova entrada de log e armazena na memoria flash.
 *  @param type   : Tipo da operacao registrada.
 *  @param status : Status do evento (OK, WARN ou ERROR).
 *  @param code   : Codigo especifico da operacao ou erro.
 *  @retval       : None
 * ******************************************************************************
 */

void nascerr_log_add(LOG_TYPE type, uint8_t status, ERROS_LOGS code) {
	NASCERR_LOG *log = &log_buffer;

	log->timestamp = (uint32_t)xTaskGetTickCount();
	log->type = type;
	log->status = status;
	log->error_code = code;

#if ENABLE_SERIAL_SYS
	DEBUG_PRINT(LOG_LOG,"Log[%d] - Time: %lu, Type: %d, Status: %d, Erro: %d\r\n",
		   log_count,
		   log->timestamp,
		   log->type,
		   log->status,
		   log->error_code);
#endif

	nascerr_sch_signal_loggerTask();
	log_count++;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_log_get_count
 *  @brief        : Retorna a quantidade atual de entradas válidas no buffer de log.
 *  @param        : None
 *  @retval       : Número de logs armazenados no momento.
 * ******************************************************************************
 */
uint16_t nascerr_log_get_count(void) {
    return log_count;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_log_get_count
 *  @brief        : Retorna o tamanho do buffer de log.
 *  @param        : None
 *  @retval       : Número de logs armazenados no momento.
 * ******************************************************************************
 */
uint16_t nascerr_log_get_buffer_size(void) {
    return LOG_BUFFER_MAX_SIZE;
}

/*
 * ******************************************************************************
 *  Function Name : nascerr_log_get_buffer
 *  @brief        : Retorna o ponteiro para o buffer de logs.
 *  @param        : None.
 *  @retval       : Ponteiro para o buffer de log ou NULL se não inicializado.
 * ******************************************************************************
 */

NASCERR_LOG* nascerr_log_get(void) {
	return &log_buffer;
}
/*
 * ******************************************************************************
 *  Function Name: nascerr_log_clear
 *  @brief        : Limpa todas as entradas do log, resetando índices e contadores.
 *  @param        : None
 *  @retval       : None
 * ******************************************************************************
 */

void nascerr_log_clear(void) {
		log_buffer.timestamp = 0;
		log_buffer.type = 0;
		log_buffer.status = 0;
		log_buffer.error_code = 0;
}
