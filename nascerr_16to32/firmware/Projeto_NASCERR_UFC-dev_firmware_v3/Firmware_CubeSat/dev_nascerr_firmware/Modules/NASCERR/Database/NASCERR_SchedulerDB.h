/**
 * ******************************************************************************
 *  @file    : nascerr_schedulerDB.h
 *  @brief   : Header for nascerr_schedulerdb.c file.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

#ifndef INC_NASCERR_SCHEDULERDB_H_
#define INC_NASCERR_SCHEDULERDB_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "main.h"
#include "NASCERR_System.h"
#include "NASCERR_Flash.h"
#include "NASCERR_DB.h"
#include "NASCERR_Registers.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Struct Name: ExpUnreadSummary_t
 *  @brief     : Resumo dos experimentos com status DB_NOT_READ
 * ******************************************************************************
 */
typedef struct
{
    uint16_t qtd_unread_total;              /* > Quantidade TOTAL de nao-lidos no sistema      */
    uint8_t  offset;                        /* > Offset do primeiro item desta pagina          */
    uint8_t  qtd_included;                  /* > Quantos itens foram incluidos neste resumo    */

    DBItem   entries[EXP_UNREAD_PAGE_SIZE]; /* > Vetor de resumos (id, num_block, status)      */

} ExpUnreadSummary_t;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

uint8_t nascerr_schdb_updateSchedulerStatus(uint8_t index, DBStatus new_status);
uint8_t nascerr_schdb_executeSchedulerCommand(uint8_t index);
uint8_t nascerr_schdb_checkSchedulerConflict(id_block block_id, uint16_t block_number);
uint8_t nascerr_schdb_getNextNewCommand(uint8_t *index_out, NASCERR_SCHEDULER *sched_out);
uint8_t nascerr_schdb_getNextRepeatingCommand(uint8_t *index_out, NASCERR_SCHEDULER *sched_out, uint32_t now_tick);
uint8_t nascerr_schdb_refreshScheduler(void);
uint8_t nascerr_schdb_refreshExperiment(void);
uint8_t nascerr_schdb_verifySchedulerFull(void);
time_t nascerr_schdb_getElapsedMs(uint8_t index, time_t now_ms);

/* Update Registers */
void nascerr_schdb_updateExperimentStatus(void);
uint8_t nascerr_schdb_updateRegistersExpSumary(uint8_t offset);

/* Get register information (Experiment)*/
NASCERR_ExperimentStatus_t nascerr_schdb_getExperimentStatus(void);

uint8_t nascerr_schdb_clearExpArea(FlashRegionTypeDef area);

/* Logs */
uint8_t nascerr_schdb_updateRegistersLogSummary(uint8_t page);
void nascerr_schdb_prepareLogResponse(uint8_t page);
uint8_t nascerr_schdb_log_markReadAndRefresh(uint8_t page);

#endif /* INC_NASCERR_SCHEDULERDB_H_ */
