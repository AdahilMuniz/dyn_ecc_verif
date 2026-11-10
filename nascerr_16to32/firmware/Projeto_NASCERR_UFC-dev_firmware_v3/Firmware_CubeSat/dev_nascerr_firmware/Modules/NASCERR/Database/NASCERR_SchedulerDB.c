/**
 * ******************************************************************************
 *  @file    : nascerr_schedulerDB.c
 *  @brief   : Biblioteca para gravar dados no SCHEDULER no Database do NASCERR
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

#include "NASCERR_SchedulerDB.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */


uint8_t nascerr_schdb_updateSchedulerStatus(uint8_t index, DBStatus new_status)
{
    if (index >= experimentHeader.scheduler_data.qtd_scheduling) {
        return false;
    }

    experimentHeader.scheduler_data.scheduled[index].status = new_status;

    return nascerr_db_updateHeader();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_schdb_executeSchedulerCommand
 *  @brief     : Registra uma execucao de comando agendado e atualiza seu status
 *  @param     : index - Indice do comando agendado no vetor scheduled[]
 *  @retval    : true se a execucao foi registrada com sucesso, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_schdb_executeSchedulerCommand(uint8_t index)
{
    if (index >= experimentHeader.scheduler_data.qtd_scheduling) {
        return false;
    }

    uint8_t page_buffer[FLASHEXT_PAGE_SIZE];
    NASCERR_SCHEDULER sched = {0};

    /* Endereco da pagina do comando */
    uint32_t addr = W25N01G_BLOCK_TO_LINEAR(FLASH_SCHEDULER_BLOCK) +
                    (index * FLASHEXT_PAGE_SIZE);

    /* Le a pagina da flash */
    FLASHTypeDef result = nascerr_flash_readAddress(addr, page_buffer, FLASHEXT_PAGE_SIZE);
    if (result.error) {
        return false;
    }

    /* Verifica o CRC */
    uint16_t crc_stored = ((uint16_t)page_buffer[CRC_AREA_OFFSET] << 8) |
                           (uint16_t)page_buffer[CRC_AREA_OFFSET + 1];
    uint16_t crc_calc = calculate_crc16(page_buffer, CRC_AREA_OFFSET);
    if (crc_stored != crc_calc) {
        return false;
    }

    /* Extrai o comando */
    memcpy(&sched, page_buffer, sizeof(NASCERR_SCHEDULER));

    /* Incrementa contador de execucoes no header */
    DBItem *item = &experimentHeader.scheduler_data.scheduled[index];
    item->num_block++;

    /* Atualiza status se necessario */
    /* Em scheduler `num_block` = numero de leituras */
    if (item->num_block > sched.repeat) {
        item->status = DB_CANCELED;
    }

    return nascerr_db_updateHeader();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_schdb_checkSchedulerConflict
 *  @brief     : Checks whether the requested block range overlaps any scheduled
 *               block range already stored in the database.
 *  @param     : block_id     - First block ID requested.
 *  @param     : block_number - Total number of consecutive blocks requested.
 *  @retval    : 1 if there is a conflict (overlap), 0 otherwise.
 * ******************************************************************************
 */
uint8_t nascerr_schdb_checkSchedulerConflict(id_block block_id, uint16_t block_number)
{
	// todo: corrigir essa possivel fonte de erro experimentHeader.scheduler_data.qtd_scheduling
    if (block_number == 0u) {
        return false;
    }

    uint32_t req_start = block_id;                     /* > requested range start */
    uint32_t req_end   = block_id + block_number - 1u; /* > requested range end   */
    if((block_id + block_number) > 64)
    {
    	return true;
    }

    uint32_t total = experimentHeader.scheduler_data.qtd_scheduling;
    NASCERR_SCHEDULER sched;

    for (uint32_t i = 0u; i < total; ++i) {
        DBItem *item = &experimentHeader.scheduler_data.scheduled[i];

        /* > ignore cancelled entries  */
        if (item->status == DB_CANCELED) {
            continue;
        }

        if (!nascerr_db_readScheduler(i, &sched)) {
            continue;
        }

        uint32_t sch_start = sched.block;                       /* > sched start */
        uint32_t sch_end   = sched.block + sched.qtd_block - 1u;/* > sched end   */

        /* overlap test: !(new_end < old_start || new_start > old_end) */
        if (!(req_end < sch_start || req_start > sch_end)) {
            return true;   /* > conflict found */
        }
    }
    return false; /* > no conflicts */
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_schdb_getNextNewCommand
 *  @brief     : Seleciona o comando DB_NEW_COMMAND mais antigo em relacao a
 *               scheduler_time e, em caso de empate, ao current_tick.
 *  @param     : index_out - indice escolhido na area de agendamento
 *               sched_out - copia completa da estrutura escolhida
 *  @retval    : true  - comando encontrado
 *               false - nenhum comando DB_NEW_COMMAND disponivel
 * ******************************************************************************
 */
uint8_t nascerr_schdb_getNextNewCommand(uint8_t *index_out,
                                     NASCERR_SCHEDULER *sched_out)
{
    if (index_out == NULL || sched_out == NULL) {
        return false;
    }

    uint32_t best_sched_time  = UINT32_MAX;
    uint32_t best_tick        = UINT32_MAX;
    uint8_t  best_index       = 0;
    uint8_t  found            = false;
    NASCERR_SCHEDULER temp;

    for (uint8_t i = 0; i < experimentHeader.scheduler_data.qtd_scheduling; i++) {
        DBItem *item = &experimentHeader.scheduler_data.scheduled[i];

        if (item->status != DB_NEW_COMMAND) {
            continue;
        }

        if (!nascerr_db_readScheduler(i, &temp)) {
    		DEBUG_PRINT(LOG_ERROR, "ERRO DE LEITURA WRITE_EXP\r\n");
            continue; /* leitura falhou, pula para o proximo */
        }

        /* Regra de prioridade:
           1) menor scheduler_time
           2) se empatar, menor current_tick (mais antigo) */
        if (!found                                     ||
            temp.scheduler_time <  best_sched_time     ||
           (temp.scheduler_time == best_sched_time &&
            temp.current_tick   <  best_tick)) {

            best_sched_time = temp.scheduler_time;
            best_tick       = temp.current_tick;
            best_index      = i;
            *sched_out      = temp;
            found           = true;
        }
    }

    if (found) {
        *index_out = best_index;
    }

    return found;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_schdb_getNextRepeatingCommand
 *  @brief     : Retorna o proximo comando com repeticao baseado no menor (current_tick + delay)
 *  @param     : index_out - Ponteiro para receber o indice do comando
 *               sched_out - Ponteiro para receber a estrutura do comando
 *  @retval    : true se um comando valido foi encontrado, false caso contrario
 * ******************************************************************************
 */
uint8_t nascerr_schdb_getNextRepeatingCommand(uint8_t *index_out,
        NASCERR_SCHEDULER *sched_out, uint32_t now_tick)
{
	// todo: Reavaliar essa lógica. Nao leva em consideracao o momento de criacao
    if (index_out == NULL || sched_out == NULL) {
        return false;
    }

    uint32_t freq           = osKernelGetTickFreq();
    uint64_t best_target_ms = UINT64_MAX;  // ← menor target_ms vence
    uint8_t  best_index     = 0;
    uint8_t  found          = false;
    NASCERR_SCHEDULER temp;

    for (uint8_t i = 0; i < experimentHeader.scheduler_data.qtd_scheduling; i++) {
        DBItem *item = &experimentHeader.scheduler_data.scheduled[i];

        if (item->status != DB_EXECUTED)         { continue; }
        if (!nascerr_db_readScheduler(i, &temp)) { continue; }
        if (item->num_block > temp.repeat)        { continue; }

        /* Alvo absoluto da proxima leitura em ms a partir de current_tick */
//        uint32_t target_ms = temp.scheduler_time
//                           + (uint32_t)(item->num_block + 1u) * temp.delay; //ATENÇÃO: numero de num_block é o contador do NUMERO DE LEITRUAS

        uint64_t ms_to_next = (uint64_t)temp.scheduler_time
                            + (uint64_t)(item->num_block + 1u) * temp.delay; // target_ms = tempo de agendador de escrita + (numero de leituras + 1)* tempos entre leituras (em ms)

        uint64_t abs_target_tick = (uint64_t)temp.current_tick
                                 + ms_to_next * freq / 1000u; // "normaliza" e converte para tick do sistema


        /* Seleciona o que tem o menor alvo — ou seja, deveria ter sido
           lido primeiro, independente de ja ter passado ou nao */
        if (!found || abs_target_tick < best_target_ms) {
            best_target_ms = abs_target_tick;
            best_index     = i;
            *sched_out     = temp;
            found          = true;
        }
    }

    if (found) {
        *index_out = best_index;
    }

    return found;
}
/*
 * ******************************************************************************
 *  Function Name: nascerr_schdb_refreshScheduler
 *  @brief     : Verifica se o escalonador atingiu MAX_SCHEDULER e se todos os
 *               agendamentos estao cancelados (DB_CANCELED).  Caso positivo,
 *               limpa a area do escalonador.
 *  @param     : None
 *  @retval    : true  - area limpa ou ja vazia
 *               false - ainda exite espaco no escalonador ou agendamentos ativos
 * ******************************************************************************
 */
uint8_t nascerr_schdb_refreshScheduler(void)
{
    uint8_t canceled_cnt = 0u;                           /* > Qtd. cancelados   */
    uint8_t active_cnt   = 0u;                           /* > Qtd. ainda ativos */

    /* Percorre todos os slots (ocupados ou nao) */
    for (uint8_t i = 0u; i < MAX_SCHEDULER; ++i)
    {
        DBItem *entry = &experimentHeader.scheduler_data.scheduled[i];

        switch (entry->status)
        {
            case DB_CANCELED:  canceled_cnt++;                break;
            case DB_EMPTY:     /* vaga livre, ignora */       break;
            default:           active_cnt++;                  break;
        }
    }

    /* Calcula quantos slots realmente “em uso” */
    uint8_t used_cnt = canceled_cnt + active_cnt;

    /* Sem ativos E CANCELADOS ≥ 50 % dos usados */
    if (active_cnt == 0 && used_cnt > 0 &&
        (canceled_cnt * 2) >= used_cnt)
    {
//    	DB_LOOK();
        nascerr_db_clearSchedulerArea();
        nascerr_db_readHeader();
//        DB_UNLOOK();
        return true;
    }

    return false;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_schdb_refreshExperiment
 *  @brief     : Varre somente os slots em uso (qtd_experiments) e decide se a
 *               area deve ser limpa.
 *               Regras:
 *                 - Se qtd_experiments == 0 => area ja vazia => retorna true.
 *                 - Ignora estados diferentes de DB_READ e DB_NOT_READ.
 *                 - Limpa se NAO houver nenhum DB_NOT_READ e a ocupacao for
 *                   maior que 1/3 de EXPERIMENTS_PER_PAGE.
 *  @param     : None
 *  @retval    : true  - area limpa ou ja vazia
 *               false - ainda ha experimento nao lido ou ocupacao < 1/3
 * ******************************************************************************
 */
uint8_t nascerr_schdb_refreshExperiment(void)
{
    uint8_t used_cnt = experimentHeader.experiment_data.qtd_experiments;
    if (used_cnt == 0u) {
        return true;                                     /* > Ja vazia                */
    }
    if (used_cnt > EXPERIMENTS_PER_PAGE) {
        used_cnt = EXPERIMENTS_PER_PAGE;                 /* > Saneamento defensivo    */
    }

    uint8_t not_read_cnt = 0u;                           /* > Qtd DB_NOT_READ         */

    /* Percorre somente os 'used_cnt' primeiros slots */
    for (uint8_t i = 0u; i < used_cnt; ++i)
    {
        const DBItem *entry = &experimentHeader.experiment_data.experiments[i];

        switch (entry->status)
        {
            case DB_NOT_READ:
                not_read_cnt++;
                break;

            case DB_READ:
                /* ok, considerado lido */
                break;

            default:
                /* Ignora: DB_EMPTY     */
                break;
        }
    }

    /* Ocupacao > 1/3 sem nenhum NOT_READ => pode limpar
       Evita float: used_cnt * 3 > EXPERIMENTS_PER_PAGE */
    if ((not_read_cnt == 0u) && ((uint16_t)used_cnt * 3u >= (uint16_t)EXPERIMENTS_PER_PAGE))
    {
//    	DB_LOOK();
        nascerr_db_clearExperimentArea();                /* > Limpa area              */
        nascerr_db_readHeader();                         /* > Recarrega header        */
//        DB_UNLOOK();
        return true;
    }

    return false;
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_schdb_verifySchedulerFull
 *  @brief     : Verifica se o campo scheduler_data.qtd_scheduling alcancou ou
 *               ultrapassou MAX_SCHEDULER, indicando que a lista de
 *               agendamentos esta cheia.
 *  @param     : None
 *  @retval    : true  - Scheduler cheio
 *               false - Ainda ha espaco disponivel
 * ******************************************************************************
 */
uint8_t nascerr_schdb_verifySchedulerFull(void)
{
	return (experimentHeader.scheduler_data.qtd_scheduling >= MAX_SCHEDULER) ? true : false;
}

/*
 * ******************************************************************************
 *  Function Name : nascerr_schdb_getElapsedMs
 *  @brief        : Calcula quanto tempo (ms) ja se passou desde scheduler_time
 *                  para o agendamento indicado.
 *  @param index  : Indice na lista de agendamentos (0-MAX_SCHEDULER-1)
 *  @param now_ms : Tick atual do sistema em milissegundos
 *  @retval       : Tempo decorrido em ms
 *                  UINT32_MAX se leitura falhar ou indice invalido
 * ******************************************************************************
 */
time_t nascerr_schdb_getElapsedMs(uint8_t index, time_t now_ms)
{
    /* Indice fora da faixa? */
    if (index >= experimentHeader.scheduler_data.qtd_scheduling){
        return UINT32_MAX;
    }
    NASCERR_SCHEDULER sched;
    if (!nascerr_db_readScheduler(index, &sched)) {
        return UINT32_MAX;
    }

    /* Se ainda nao chegou a hora, elapsed = 0 */
    if (now_ms < sched.current_tick) {
        return 0;
    }

    //DEBUG_PRINT(LOG_INFO, "Now_ms: %lu, sched_time: %lu\r\n", now_ms, sched.current_tick);
    return (uint32_t)(now_ms - sched.current_tick);
}

/*
 * *******************************************************************************
 *  Function Name : nascerr_schdb_updateExperimentStatus
 *  @brief        : Refreshes EXP_STAT register from the current database header.
 *  @param        : None
 *  @retval       : None
 * *******************************************************************************
 */
void nascerr_schdb_updateExperimentStatus(void)
{
    NASCERR_ExperimentStatus_t stat = {0};

    /* Scheduler counters  */
    stat.qtd_scheduler_total = experimentHeader.scheduler_data.qtd_scheduling;

    for (uint8_t i = 0; i < stat.qtd_scheduler_total; ++i)
    {
        DBItem *it = &experimentHeader.scheduler_data.scheduled[i];

        switch (it->status)
        {
            case DB_NEW_COMMAND:  stat.qtd_scheduler_pending++;  break;
            case DB_EXECUTED:     stat.qtd_scheduler_executed++; break;
            default: /* ignore */                                break;
        }
    }
    /* Experiment counters */
    stat.qtd_experiments_total = experimentHeader.experiment_data.qtd_experiments;

    for (uint8_t i = 0; i < stat.qtd_experiments_total; ++i)
    {
        DBItem *it = &experimentHeader.experiment_data.experiments[i];

        switch (it->status)
        {
            case DB_NOT_READ: stat.qtd_experiments_unread++;   break;
            case DB_READ:     stat.qtd_experiments_finished++; break;
            default:          /* running / other */            break;
        }
    }

    /* Running = total – (finished + unread) */
    stat.qtd_experiments_running =
        stat.qtd_experiments_total -
        (stat.qtd_experiments_finished + stat.qtd_experiments_unread);

    /* Flags */
    stat.scheduler_full_flag = (stat.qtd_scheduler_total >= MAX_SCHEDULER);
    stat.flash_full_flag     = (stat.qtd_experiments_total >= EXPERIMENTS_PER_PAGE);

    /* Copy packed structure into I2C register buffer */
    memcpy(g_register_data[EXP_STAT].value, &stat, sizeof(NASCERR_ExperimentStatus_t));
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_schdb_updateRegistersExpSumary
 *  @brief     : Updates the EXP_SUM_DATA register with a page (up to
 *               EXP_UNREAD_PAGE_SIZE items) of unread-experiment summaries,
 *               starting at the requested offset.
 *  @param     : offset – Index of the first unread experiment to include
 *               in this page (0, 30, 60, …).
 *  @retval    : false – No unread experiments exist
 *               true  – Summary page successfully generated
 * ******************************************************************************
 */
uint8_t nascerr_schdb_updateRegistersExpSumary(uint8_t offset)
{
    uint8_t *buf = g_register_data[EXP_SUM_DATA].value;
    ExpUnreadSummary_t summary = { 0 };

    /* Count TOTAL not-empty experiments */
    for (uint8_t i = 0; i < experimentHeader.experiment_data.qtd_experiments; ++i) {
        if (experimentHeader.experiment_data.experiments[i].status != DB_EMPTY) {
            ++summary.qtd_unread_total;
        }
    }

    if (summary.qtd_unread_total == 0) {
        /* Nothing to report: clear buffer and return 0 */
        memset(buf, 0, REG_EXP_SUM_DATA_SIZE);
        return false;
    }

    /* Fill page */
    summary.offset = offset;
    uint8_t skipped = 0;
    uint8_t included = 0;

    for (uint8_t i = 0;
         i < experimentHeader.experiment_data.qtd_experiments &&
         included < EXP_UNREAD_PAGE_SIZE;
         ++i)
    {
        DBItem *src = &experimentHeader.experiment_data.experiments[i];

        if (src->status == DB_EMPTY) 	{ continue; }
        if (skipped++ < offset) 		{ continue; }

        /* Copy full DBItem into summary array */
        summary.entries[included++] = *src;
    }

    summary.qtd_included = included;

    /* Copy to register buffer */
    memcpy(buf, &summary, sizeof(summary));

    /* Zero-pad remaining bytes, if any */
    if (sizeof(summary) < REG_EXP_SUM_DATA_SIZE) {
        memset(buf + sizeof(summary), 0, REG_EXP_SUM_DATA_SIZE - sizeof(summary));
    }

    return true;
}

/*
 * *******************************************************************************
 *  Function Name : nascerr_schdb_getExperimentStatus
 *  @brief        : Returns the content of EXP_STAT register from the current
 *  database header.
 *  @param        : None
 *  @retval       : NASCERR_ExperimentStatus_t: stat containg the info of the
 *  experiment_status
 * *******************************************************************************
 */
NASCERR_ExperimentStatus_t nascerr_schdb_getExperimentStatus(void)
{
    NASCERR_ExperimentStatus_t stat = {0};

    /* Scheduler counters  */
    stat.qtd_scheduler_total = experimentHeader.scheduler_data.qtd_scheduling;

    for (uint8_t i = 0; i < stat.qtd_scheduler_total; ++i)
    {
        DBItem *it = &experimentHeader.scheduler_data.scheduled[i];

        switch (it->status)
        {
            case DB_NEW_COMMAND:  stat.qtd_scheduler_pending++;  break;
            case DB_EXECUTED:     stat.qtd_scheduler_executed++; break;
            default: /* ignore */                                break;
        }
    }
    /* Experiment counters */
    stat.qtd_experiments_total = experimentHeader.experiment_data.qtd_experiments;

    for (uint8_t i = 0; i < stat.qtd_experiments_total; ++i)
    {
        DBItem *it = &experimentHeader.experiment_data.experiments[i];

        switch (it->status)
        {
            case DB_NOT_READ: stat.qtd_experiments_unread++;   break;
            case DB_READ:     stat.qtd_experiments_finished++; break;
            default:          /* running / other */            break;
        }
    }

    /* Running = total – (finished + unread) */
    stat.qtd_experiments_running =
        stat.qtd_experiments_total -
        (stat.qtd_experiments_finished + stat.qtd_experiments_unread);

    /* Flags */
    stat.scheduler_full_flag = (stat.qtd_scheduler_total >= MAX_SCHEDULER);
    stat.flash_full_flag     = (stat.qtd_experiments_total >= EXPERIMENTS_PER_PAGE);

    return stat;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_schdb_clearExpArea
 *  @brief     : Apaga (erase) e reinicializa a regiao indicada da memoria flash
 *               para permitir comandos remotos de limpeza.
 *  @param     : area - Regiao a ser apagada (FlashRegionTypeDef)
 *  @retval    : 1 (true)  – operacao concluida com sucesso
 *               0 (false) – falha ou regiao nao suportada
 * ******************************************************************************
 */
uint8_t nascerr_schdb_clearExpArea(FlashRegionTypeDef area)
{
    uint8_t ok = false;

    /* Por segurança, nunca apagar o HEADER remotamente */
    if (area == FLASH_HEADER || area >= MAX_FLASH_REGION)
        return false;

    switch (area)
    {
        case FLASH_LOGS:
            ok = nascerr_db_clearLogArea();           /* > Limpa logs */
            /* Inicializa os dados de logs */
			experimentHeader.logs_data.header.in_use = FLASH_IN_USE_BYTES;
			experimentHeader.logs_data.header.start_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_LOGS_BLOCK);
			experimentHeader.logs_data.header.current_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_LOGS_BLOCK);
			experimentHeader.logs_data.header.data_load = 0;
			experimentHeader.logs_data.qtd_logs = 0;

			for (int i = 0; i < LOGS_PER_PAGE; i++) {
				experimentHeader.logs_data.logs[i].id = MAX_ID_DB;
				experimentHeader.logs_data.logs[i].num_block = 0;
				experimentHeader.logs_data.logs[i].status = DB_EMPTY;
			}
            break;

        case FLASH_SCHEDULER:
            ok = nascerr_db_clearSchedulerArea();     /* > Limpa agenda */
            /* Inicializa os dados de agendamento (scheduler) */
			experimentHeader.scheduler_data.header.in_use = FLASH_IN_USE_BYTES;
			experimentHeader.scheduler_data.header.start_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_SCHEDULER_BLOCK);
			experimentHeader.scheduler_data.header.current_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_SCHEDULER_BLOCK);
			experimentHeader.scheduler_data.header.data_load = 0;
			experimentHeader.scheduler_data.qtd_scheduling = 0;

			for (int i = 0; i < SCHEDULER_PER_PAGE; i++) {
				experimentHeader.scheduler_data.scheduled[i].id = MAX_ID_DB;
				experimentHeader.scheduler_data.scheduled[i].num_block = 0;
				experimentHeader.scheduler_data.scheduled[i].status = DB_EMPTY;
			}
            break;

        case FLASH_EXPERIMENT:
            ok = nascerr_db_clearExperimentArea();    /* > Limpa exp. */
            /* Inicializa os dados de experimentos */
			experimentHeader.experiment_data.header.in_use = FLASH_IN_USE_BYTES;
			experimentHeader.experiment_data.header.start_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_EXPERIMENT_BLOCK);
			experimentHeader.experiment_data.header.current_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_EXPERIMENT_BLOCK);
			experimentHeader.experiment_data.header.data_load = 0;
			experimentHeader.experiment_data.qtd_experiments = 0;

			for (int i = 0; i < EXPERIMENTS_PER_PAGE; i++) {
				experimentHeader.experiment_data.experiments[i].id = MAX_ID_DB;
				experimentHeader.experiment_data.experiments[i].num_block = 0;
				experimentHeader.experiment_data.experiments[i].status = DB_EMPTY;
			}
            break;

        case FLASH_BACKUP_FIRMWARE:
        	ok = nascerr_db_clearBootArea();          /* Limpa Boot/Firmware */
        	/* Inicializa os dados de boot/firmware backup */
			experimentHeader.boot_data.header.in_use = FLASH_IN_USE_BYTES;
			experimentHeader.boot_data.header.start_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_BACKUP_FIRMWARE_BLOCK);
			experimentHeader.boot_data.header.current_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_BACKUP_FIRMWARE_BLOCK);
			experimentHeader.boot_data.header.data_load = 0;
			experimentHeader.boot_data.version = 0;
			experimentHeader.boot_data.crc = 0;
            break;

        default:
            break;
    }

    /* Se apagou com sucesso, grava o novo header com contadores zerados  */
    if (ok) { nascerr_db_updateHeader(); }

    return nascerr_db_readHeader();
}

/*
 * ******************************************************************************
 * Function Name: nascerr_schdb_updateRegistersLogSummary
 * @brief     : Prepara o registrador de sumario de logs (LOG_SUM) para uma pagina especifica.
 * @param     : page - Indice da pagina de logs cujo sumario sera preparado.
 * @retval    : 1 (true)  – O sumario foi gerado e copiado para o registrador.
 * 0 (false) – A pagina solicitada e invalida (maior ou igual ao
 * total de paginas existentes, e existem paginas).
 * @details   :
 * Esta funcao preenche o buffer global do registrador LOG_SUM com metadados
 * referentes a regiao de logs e a pagina especifica solicitada.
 * ******************************************************************************
 */

uint8_t nascerr_schdb_updateRegistersLogSummary(uint8_t page)
{
    uint8_t *out_sum = g_register_data[LOG_SUM].value;
    const uint16_t total_pages = experimentHeader.logs_data.qtd_logs;
    const uint16_t PAYLOAD_SUM_SIZE = REG_LOG_SUM_SIZE - 2;
    uint16_t pos = 0;

    memset(out_sum, 0, REG_LOG_SUM_SIZE);

    if (page >= total_pages && total_pages > 0) {
        return false; // Página inválida
    }

    /* Preenche o buffer com os metadados */
    memcpy(&out_sum[pos], &experimentHeader.logs_data.header.data_load, 4);
    pos += 4;
    memcpy(&out_sum[pos], &total_pages, 2);
    pos += 2;
    out_sum[pos++] = page;
    memcpy(&out_sum[pos], &experimentHeader.logs_data.logs[page], sizeof(DBItem));

    if (page >= total_pages) {
         /* A PÁGINA NÃO EXISTE. Criamos um header padrão com zeros. */
         DBItem empty_header = {0}; // Inicializa todos os campos com 0
         empty_header.status = DB_EMPTY; // Define o status como EMPTY

         /* Copia o header padrão para o buffer de sumário */
         memcpy(&out_sum[pos], &empty_header, sizeof(DBItem));

     } else {
         /* A PÁGINA EXISTE. Copia o header real. */
         memcpy(&out_sum[pos], &experimentHeader.logs_data.logs[page], sizeof(DBItem));
     }

    /* Calcula e anexa o CRC */
    uint16_t crc_sum = calculate_crc16(out_sum, PAYLOAD_SUM_SIZE);
    out_sum[PAYLOAD_SUM_SIZE]     = (uint8_t)(crc_sum >> 8);
    out_sum[PAYLOAD_SUM_SIZE + 1] = (uint8_t)(crc_sum);

    return true;
}

///*
// * ******************************************************************************
// *  Function Name: nascerr_schdb_recording_memory_external
// *  @brief        : Transfere os logs armazenados na memoria interna para a memoria externa.
// *  @param         : None
// *  @retval       : true se a operacao foi concluida com sucesso, false caso contrario.
// * ******************************************************************************
// */
//
//uint8_t nascerr_schdb_recording_memory_external(void)
//{
//	uint8_t *init_adress = nascerr_flash_get_log_address();
//	uint16_t occupied_memory = nascerr_flash_get_log_occupied_size();
//	uint16_t total = (occupied_memory/sizeof(NASCERR_LOG));
//	const NASCERR_LOG *src = (const NASCERR_LOG *)init_adress;
//	NASCERR_LOG buffer_write;
//
//	DEBUG_PRINT(LOG_INFO, "Iniciando pagina de log \r\n");
//	if (!nascerr_db_beginLogPage()) {
//		DEBUG_PRINT(LOG_ERROR, "Erro ao iniciar pagina de logs \r\n");
//		return false;
//	}else{
//		DEBUG_PRINT(LOG_INFO, "SUCESSO ao iniciar pagina de logs \r\n");
//	}
//
//	for(uint16_t i = 0; i < total; i++){
//
//		buffer_write.timestamp = src[i].timestamp;
//		buffer_write.type = (src[i].type >= LOG_SUCCESS && src[i].type <= LOG_BOOT_OPERATION) ? src[i].type : 0;
//		buffer_write.status = (src[i].status >= LOG_STATUS_SUCCESS && src[i].status <= LOG_STATUS_ERROR) ? src[i].status : 0;
//		buffer_write.error_code = (src[i].error_code >= ERR_NONE && src[i].error_code <= WARN_HW_TEMPERATURE_THRESHOLD) ? src[i].error_code: 0;
//		DEBUG_PRINT(LOG_INFO,
//			"LOG[%u] timestamp=%lu - type=0x%02X - status=%u - erro=%u\r\n",
//				i,
//				src[i].timestamp,
//				src[i].type,
//				src[i].status,
//				src[i].error_code
//			);
//
//        if (!nascerr_db_addLog(&buffer_write)) {
//            DEBUG_PRINT(LOG_ERROR, "Erro ao adicionar log %u na regiao \r\n", i);
//            return false;
//        }else{
//        	DEBUG_PRINT(LOG_INFO, "SUCESSO ao adicionar log %u na regiao \r\n", i);
//        }
////        osDelay(50);
//	}
//
//    if (!nascerr_db_commitLogPage()) {
//        DEBUG_PRINT(LOG_ERROR, "Erro ao commitar pagina de logs \r\n");
//        return false;
//    }else{
//        DEBUG_PRINT(LOG_INFO, "SUCESSO ao commitar pagina de logs \r\n");
//    }
//
//    if (!nascerr_flash_reset_log_buffer()) {
//        DEBUG_PRINT(LOG_ERROR, "Erro ao resetar a memoria interna reservada para os logs \r\n");
//        return false;
//    }else{
//        DEBUG_PRINT(LOG_ERROR, "SUCESSO ao resetar a memoria interna reservada para os logs \r\n");
//    }
//
//    return true;
//}

/*
 * ******************************************************************************
 * Function Name: nascerr_schdb_prepareLogResponse
 * @brief     : Prepara os registradores I2C com uma pagina de logs para leitura pelo mestre.
 * @param     : page - Indice da pagina de logs a ser lida do banco de dados (0-N).
 * @retval    : Nenhum (void)
 * @details   : Esta funcao implementa o protocolo de resposta de tamanho variavel.
 * ******************************************************************************
 */
void nascerr_schdb_prepareLogResponse(uint8_t page)
{
    uint16_t logs_read_count = 0;
    uint8_t* page_buffer = g_register_data[LOG_READ_PAGE].value;

    if (!nascerr_db_readLogPage(page, (NASCERR_LOG*)page_buffer, &logs_read_count)) {
        DEBUG_PRINT(LOG_ERROR, "Falha ao ler pagina de log %u do DB\r\n", page);
        logs_read_count = 0; // Força uma resposta vazia em caso de erro
    }

    uint16_t bytes_to_send = logs_read_count * sizeof(NASCERR_LOG);

//    memcpy(buffer_aux, page_buffer, bytes_to_send); /*TODO: Remover*/

    uint16_t total_packet_size = bytes_to_send + 2; // payload + CRC
    g_register_data[IDX].value[0] = (uint8_t)(total_packet_size);
    g_register_data[IDX].value[1] = (uint8_t)(total_packet_size >> 8);

    uint16_t response_crc = calculate_crc16(page_buffer, bytes_to_send);
    page_buffer[bytes_to_send]     = (uint8_t)(response_crc >> 8);
    page_buffer[bytes_to_send + 1] = (uint8_t)(response_crc);



//    DEBUG_PRINT(LOG_DEBUG, "--- DEBUG FIRMWARE: Preparando para enviar ---\r\n");
//    DEBUG_PRINT(LOG_DEBUG, "Tamanho a ser enviado (do IDX): %u bytes\r\n", total_packet_size);
//    if (total_packet_size > 0)
//    {
//        /* Imprime o buffer em formato hexadecimal, 16 bytes por linha */
//        char hex_line[64]; // Buffer temporário para a linha de hex
//        int offset = 0;
//        for (int i = 0; i < total_packet_size; i++) {
//            // Converte um byte para 2 chars hex + espaço
//            offset += sprintf(hex_line + offset, "%02X ", page_buffer[i]);
//            /* Imprime a cada 16 bytes ou quando for o último byte */
//            if ((i + 1) % 16 == 0 || i == total_packet_size - 1) {
//                DEBUG_PRINT(LOG_DEBUG, "LOG_READ_PAGE[%04d]: %s \r\n", i - (i % 16), hex_line);
//                offset = 0; // Reseta o buffer da linha
//                memset(hex_line, 0, sizeof(hex_line));
//            }
//        }
//    }
//    DEBUG_PRINT(LOG_DEBUG, "------------------------------------------\r\n");
}

uint8_t nascerr_schdb_log_markReadAndRefresh(uint8_t page)
{
    /* Valida se a pagina solicitada existe. */
    if (page >= experimentHeader.logs_data.qtd_logs) {
        return false;
    }

    experimentHeader.logs_data.logs[page].status = DB_READ;

    if (!nascerr_db_updateHeader()) {
        return false;
    }

    nascerr_schdb_updateRegistersLogSummary(page);

    /* Verifica se todas as paginas foram lidas e, se sim, limpa a area. */
//    if (auto_clear && nascerr_schdb_logs_allRead()) {
//        (void)nascerr_schdb_clearExpArea(FLASH_LOGS);
//        // Prepara a pagina 0 como o novo padrao apos a limpeza.
//        nascerr_schdb_prepareLogResponse(0);
//        nascerr_schdb_updateRegistersLogSummary(0);
//    }

    return true;
}


/*
 * ******************************************************************************
 *  Local Functions Definitions
 * ******************************************************************************
 */
