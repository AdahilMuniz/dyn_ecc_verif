/**
 * ******************************************************************************
 *  @file    : nascerr_scheduler.c
 *  @brief   : Biblioteca para o SCHEDULER do NASCERR
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

#include "NASCERR_Scheduler.h"
#include "NASCERR_ImageProcessing.h"
#include "uart_handler.h"
#include "NASCERR_HW_Status.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

/* > IDs de thread retornados por osThreadNew */
osThreadId_t schedulerId;    /* > Thread central de escalonamento             */
osThreadId_t expWriteId;     /* > Escrita de experimento na SRAM              */
osThreadId_t expReadId;      /* > Leitura  de experimento                     */
osThreadId_t hwCheckId;      /* > Verificacao de hardware                     */
osThreadId_t bootloaderId;   /* > Bootloader FW                               */
osThreadId_t loggerId;		 /* > Gravacao de parametro de logs               */
osThreadId_t memCheckId;     /* > Verificacao da SRAM e encoders/decoders     */
osThreadId_t imgUploadId; 	 /* > Recepção e gravação de imagem na flash      */

/* > Fila de comando */
osMessageQueueId_t commandQ; /* > Fila de comandos da Master                  */

/* > Mutex do banco de dados */
static osMutexId_t dbMutex;

/* > Mutex da SRAM */
static osMutexId_t sramMutex;

/* > Periodos iniciais (ms) – mutaveis em runtime */
static uint32_t expWritePeriodMs = 0;
static uint32_t expReadPeriodMs  = 0;
static uint32_t hwCheckPeriodMs  = 2000u;
static uint32_t memCheckPeriodMs  = 10000u;

/* > Auxiliar que salva o contexto dos periodos – Uso antes do dormir e pos-wakeup*/
static uint32_t tick_context[4] = {0, 0, 2000, 10000}; //expWritePeriodMs, expReadPeriodMs, hwCheckPeriodM, memCheckPeriodMs

/* > Tabela de eventos periodicos */
static EventScheduler_t schedTbl[SCHEDULER_THREADS_COUNT];

/* > Atributos padrao de threads */
osThreadAttr_t commAttr = { .priority = PRIO_COMM,    .stack_size = 1024 * 4 };
osThreadAttr_t ioAttr   = { .priority = PRIO_IO_HIGH, .stack_size = 1024 * 4 };
osThreadAttr_t medAttr  = { .priority = PRIO_MED,     .stack_size = 1024 * 4 };
osThreadAttr_t lowAttr  = { .priority = PRIO_LOW,     .stack_size = 1024 * 4 };
osThreadAttr_t bgAttr   = { .priority = PRIO_BG,      .stack_size = 1024 * 4 };

/* > Proximo experimento para executar */
static NASCERR_SCHEDULER g_runtimeSched;
static uint8_t g_current_idx;

/* > Proximo experimento para ler */
static NASCERR_SCHEDULER g_readExp;
static uint8_t g_read_idx;

/* > Variavel de controle do enfileiramento*/ //Usada como auxiliar do wake_up/sleep que nao sao enfileirados.
static volatile uint8_t allowCmdEnqueue = true;

/* > Posicao atual global do sistema e informacoes de data */
static MDCA_Data_t gps_info;

/* > Flag de controle do bootloader */
extern uint8_t boot_in_process;

/* > Flag de restart das informacoes da SRAM */
extern uint8_t restore_robust_mem_info;

/* Fila global do logger */
osMessageQueueId_t g_loggerQ = NULL;

/* Experimento transmitido via I2C */
static uint16_t g_readPage = 0;


#if MOCK_COMMAND

uint8_t send_mocked = false;

NASCERR_SCHEDULER sched = {
    .id             = 1,
    .block          = 1,
    .qtd_block      = 4,
    .ecc            = BYPASS_1,
    .write_type     = WRITE_xAAh,
    .scheduler_time = 5000,
    .repeat         = 3,
    .delay          = 5000,
    .current_tick   = 0
};

void nascerr_mock_write_cmd(const NASCERR_SCHEDULER *sched)
{
    if (!sched) return;

    uint8_t *p = g_register_data[CMD].value;

    PUT_U16_BE(p, NASCERR_CMD_PAYLOAD_LEN);  p += 2;
    *p++ = (uint8_t)WRITE_CMD;

    PUT_U32_BE(p, sched->id);             p += 4;
    PUT_U16_BE(p, sched->block);          p += 2;
    PUT_U16_BE(p, sched->qtd_block);      p += 2;
    *p++ = (uint8_t)sched->ecc;
    *p++ = (uint8_t)sched->write_type;
    PUT_U32_BE(p, sched->scheduler_time); p += 4;
    PUT_U16_BE(p, sched->repeat);         p += 2;
    PUT_U32_BE(p, sched->delay);          p += 4;
    PUT_U32_BE(p, sched->current_tick);   p += 4;

    uint16_t crc = calculate_crc16(g_register_data[CMD].value, NASCERR_CMD_FRAME_SIZE - 2);
    PUT_U16_BE(p, crc);
}
#endif


/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

/* > Funcoes de suporte */
static uint32_t ms_to_ticks(uint32_t ms);
static void sch_reset_event(EventScheduler_t *e, uint32_t newMs);
static void sch_setExpWritePeriodMs(uint32_t ms);
static void sch_setExpReadPeriodMs(uint32_t ms);
static void sch_setHwCheckPeriodMs(uint32_t ms);
static void sch_setMemCheckPeriodMs(uint32_t ms);
static uint8_t sch_check_payload_crc(const CommandMessage_t *cmd);
static uint32_t sch_next_read_ms(const NASCERR_SCHEDULER *s, uint32_t num_done);
static void sch_to_query(const NASCERR_SCHEDULER *sch, NASCERR_QUERY *q);
static void img_rx_notify_cb(void);

static void handle_write_scheduler(const NASCERR_SCHEDULER *sch);
static void handle_read_memory(const NASCERR_QUERY *qry);
static void update_system_status();

/*
 * ******************************************************************************
 *  Function Name: nascerr_sch_initScheduler
 *  @brief     : Inicializa filas, threads, tabela de eventos do scheduler e
 *  instancia os mutexes.
 *  @param     : None
 *  @retval    : None
 * ******************************************************************************
 */
void nascerr_sch_initScheduler(void)
{
    /* Cria filas */
    commandQ = osMessageQueueNew(CMD_QUEUE_LENGTH, sizeof(CommandMessage_t), NULL);
    /* Fila de logger*/
    g_loggerQ = osMessageQueueNew(8, sizeof(LoggerMsg_t), NULL);
    /* Cria threads */
    {
    	ioAttr.name   = "nascerrSch";
    	schedulerId   = osThreadNew(nascerr_sch_schedulerTask,  NULL, &ioAttr);
    }
    {
    	medAttr.name  = "nascerrWriteExp";
    	expWriteId    = osThreadNew(nascerr_sch_expWriteTask,   NULL, &medAttr);
    }
    {
    	ioAttr.name   = "nascerrReadExp";
		expReadId     = osThreadNew(nascerr_sch_expReadTask,    NULL, &ioAttr);
	}
    {
    	lowAttr.name  = "nascerrBoot";
		bootloaderId  = osThreadNew(nascerr_sch_bootloaderTask, NULL, &lowAttr);
	}
	{
		lowAttr.name  = "nascerrHwCk";
		hwCheckId     = osThreadNew(nascerr_sch_hwCheckTask,    NULL, &lowAttr);
	}
	{
		lowAttr.name  = "nascerrLogger";
		loggerId      = osThreadNew(nascerr_sch_loggerTask,     NULL, &lowAttr);
    }
	{
		lowAttr.name  = "nascerrMemCk";
		memCheckId    = osThreadNew(nascerr_sch_memCheckTask,   NULL, &lowAttr);
	}
	{
		lowAttr.name  = "nascerrImgUp";
		imgUploadId   = osThreadNew(nascerr_sch_imgUploadTask,  NULL, &lowAttr);
	}

    /* Preenche tabela de eventos */
    uint32_t now = osKernelGetTickCount();
    schedTbl[0] = (EventScheduler_t){ 0u,  now, expWriteId, FLAG_EXP_WRITE,  EVT_ONESHOT };
    schedTbl[1] = (EventScheduler_t){ 0u,  now, expReadId,  FLAG_EXP_READ,  EVT_ONESHOT };
    schedTbl[2] = (EventScheduler_t){ ms_to_ticks(hwCheckPeriodMs),  now, hwCheckId,  FLAG_HW_CHECK,  EVT_PERIODIC };
    schedTbl[3] = (EventScheduler_t){ ms_to_ticks(memCheckPeriodMs),  now, memCheckId,  FLAG_MEMORY_CHECK,  EVT_PERIODIC };
    /* Instancia do Mutex */
    dbMutex = osMutexNew(NULL);
    sramMutex = osMutexNew(NULL);

    UART_RegisterRxEventCallback(nascerr_uart_image_rx_event);
    nascerr_uart_image_register_notify(img_rx_notify_cb);
    nascerr_uart_image_init();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_sch_masterCommTask
 *  @brief     : Polls protocolo I2C; quando ha comando novo processa e coloca
 *               na fila de comandos para o scheduler.
 *  @param     : None
 *  @retval    : None
 *  @note      : Chame esta funcao dentro do loop while(1) da defaultTask
 * ******************************************************************************
 */
void nascerr_sch_masterCommTask(void)
{
    /* Verifica se um novo comando foi recebido pelo protocolo I2C */
#if MOCK_COMMAND
	if(send_mocked) { nascerr_mock_write_cmd(&sched); }

	if (nascerr_protocol_receivedNewCommand() || send_mocked)
#else
    if (nascerr_protocol_receivedNewCommand())
#endif
    {
    	allowCmdEnqueue = true;
        CommandMessage_t cmd = {0};

#if MOCK_COMMAND
        send_mocked = false;
#endif

        /* Processa, decodifica e valida o comando */
        if (nascerr_sch_processCommand(&cmd)) {
        	if(allowCmdEnqueue) {
				/* Enfileira para o scheduler tratar; timeout 0 => nao bloqueia  */
				osStatus_t st = osMessageQueuePut(commandQ, &cmd, 0u, 0u);
				nascerr_register_changeQueueStatus(osMessageQueueGetCount(commandQ));
//				DEBUG_PRINT(LOG_DEBUG, "adicionado fila %lu \n\r", osMessageQueueGetCount(commandQ));

				/* Se fila cheia, registrar log de overflow */
				if (st != osOK) {
					LOG_AND_NOTIFY(LOG_FAIL, LOG_STATUS_WARN, ERR_CMD_OVERFLOW);
					cmd.type == CMD_SET_WRITE_SCH ?
					nascerr_register_changeAckPhase(ACK_FAIL):
					nascerr_registers_changeExpStatus(nascerr_registers_getStatusSystem().OP.EXP, REQ_EXP_FAIL);
					DEBUG_PRINT(LOG_ERROR, "FILA DE COMANDOS ESTOUROU \n\r");
				}
        	}
        }
        else {
            /* Comando invalido – log de erro */
        	LOG_AND_NOTIFY(LOG_FAIL, LOG_STATUS_WARN, ERR_PROTO_CMD_DECODE);
        }
    }

    /* Poll UART transport (ST-Link VCP) for commands from laptop */

    /* Limpar se full */
    DB_LOCK();
    nascerr_schdb_refreshScheduler();
    nascerr_schdb_refreshExperiment();
    DB_UNLOCK();
    /* Verificar Barramento I2C */
	i2c_slave_noBlockBus();

}

/*
 * ******************************************************************************
 *  Function Name: nascerr_sch_schedulerTask
 *  @brief     : Thread central que gerencia eventos periodicos e comandos
 *  @param     : arg – nao utilizado
 *  @retval    : None
 * ******************************************************************************
 */
void nascerr_sch_schedulerTask(void *arg)
{
    (void)arg;
    CommandMessage_t cmd;

    for (;;)
    {
        uint32_t now = osKernelGetTickCount();

        for (int i = 0; i < SCHEDULER_THREADS_COUNT; ++i) {
            EventScheduler_t *evt = &schedTbl[i];

            if (evt->periodTicks && now >= evt->nextWakeTick) {
                osThreadFlagsSet(evt->targetId, evt->flagMask); /* sinaliza thread  */
                if (evt->type == EVT_PERIODIC)
                {
					evt->nextWakeTick = now + evt->periodTicks;
				}
            }

        }

        /* Processamento de comandos vindos da master */
        if (osMessageQueueGet(commandQ, &cmd, NULL, 0u) == osOK) {
        	nascerr_sch_handleCommand(&cmd);
        	nascerr_register_changeQueueStatus(osMessageQueueGetCount(commandQ));
//        	DEBUG_PRINT(LOG_DEBUG, "Retirado fila %lu, cmd: %d\n\r", osMessageQueueGetCount(commandQ), cmd.type);
        	DEBUG_PRINT(LOG_DEBUG, "Retirado fila %lu, cmd: %d\n\r", osMessageQueueGetCount(commandQ), cmd.type);
        }

        /* Atualizar registro */
        nascerr_schdb_updateExperimentStatus();
        update_system_status();

        osDelay(50);  /* folga de 50 ms antes do proximo loop */
    }
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_sch_expWriteTask
 *  @brief     : Executa escrita de experimento na SRAM
 *  @param     : arg – nao utilizado
 *  @retval    : void
 * ******************************************************************************
 */
void nascerr_sch_expWriteTask(void *arg)
{
    (void)arg;
    for (;;)
    {
        osThreadFlagsWait(FLAG_EXP_WRITE, osFlagsWaitAny, osWaitForever);
        if ((schedTbl[0].periodTicks == 0u) && (g_runtimeSched.qtd_block == 0u)) {
			osThreadFlagsClear(FLAG_EXP_WRITE);
			continue;
		}

        DEBUG_PRINT(LOG_INFO,
					"EXP_WRITE_TASK start  | id=%u block=%u  nblocks=%u  sch=%u ms  repeat=%u  tick=%u\r\n",
					g_runtimeSched.id,
					g_runtimeSched.block,
					g_runtimeSched.qtd_block,
					g_runtimeSched.scheduler_time,
					g_runtimeSched.repeat,
					osKernelGetTickCount());

        DB_LOCK();

        SRAM_LOCK();
		if (g_runtimeSched.write_type == WRITE_IMAGE) {
			if (!nascerr_image_flash_write(g_runtimeSched)) {
				LOG_AND_NOTIFY(LOG_FAIL, LOG_STATUS_ERROR, ERR_DB_WRITE);
				nascerr_schdb_updateSchedulerStatus(g_current_idx, DB_CANCELED);
				SRAM_UNLOCK();
				DB_UNLOCK();
				continue;
			}
		} else {
			if(!nascerr_experiment_init(g_runtimeSched)) {
				LOG_AND_NOTIFY(LOG_FAIL, LOG_STATUS_ERROR, ERR_DB_WRITE);
				nascerr_schdb_updateSchedulerStatus(g_current_idx, DB_CANCELED);
				SRAM_UNLOCK();
				DB_UNLOCK();
				continue;
			}
		}
		SRAM_UNLOCK();

        /* Registra que o experimento foi escrito na memoria SRAM */
        nascerr_schdb_updateSchedulerStatus(g_current_idx, DB_EXECUTED);

        /* Atualiza o proximo da fila e reprograma o periodo */
		uint8_t found = nascerr_schdb_getNextNewCommand(&g_current_idx, &g_runtimeSched);
		uint32_t next_ms = 0;

		if (found) {
			/* Tick atual e tempo decorrido desde que o comando entrou */
			uint32_t now_tick   = osKernelGetTickCount();
			uint32_t elapsed_ms = (now_tick - g_runtimeSched.current_tick) * 1000
								  / osKernelGetTickFreq();

			/* Quanto ainda falta ate o horario absoluto programado */
			next_ms = (g_runtimeSched.scheduler_time > elapsed_ms)
					  ? (g_runtimeSched.scheduler_time - elapsed_ms)
					  : 1;  /* se ja passou, chamar em seguida */
		}

		sch_setExpWritePeriodMs(next_ms);

		/* Proxima leitura */
		uint8_t  readFound = nascerr_schdb_getNextRepeatingCommand(&g_read_idx,
		                                                           &g_readExp, osKernelGetTickCount());

		if (readFound) {
		    uint32_t num_done = experimentHeader.scheduler_data
		                                        .scheduled[g_read_idx].num_block;
		    uint32_t next_ms  = sch_next_read_ms(&g_readExp, num_done);
		    sch_setExpReadPeriodMs(next_ms);
		} else {
		    sch_setExpReadPeriodMs(0);/* nenhum comando valido -> desarma o timer */
		}

		DB_UNLOCK();

		LOG_AND_NOTIFY(LOG_EXP_EXECUT, LOG_STATUS_SUCCESS, ERR_NONE);
		osDelay(50);
    }
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_sch_expReadTask
 *  @brief     : Executa leitura de experimento na SRAM e sinaliza master
 *  @param     : arg – nao utilizado
 *  @retval    : void
 * ******************************************************************************
 */
void nascerr_sch_expReadTask(void *arg)
{
    (void)arg;

    for (;;)
    {
        osThreadFlagsWait(FLAG_EXP_READ, osFlagsWaitAny, osWaitForever);
        if ((schedTbl[1].periodTicks == 0u) && (g_readExp.qtd_block == 0u))
		{
			osThreadFlagsClear(FLAG_EXP_READ);
			continue;
		}

        DB_LOCK();

        /* Incializando pagina de experimento no DB */
        if (!nascerr_db_beginExperimentPage(g_readExp.id)) {
		   LOG_AND_NOTIFY(LOG_DB, LOG_STATUS_WARN, ERR_DB_INIT);
		   DB_UNLOCK();
		   continue;
        }

        DEBUG_PRINT(LOG_INFO,
					"EXP_READ_TASK start  | id=%u block=%u  nblocks=%u  delay=%u ms  tick=%u\r\n",
					g_readExp.id,
					g_readExp.block,
					g_readExp.qtd_block,
					g_readExp.delay,
					osKernelGetTickCount());

        NASCERR_EXPERIMENT buffer_read = {0};
        uint16_t pageCount  = 0;
        time_t time_delta = nascerr_schdb_getElapsedMs(g_read_idx, osKernelGetTickCount());
        nascerr_data number_errors = 0, corrected_data = 0;
        SRAM_LOCK();
        uint8_t loop_ok = true;
        if (g_readExp.write_type == WRITE_IMAGE) {
            NASCERR_EXPERIMENT img_result = nascerr_image_flash_read(g_readExp, time_delta);
            nascerr_control_fill_experiment_gps(&img_result, &gps_info);

            if (img_result.ecc_telemetry.ecc_status == ECC_ERR_UNKNOWN)
                LOG_AND_NOTIFY(LOG_FAIL, LOG_STATUS_ERROR, ERR_UNKNOWN);

            if (!nascerr_db_addExperiment(&img_result)) {
                LOG_AND_NOTIFY(LOG_DB, LOG_STATUS_WARN, ERR_DB_WRITE);
                loop_ok = false;
            }
        } else {
        for(int i = 0; i < g_readExp.qtd_block; i++) {

#if MOCK_EXPERIMENT
            nascerr_control_makeExperiment(&buffer_read, g_readExp.id, i, time_delta);  /* mock */
#else
            //auxiliar para realizacao leitura
            NASCERR_SCHEDULER aux = g_readExp;
            aux.block = aux.block+i;
            aux.qtd_block = aux.qtd_block - i;

            //preenchimento info
            buffer_read.id = g_readExp.id;
            buffer_read.block = aux.block;
            buffer_read.data_lenght = g_readExp.qtd_block;
            buffer_read.time_delta = time_delta;
			nascerr_control_fill_experiment_gps(&buffer_read, &gps_info);

            buffer_read.ecc_telemetry = nascerr_experiment_read_result(aux);

            //calculo cumulativo dados com erros e e dados com erros corrigidos
            number_errors =+ nascerr_experiment_number_errors(buffer_read.ecc_telemetry);
            corrected_data =+ nascerr_experiment_number_corrected(buffer_read.ecc_telemetry);

			if(buffer_read.ecc_telemetry.ecc_status == ECC_ERR_UNKNOWN)
			{
				LOG_AND_NOTIFY(LOG_FAIL, LOG_STATUS_ERROR, ERR_UNKNOWN);
			}
			//calculo percentual do ciclo do experimento
			buffer_read.percent_error = nascerr_experiment_synthesise_error_info(number_errors, corrected_data);
			buffer_read.corrected_data = corrected_data;
			buffer_read.number_errors = number_errors;

#endif

        	/* Escrever experimento na Banco de Dados */
        	if (!nascerr_db_addExperiment(&buffer_read)) {
				LOG_AND_NOTIFY(LOG_DB, LOG_STATUS_WARN, ERR_DB_WRITE);
//				SRAM_UNLOCK();
//				DB_UNLOCK();
				loop_ok = false;
				break;
			}

        	pageCount++;

        	/* Commit se a pagina encher */
        	if (pageCount >= MAX_EXPERIMENTS_PER_PAGE) {
				if (!nascerr_db_commitExperimentPage())	{
					LOG_AND_NOTIFY(LOG_DB, LOG_STATUS_WARN, ERR_DB_WRITE);
//					SRAM_UNLOCK();
//					DB_UNLOCK();
					loop_ok = false;
					break;
				}
				if (!nascerr_db_beginExperimentPage(g_readExp.id)) {
					LOG_AND_NOTIFY(LOG_DB, LOG_STATUS_WARN, ERR_DB_INIT);
//					SRAM_UNLOCK();
//					DB_UNLOCK();
					loop_ok = false;
					break;
				}
				pageCount = 0;
			}
        	i2c_slave_noBlockBus();
		}
        } /* end else (non-image experiment) */

        SRAM_UNLOCK();

        if(!loop_ok){
        	DB_UNLOCK();
        	continue;
        }


        /* Finalizar escrita */
        nascerr_db_commitExperimentPage();

        /* Registra que o experimento foi executado */
		nascerr_schdb_executeSchedulerCommand(g_read_idx);

		/* Agenda o proxima leitura */
		uint8_t found = nascerr_schdb_getNextRepeatingCommand(&g_read_idx, &g_readExp, osKernelGetTickCount());
		uint8_t num_done = 0;
		if (found) {
		    num_done = experimentHeader.scheduler_data
		                               .scheduled[g_read_idx].num_block;
		}

		DEBUG_PRINT(LOG_DEBUG,
		    "g_readExp check | current_tick=%lu  sched_time=%lu  delay=%lu  num_done=%lu\r\n",
		    g_readExp.current_tick,
		    g_readExp.scheduler_time,
		    g_readExp.delay,
		    num_done);

		uint32_t next_ms = found ? sch_next_read_ms(&g_readExp, num_done) : 0u;

		/* Programa o timer para a janela futura */
		sch_setExpReadPeriodMs(next_ms);

		DB_UNLOCK();

		nascerr_schdb_updateRegistersExpSumary(0);

		LOG_AND_NOTIFY(LOG_EXP_FINNISH, LOG_STATUS_SUCCESS, ERR_NONE);
    }
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_sch_hwCheckTask
 *  @brief     : Executa verificacao periodica de hardware
 *  @param     : arg – nao utilizado
 *  @retval    : void
 * ******************************************************************************
 */
void nascerr_sch_hwCheckTask(void *arg)
{
    (void)arg;

    uint8_t prev_flags = 0;
    const uint32_t period = 10;
    const uint32_t window = 1000;


//    uint8_t        j          = 0;
//	const uint32_t image_size = 2000;        // tamanho real da "imagem" na flash
//	uint8_t        buffer[IMG_CHUNK_SIZE];   // só um chunk na stack

    for (;;)
    {
        osThreadFlagsWait(FLAG_HW_CHECK, osFlagsWaitAny, osWaitForever);

        uint32_t start = osKernelGetTickCount();
        uint32_t next  = start + period;

        while ((osKernelGetTickCount() - start) < window)
        {
            nascerr_hw_adc_convert_to_currents();
            nascerr_hw_adc_convert_to_temperature();
            nascerr_hw_check_limits();

            uint8_t curr_flags   = nascerr_hw_get_fail_flag();
            uint8_t rising_flags = curr_flags & ~prev_flags;

            if (rising_flags)
            {
                if (rising_flags & (1u << 5))
                    LOG_AND_NOTIFY(LOG_HW_WARN, LOG_STATUS_WARN, WARN_HW_TEMPERATURE_THRESHOLD);

                if (rising_flags & ~(1u << 5))
                    LOG_AND_NOTIFY(LOG_HW_WARN, LOG_STATUS_WARN, WARN_HW_CURRENT_THRESHOLD);
            }

            prev_flags = curr_flags;
            i2c_slave_noBlockBus();

            osDelayUntil(next);
            next += period;
        }

        /* Imprime telemetria ao final de cada ciclo de verificação */
        NASCERR_HW_RESPONSE hw = NASCERR_HWRead();
        uint8_t fail_flags = nascerr_hw_get_fail_flag();
        DEBUG_PRINT(LOG_INFO, "[HW] T=%.2f C | fail=0x%02X\r\n", hw.temp, fail_flags);
        DEBUG_PRINT(LOG_INFO, "[HW] I(mA): 1V2=%.2f | 2V5=%.2f | 2V5vpp=%.2f | 3V3=%.2f | 3V3fpga=%.2f\r\n", hw.currents[0], hw.currents[1], hw.currents[2], hw.currents[3], hw.currents[4]);
        DEBUG_PRINT(LOG_INFO, "[HW] P(mW): 1V2=%.2f | 2V5=%.2f | 2V5vpp=%.2f | 3V3=%.2f | 3V3fpga=%.2f | Total=%.2f\r\n", hw.powers[0], hw.powers[1], hw.powers[2], hw.powers[3], hw.powers[4], hw.total_power);

	   DB_UNLOCK();


    }
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_sch_bootloaderTask
 *  @brief     : Placeholder – executa quando a Master pedir transicao p/ bootloader
 *  @param     : void
 * ******************************************************************************
 */
void nascerr_sch_bootloaderTask(void *arg)
{
  (void)arg;
  for (;;) {
    // bloqueia até o comando “entra em bootloader”
    osThreadFlagsWait(FLAG_BOOTLOADER_TRIGGER, osFlagsWaitAny, osWaitForever);

    DEBUG_PRINT(LOG_INFO, "BOOTLOADER MODE: Suspendendo experimentos...\r\n");

    /* SUSPENDE TODAS AS TASKS EXPERIMENTAIS */
    osThreadSuspend(expWriteId);
    osThreadSuspend(expReadId);
    osThreadSuspend(hwCheckId);
    osThreadSuspend(memCheckId);

    /* Desabilita timers periódicos */
    sch_setExpWritePeriodMs(0);
    sch_setExpReadPeriodMs(0);
    sch_setHwCheckPeriodMs(0);
    sch_setMemCheckPeriodMs(0);

    DEBUG_PRINT(LOG_INFO, "BOOTLOADER MODE: Iniciando recepcao\r\n");

    // loop de serviço não-bloqueante
    while (boot_in_process) {
		vTaskSuspendAll();
		nascerr_bootloader_fsmStep();
		xTaskResumeAll();
		osDelay(1u);
    }

    DEBUG_PRINT(LOG_INFO, "BOOTLOADER MODE: Saindo do modo de recepcao...\r\n");

    /* REATIVA TASKS */
    osThreadResume(expWriteId);
    osThreadResume(expReadId);
    osThreadResume(hwCheckId);
    osThreadResume(memCheckId);

    /* Restaura período do HW check */
    sch_setHwCheckPeriodMs(hwCheckPeriodMs);
    sch_setMemCheckPeriodMs(memCheckPeriodMs);
  }
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_sch_loggerTask
 *  @brief     : Placeholder – grava logs quando sinalizado
 *  @param     : void
 * ******************************************************************************
 */
void nascerr_sch_loggerTask(void *arg)
{
    for (;;)
    {
        /* Aguarda flag de qualquer produtor de log */
        osThreadFlagsWait(1u, osFlagsWaitAny, osWaitForever);

		DB_LOCK();
		switch(nascerr_flash_log_append((uint8_t *)nascerr_log_get(), sizeof(NASCERR_LOG))){
			case 1:
//				DEBUG_PRINT(LOG_DEBUG, "Recorded successfully\r\n");
				nascerr_log_clear();
				break;
			case 0:
//				DEBUG_PRINT(LOG_WARN, "Recording error. Resetting internal log buffer\r\n");
				(void)nascerr_flash_reset_log_buffer();
				break;
			default:
//				DEBUG_PRINT(LOG_INFO, "Memoria interna cheia. Gravando na memoria externa...\r\n"); //TODO: verificar a necessidade desses debug dps
				if(nascerr_sch_recording_memory_external()){
//					DEBUG_PRINT(LOG_INFO, "Retentando gravar logs da RAM na memoria interna...\r\n");
					if (nascerr_flash_log_append((uint8_t *)nascerr_log_get(), sizeof(NASCERR_LOG)) == 1) {
		//				nascerr_log_clear();
					} else {
						DEBUG_PRINT(LOG_ERROR, "Falha ao gravar na memoria interna apos limpeza!\r\n");
					}
				}
				nascerr_schdb_updateRegistersLogSummary(g_register_data[LOG_SET_PAGE].value[0]);
				break;
		}
		DB_UNLOCK();
		osDelay(50);
	}
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_sch_memCheckTask
 *  @brief     : Executa verificacao periodica da SRAM e encoders/decoders
 *  @param     : arg – nao utilizado
 *  @retval    : void
 * ******************************************************************************
 */
void nascerr_sch_memCheckTask(void *arg){
	(void)arg;

	uint64_t prev_corrupted_1 = 0;
	uint64_t prev_corrupted_2 = 0;

	uint8_t prev_encdec_fault = false;

	uint32_t prev_encdec_error_sum = 0;

	Robust_Memory_Health robust_mem = {0};

	uint16_t full_check_counter = 0;

	for (;;)
	{
		osThreadFlagsWait(FLAG_MEMORY_CHECK, osFlagsWaitAny, osWaitForever);

		if(nascerr_registers_getStatusSystem().OP.EXP == OP_EXP_IDLE)
		{
			SRAM_LOCK();
			if(full_check_counter < FULL_SRAMs_CHECK_CYCLE_TRIGGER)
			{
				robust_mem = nascerr_memory_encoder_decoders_selfcheck();
//				DEBUG_PRINT(LOG_DEBUG, "ENCODERS_DECODERS_SELFCHECK_FINISHED\r\n");
			}
			else
			{
				robust_mem = nascerr_memory_complete_selfcheck();
				DEBUG_PRINT(LOG_DEBUG, "MEMORIES_SELFCHECK_FINISHED\r\n");
				full_check_counter = 0;
			}
			SRAM_UNLOCK();
		}

		uint64_t curr_corrupted_1 = robust_mem.srams_health.mem_1_affected_blocks;
		uint64_t curr_corrupted_2 = robust_mem.srams_health.mem_2_affected_blocks;

		uint64_t new_corrupted_1 = curr_corrupted_1 & ~prev_corrupted_1;
		uint64_t new_corrupted_2 = curr_corrupted_2 & ~prev_corrupted_2;


		if (new_corrupted_1 || new_corrupted_2)
		{
			DEBUG_PRINT(LOG_WARN, "Bloco Corrompido \r\n");
			if(new_corrupted_1)
			{
				LOG_AND_NOTIFY(LOG_ROBUST_MEM_HEALTH, LOG_STATUS_WARN, WARN_ROBUST_MEM_SRAM_1);
			}
			else
			{
				LOG_AND_NOTIFY(LOG_ROBUST_MEM_HEALTH, LOG_STATUS_WARN, WARN_ROBUST_MEM_SRAM_2);
			}
		}

		uint8_t curr_encdec_fault = (robust_mem.enc_dec_health.errors_mrsc == 0xFFFFu) ||
		            				(robust_mem.enc_dec_health.errors_tbec == 0xFFFFu);

		if (curr_encdec_fault && !prev_encdec_fault)
		{
			LOG_AND_NOTIFY(LOG_ROBUST_MEM_HEALTH,
						   LOG_STATUS_ERROR,
						   ERR_ROBUST_MEM_ENC_DEC);
		}

		uint32_t curr_encdec_error_sum = robust_mem.enc_dec_health.errors_mrsc +
										 robust_mem.enc_dec_health.errors_tbec;

		if (curr_encdec_error_sum > prev_encdec_error_sum)
		{
			LOG_AND_NOTIFY(LOG_ROBUST_MEM_HEALTH,
						   LOG_STATUS_WARN,
						   WARN_ROBUST_MEM_ENC_DEC);
		}

		prev_encdec_fault     = curr_encdec_fault;
		prev_encdec_error_sum = curr_encdec_error_sum;

		prev_corrupted_1 = curr_corrupted_1;
		prev_corrupted_2 = curr_corrupted_2;
		full_check_counter++;

		i2c_slave_noBlockBus();
	}
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_sch_imgUploadTask
 *  @brief     : Processa frames de upload de imagem recebidos via UART/DMA
 *  @param     : arg – não utilizado
 *  @retval    : void
 * ******************************************************************************
 */
void nascerr_sch_imgUploadTask(void *arg)
{
    (void)arg;

    for (;;)
    {
        osThreadFlagsWait(FLAG_IMG_DMA_RX_DONE, osFlagsWaitAny, osWaitForever);

        DB_LOCK();
        nascerr_uart_image_process_frame();
        DB_UNLOCK();

        i2c_slave_noBlockBus();

        /* DMA circular continua sozinho; restart só em erro de hardware (FE/NE/ORE) */
        if (nascerr_uart_image_need_restart())
            nascerr_uart_image_restart_dma();
    }
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_sch_processCommand
 *  @brief     : Converte frame I2C em CommandMessage_t para o escalonador
 *  @param     : cmd – ponteiro para estrutura de saida
 *  @retval    : uint8_t (0 = erro, 1 = sucesso)
 * ******************************************************************************
 */
uint8_t nascerr_sch_processCommand(CommandMessage_t *cmd)
{
	/* Le o comando bruto do protocolo */
	NASCERR_COMMANDS protoCmd = nascerr_protocol_processCommand();

#if MOCK_COMMAND
	if(protoCmd == NO_COMMANDS)
	{
		protoCmd = CMD_RECEIVED;
	}
#endif

	/* Caso nao tenha comando novo, retorna falha */
	if (protoCmd == NO_COMMANDS) { return false;}

    /* Mapeia tipo de comando */
    switch (protoCmd)
    {
        case CMD_RECEIVED:
        {
        	uint8_t raw[sizeof(NASCERR_CMD)];
        	memcpy(raw, g_register_data[CMD].value, sizeof(NASCERR_CMD));

        	 NASCERR_CMD local;
        	if(!nascerr_control_cmdCasting(&local, raw)) {
        		LOG_AND_NOTIFY(LOG_EXP_RECEIVED, LOG_STATUS_WARN, WARN_COMM_DECODE);
				DEBUG_PRINT(LOG_WARN, "Command decode failed\r\n");
				return false; /* falha ao decodificar comando */
			}

			local.scheduler.current_tick = osKernelGetTickCount(); /* Tick atual */

			if(local.cmd_type == WRITE_CMD) {
				cmd->type  = CMD_SET_WRITE_SCH;
				cmd->payload_len = sizeof(NASCERR_SCHEDULER);
				memcpy(cmd->payload, &local.scheduler, cmd->payload_len);
				nascerr_register_changeAckPhase(ACK_WRITE_RECEIVED);
			} else {
				DEBUG_PRINT(LOG_INFO, "New read command received");
				NASCERR_QUERY query = {0};
				sch_to_query(&local.scheduler, &query);

				allowCmdEnqueue = false;
				nascerr_registers_changeExpStatus(nascerr_registers_getStatusSystem().OP.EXP, REQ_EXP_CMD_RECEIVED);
				handle_read_memory(&query);
			}

			cmd->param_u32   = cmd->payload_len;
			cmd->payload_crc = calculate_crc16(cmd->payload, cmd->payload_len);

        	LOG_AND_NOTIFY(LOG_RECEIVE_COMMAND, LOG_STATUS_SUCCESS, ERR_NONE);

			DEBUG_PRINT(LOG_INFO, "Command received: %s\r\n",
						(local.cmd_type == WRITE_CMD) ? "WRITE" : "READ");

            break;
        }
        case SET_PAGE_SUMARY:
        	DEBUG_PRINT(LOG_INFO, "Command de summery\n");
        	nascerr_schdb_updateRegistersExpSumary(g_register_data[EXP_SUM_IDX].value[0]);
        	{
        		NASCERR_QUERY query = {0};
        		query.page = g_register_data[EXP_SUM_IDX].value[0];
        		query.qtd_block = 0u;
        		handle_read_memory(&query);
        	}
        	allowCmdEnqueue = false;
        	break;

        case LOG_SUMMARY:
        	DEBUG_PRINT(LOG_INFO, "CMD summary\n");
            cmd->type = CMD_PREPARE_LOG_PAGE;
            cmd->param_u32 = g_register_data[LOG_SET_PAGE].value[0];
            cmd->payload_len = 0;
            cmd->payload_crc = calculate_crc16(cmd->payload, 0);
        	break;

        case LOG_MARK_READ:
        	DEBUG_PRINT(LOG_INFO, "LOG MRK READ\n");
        	cmd->type = CMD_MARK_LOG_PAGE;
			cmd->param_u32 = g_register_data[LOG_SET_PAGE].value[0];
			cmd->payload_len = 0;
			cmd->payload_crc = calculate_crc16(cmd->payload, 0);
			break;

        case CLEAR_DB_COMMAND:
        	allowCmdEnqueue = false;
			DB_LOCK();
			nascerr_schdb_clearExpArea((FlashRegionTypeDef) g_register_data[DB_CLEAR].value[0]);
			nascerr_schdb_updateRegistersLogSummary(g_register_data[LOG_SET_PAGE].value[0]);
			DB_UNLOCK();
			break;

        case COMMAND_TO_SLEEP:
        	//Salvar direto os ticks como estam, muda-los para zero e depois entrar em shutdown.
        	if(nascerr_hw_check_sleep()) {
        		//Não deixa enfileirar esse comando (intermediário entre protocol e Scheduler)
        		allowCmdEnqueue = false;
        		tick_context[2] = hwCheckPeriodMs;
        		tick_context[3] = memCheckPeriodMs;

        		//Desinibe as ativações periódicas. (As por demanda também, tanto as executadas agora quanto as que viriam ser executadas)

        		osThreadSuspend(expWriteId);
        		osThreadSuspend(expReadId);
        		osThreadSuspend(hwCheckId);
        		osThreadSuspend(memCheckId);

        		sch_setExpWritePeriodMs(0);
        		sch_setExpReadPeriodMs(0);
        		sch_setHwCheckPeriodMs(0);
        		sch_setMemCheckPeriodMs(0);

        		DEBUG_PRINT(LOG_DEBUG, "Dormiu \n\r");
        		//Desliga efetivamente.
        		if(!(nascerr_schdb_clearExpArea(FLASH_SCHEDULER) && nascerr_schdb_clearExpArea(FLASH_EXPERIMENT)))
        		{
        			DEBUG_PRINT(LOG_WARN, "Nao limpou o header \n\r");
        		}
            	nascerr_hw_shutdown();
        	}
        	break;

        case COMMAND_TO_WAKEUP:
        	if(nascerr_hw_check_wakeup())
        	{
        		allowCmdEnqueue = false;

        		//Reativa o sistema
        		nascerr_hw_waking_up();

        		osThreadResume(expWriteId);
        		osThreadResume(expReadId);
        		osThreadResume(hwCheckId);
        		osThreadResume(memCheckId);
        		//Recupera o contexto
				sch_setHwCheckPeriodMs(tick_context[2]);
				sch_setMemCheckPeriodMs(tick_context[3]);
        	}
        	break;

        case GPS_DATA_RECEIVED:
        	if(nascerr_control_gps_check_crc())
        	{
        		memcpy(&gps_info, g_register_data[GPS].value, g_register_data[GPS].size - 2);
        	}
        	return false;
        	break;

        case BOOT_COMMAND:
        {
        	BOOT_DATA_PACKET local;
        	uint8_t *raw = g_register_data[BOOT_DATA].value;


        	if(!nascerr_control_castingBootCMD(&local, raw)) {
        		nascerr_bootloader_resetRuntimeFlag(BOOT_FLAG_ERROR_DETECTED);
        		LOG_AND_NOTIFY(LOG_BOOT_RECEIVE_COMMAND, LOG_STATUS_ERROR, ERR_BOOT_CMD_DECODE);
				return false;
			}
        	LOG_AND_NOTIFY(LOG_BOOT_INITIALIZED, LOG_STATUS_SUCCESS, ERR_NONE);
        	nascerr_bootloader_resetRuntimeFlag(BOOT_FLAG_IN_DOWNLOAD);

        	/* Tamanho do payload de dados uteis */
        	cmd->param_u32  = local.dataSize;

        	/* Preenche campos genericos */
			cmd->type       = CMD_TO_BOOTLOADER;

			/* Copia scheduler para payload */
			cmd->payload_len = sizeof(BOOT_DATA_PACKET); // tamanho do payload recebido (deve somar os outros padroes?)

			memcpy(cmd->payload, &local, sizeof(BOOT_DATA_PACKET)); // payload da mensagem

			/* Calcula CRC16 do payload */
			cmd->payload_crc = calculate_crc16(cmd->payload, cmd->payload_len);

            break;
        }

        case EXP_TRANSFER_COMPLETE:
        	DB_LOCK();
        	nascerr_db_setReadExperimentPage(g_readPage);
        	nascerr_schdb_updateRegistersExpSumary(0);
        	DB_UNLOCK();

        	allowCmdEnqueue = false;
        	DEBUG_PRINT(LOG_INFO, "Experiment transfer complete\r\n");
        	break;

        case ROBUST_MEM_HEALTH_READ:
        	restore_robust_mem_info = true;
        	allowCmdEnqueue = false;
        	DEBUG_PRINT(LOG_INFO, "Robust Memory info restored \r\n");
        	nascerr_memory_clear_health_info();
        	break;

        default:
            return false; /* tipo desconhecido */
    }

    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_sch_handleCommand
 *  @brief     : Processa comandos vindos da Master
 *  @param     : cmd – ponteiro para mensagem de comando
 *  @retval    : None
 * ******************************************************************************
 */
void nascerr_sch_handleCommand(const CommandMessage_t *cmd)
{
    NASCERR_SCHEDULER newSched = {0};
    NASCERR_QUERY query = {0};


    if (!sch_check_payload_crc(cmd)) return;

    switch (cmd->type)
    {
        case CMD_SET_WRITE_SCH:
        {
            /* Copiar payload para struct local */
            memcpy(&newSched, cmd->payload, sizeof(newSched));
            nascerr_register_changeAckPhase(ACK_CMD_PROCESSING);
            handle_write_scheduler(&newSched);
        }
            break;
        case CMD_READ_MEMORY:
        {
        	memcpy(&query, cmd->payload, sizeof(query));
        	handle_read_memory(&query);
        }
            break;
        case CMD_TO_BOOTLOADER:
        	/* Se o bootloader ja estiver em processo, ignora o comando */
			if (boot_in_process) {
				return;
			}
			/* Copia payload para struct local */
			if (cmd->payload_len > sizeof(BOOT_DATA_PACKET)) {
				LOG_AND_NOTIFY(LOG_BOOT, LOG_STATUS_WARN, ERR_BOOT_CMD_SIZE);
				return;
			}

			BOOT_DATA_PACKET bootData;
			memcpy(&bootData, cmd->payload, cmd->payload_len);

			/* Verifica se o tamanho do payload e valido */
			if (bootData.dataSize > REG_DATA_BOOT_SIZE) {
				LOG_AND_NOTIFY(LOG_BOOT, LOG_STATUS_WARN, ERR_BOOT_CMD_SIZE);
				return;
			}

        	if (!nascerr_bootloader_init(&bootData)) {
        		LOG_AND_NOTIFY(LOG_BOOT, LOG_STATUS_WARN, ERR_BOOT_CMD_INIT);
        		return;
        	 }
        	nascerr_bootloader_fsmInit();
        	osThreadFlagsSet(bootloaderId, 1u);

            break;
        case CMD_SET_HW_CHECK_PERIOD:
        	sch_setHwCheckPeriodMs(cmd->param_u32);
            break;

        case CMD_PREPARE_LOG_PAGE:
		{
			uint8_t page_to_prepare = (uint8_t)cmd->param_u32;
//			DEBUG_PRINT(LOG_INFO, "Preparando pagina %u\n\r", page_to_prepare); TODO: adicionei o logs system
			DB_LOCK();
			nascerr_schdb_prepareLogResponse(page_to_prepare);
			nascerr_schdb_updateRegistersLogSummary(page_to_prepare);
			DB_UNLOCK();

			break;
		}
        case CMD_MARK_LOG_PAGE:
		{
			uint8_t page_to_prepare = (uint8_t)cmd->param_u32;
//			DEBUG_PRINT(LOG_INFO, "Marcando pagina de log %u como lida...\n\r", page_to_prepare);
			DB_LOCK();
			nascerr_schdb_log_markReadAndRefresh(page_to_prepare);
			DB_UNLOCK();

			break;
		}
        default:
        {
        	LOG_AND_NOTIFY(LOG_FAIL, LOG_STATUS_WARN, WARN_COMM_DECODE);
        }
            break;
    }
}

/*
 * ******************************************************************************
 *  Local Functions Definitions
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: ms_to_ticks
 *  @brief     : Converte millisegundos em ticks do kernel
 *  @param     : ms – valor em millisegundos
 *  @retval    : numero de ticks correspondente
 * ******************************************************************************
 */
static uint32_t ms_to_ticks(uint32_t ms)
{
    return (ms * osKernelGetTickFreq() + 999u) / 1000u;      /* > Arredonda */
}

/*
 * ******************************************************************************
 *  Function Name: sch_reset_event
 *  @brief     : Atualiza periodo e reinicia contador de um evento
 *  @param     : e   – ponteiro para evento
 *  @param     : newMs – novo periodo em ms
 *  @retval    : void
 * ******************************************************************************
 */
static void sch_reset_event(EventScheduler_t *e, uint32_t newMs)
{
	e->periodTicks  = newMs ? ms_to_ticks(newMs) : 0u;
    e->nextWakeTick = (e->periodTicks > 0u)
                          ? osKernelGetTickCount() + e->periodTicks
                          : 0u;
}

/*
 * ******************************************************************************
 *  Function Name: sch_setExpWritePeriodMs
 *  @brief     : Ajusta periodo da escrita de experimento
 *  @param     : ms – novo periodo em ms
 *  @retval    : None
 * ******************************************************************************
 */
static void sch_setExpWritePeriodMs(uint32_t ms)
{
    expWritePeriodMs = ms;
    sch_reset_event(&schedTbl[0], ms);
}

/*
 * ******************************************************************************
 *  Function Name: sch_setExpReadPeriodMs
 *  @brief     : Ajusta periodo da leitura de experimento
 *  @param     : ms – novo periodo em ms
 *  @retval    : Noned
 * ******************************************************************************
 */
static void sch_setExpReadPeriodMs(uint32_t ms)
{
    expReadPeriodMs = ms;
    sch_reset_event(&schedTbl[1], ms);
}

/*
 * ******************************************************************************
 *  Function Name: sch_setHwCheckPeriodMs
 *  @brief     : Ajusta periodo da verificacao de hardware
 *  @param     : ms – novo periodo em ms
 *  @retval    : None
 * ******************************************************************************
 */
static void sch_setHwCheckPeriodMs(uint32_t ms)
{
    hwCheckPeriodMs = ms;
    sch_reset_event(&schedTbl[2], ms);
}

/*
 * ******************************************************************************
 *  Function Name: sch_setMemCheckPeriodMs
 *  @brief     : Ajusta periodo da verificacao de memoria e dos enconders/decoders
 *  @param     : ms – novo periodo em ms
 *  @retval    : None
 * ******************************************************************************
 */
static void sch_setMemCheckPeriodMs(uint32_t ms)
{
    memCheckPeriodMs = ms;
    sch_reset_event(&schedTbl[3], ms);
}

/*
 * ******************************************************************************
 *  Function Name: sch_check_payload_crc
 *  @brief     : Verify that the calculated CRC16 of the payload matches the
 *               received CRC16; on mismatch, log the error and wake the logger
 *  @param     : cmd - Pointer to CommandMessage_t with payload, length and CRC
 *  @retval    : true if CRCs match (SUCCESS); false if they differ (FAIL)
 * ******************************************************************************
 */
static uint8_t sch_check_payload_crc(const CommandMessage_t *cmd)
{
    if (verify_crc(cmd->payload, cmd->payload_len, cmd->payload_crc)) return true;

    if (cmd->payload_len > 3) LOG_AND_NOTIFY(LOG_CRC, LOG_STATUS_WARN, ERR_CMD_CRC);
    return false;
}

/*
 * ******************************************************************************
 *  Function Name: sch_next_read_ms
 *  @brief     : Quanto falta (ms) para a proxima leitura do experimento
 *  @param     : s         – ponteiro p/ NASCERR_SCHEDULER
 *               num_done  – leituras ja concluidas (0, 1, 2…)
 *  @retval    : Milissegundos ate a proxima leitura (minimo 100 ms)
 * ******************************************************************************
 */
static uint32_t sch_next_read_ms(const NASCERR_SCHEDULER *s, uint32_t num_done)
{
    uint32_t freq      = osKernelGetTickFreq();
    uint32_t now_ticks = osKernelGetTickCount();

//    DEBUG_PRINT(LOG_DEBUG,"PASSOU");

    uint64_t ms_to_last = (uint64_t)s->scheduler_time
                        + (uint64_t)num_done * s->delay;

    uint32_t ideal_last_tick = s->current_tick
                             + (uint32_t)(ms_to_last * freq / 1000u);
    uint32_t ideal_next_tick = ideal_last_tick
                             + (uint32_t)((uint64_t)s->delay * freq / 1000u);
    uint32_t result;

    if (now_ticks >= ideal_next_tick)
        result = 100u;
    else
        result = (uint32_t)((uint64_t)(ideal_next_tick - now_ticks) * 1000u / freq);

    // DEBUG TEMPORÁRIO
    uint32_t ms_to_last_lo = (uint32_t)(ms_to_last & 0xFFFFFFFF);
    uint32_t ms_to_last_hi = (uint32_t)(ms_to_last >> 32);

    DEBUG_PRINT(LOG_DEBUG,
        "sch_next_read | num_done=%lu  ms_to_last=%lu_%lu"
        "  ideal_last=%lu  ideal_next=%lu  now=%lu  result=%lu ms\r\n",
        num_done, ms_to_last_hi, ms_to_last_lo,
        ideal_last_tick, ideal_next_tick, now_ticks, result);
    return result;
}

/*
 * ******************************************************************************
 *  Function Name: scheduler_to_query
 *  @brief     : Converte os campos relevantes de uma estrutura
 *               NASCERR_SCHEDULER em uma NASCERR_QUERY compacta, usada
 *               na operacao READ_CMD.
 *  @param     : sch – ponteiro para NASCERR_SCHEDULER de entrada
 *               q   – ponteiro para NASCERR_QUERY de saída (preenchida)
 *  @retval    : Nenhum (void)
 * ******************************************************************************
 */
static void sch_to_query(const NASCERR_SCHEDULER *sch, NASCERR_QUERY *q)
{
	q->id        = sch->id;
	q->page      = sch->block;       /* block to OFFSET/PAGE             */
	q->qtd_block = sch->qtd_block;   /* 0 = enviar maior janela possivel */
}

/*
 * ******************************************************************************
 *  Function Name: handle_write_scheduler
 *  @brief     : Processa CMD_SET_WRITE_SCH – grava um novo experimento no banco
 *  @param     : newSched – ponteiro para NASCERR_SCHEDULER recebido
 *  @retval    : None
 * ******************************************************************************
 */
static void handle_write_scheduler(const NASCERR_SCHEDULER *newSched)
{
	DB_LOCK();

	/* Verifica se full */
	if(nascerr_schdb_verifySchedulerFull()) {
		LOG_AND_NOTIFY(LOG_EXP_RECEIVED, LOG_STATUS_WARN, ERR_EXP_SCH_FULL);
		DEBUG_PRINT(LOG_WARN, "Scheduler is full, cannot add new command\r\n");
		nascerr_register_changeAckPhase(ACK_FAIL);
		DB_UNLOCK();
		return;
	}

	/* Verifica conflito de escrita */
	if (nascerr_schdb_checkSchedulerConflict(newSched->block,
											 newSched->qtd_block))
	{
		LOG_AND_NOTIFY(LOG_EXP_RECEIVED, LOG_STATUS_WARN, ERR_EXP_CONFLICT);
		DEBUG_PRINT(LOG_WARN, "Scheduler in Conflict, cannot add new command\r\n");
		nascerr_register_changeAckPhase(ACK_FAIL);
		DB_UNLOCK();
		return;
	}

	/* Gravar no banco */
	uint8_t db_ok = nascerr_db_writeScheduler(newSched);
	if (!db_ok) {
		LOG_AND_NOTIFY(LOG_DB, LOG_STATUS_WARN, ERR_DB_WRITE);
		nascerr_register_changeAckPhase(ACK_FAIL);
		DB_UNLOCK();
		return;
	}

	nascerr_register_changeAckPhase(ACK_SUCESS);

	/* Procurar escalonamento pendente apos a gravacao */
	uint8_t idx;
	NASCERR_SCHEDULER next;
	uint8_t haveNext = nascerr_schdb_getNextNewCommand(&idx, &next);
	nascerr_registers_changeExpStatus(OP_EXP_RUNNING, nascerr_registers_getStatusSystem().REQ.EXP);

	DB_UNLOCK();

	if (haveNext) {
		/* existe experimento pendente no banco -> executa o mais antigo */
		g_runtimeSched = next;
		g_current_idx = idx;

	} else {
		/* DB vazio OU gravacao falhou -> executa o que acabou de chegar */
		g_runtimeSched = *newSched;
		g_current_idx++;
	}

	/* se delay for zero, ja dispara uma execucao unica   */
	if (g_runtimeSched.scheduler_time == 0u) {
		osThreadFlagsSet(expWriteId, FLAG_EXP_WRITE);
	} else {
		/* reprograma apenas se o novo alvo ficar antes do ja agendado */
		uint32_t nowTick        = osKernelGetTickCount();
		uint32_t newTicks       = ms_to_ticks(g_runtimeSched.scheduler_time);

		uint32_t remainingTicks = 0u;
		if (schedTbl[0].periodTicks != 0u) {
			if (schedTbl[0].nextWakeTick > nowTick)
				remainingTicks = schedTbl[0].nextWakeTick - nowTick;
		}

		if (remainingTicks == 0u || newTicks < remainingTicks) {
			sch_setExpWritePeriodMs(g_runtimeSched.scheduler_time);
#if ENABLE_SERIAL_SYS
			DEBUG_PRINT(LOG_DEBUG,
				"SCHEDULER_EXP_WRITE re-armed | newTicks=%lu  prevRemain=%lu\r\n",
				newTicks, remainingTicks);
#endif
		} else{
#if ENABLE_SERIAL_SYS
			DEBUG_PRINT(LOG_DEBUG,
				"SCHEDULER_EXP_WRITE kept     | newTicks=%lu  prevRemain=%lu\r\n",
				newTicks, remainingTicks);
#endif
		}
#if ENABLE_SERIAL_SYS
		DEBUG_PRINT(LOG_DEBUG, "Current Tick = %lu\r\n",	nowTick);
#endif
	}

	LOG_AND_NOTIFY(LOG_SUCCESS, LOG_STATUS_SUCCESS, ERR_NONE);
}

/*
 * ******************************************************************************
 *  Function Name: handle_read_memory
 *  @brief     : Processa CMD_READ_MEMORY – envia experimentos via TXB
 *  @param     : qry – ponteiro para NASCERR_QUERY com id, page(offset) e qtd
 *  @retval    : None
 * ******************************************************************************
 */
static void handle_read_memory(const NASCERR_QUERY *qry)
{
	uint8_t *bytePointer;
	uint16_t response_crc = 0;

	DEBUG_PRINT(LOG_INFO, "Read Recieved %d\r\n", qry->page); // TODO: DELETAR
	if (qry->page >= experimentHeader.experiment_data.qtd_experiments) {
		LOG_AND_NOTIFY(LOG_EXP_RECEIVED, LOG_STATUS_WARN, ERR_EXP_READ_OUT_OF_RANGE);
		DEBUG_PRINT(LOG_WARN, "Read query page %u out of range\r\n", qry->page);
		nascerr_registers_changeExpStatus(nascerr_registers_getStatusSystem().OP.EXP, REQ_EXP_FAIL);
		return;
	}
	nascerr_registers_changeExpStatus(nascerr_registers_getStatusSystem().OP.EXP, REQ_EXP_READ_PROCESSING);
	DB_LOCK();

	/* Guarda pagina atual */
	g_readPage = qry->page;

	/* Ler pagina */
	NASCERR_EXPERIMENT pageBuf[MAX_EXPERIMENTS_PER_PAGE] = {0};
	if (!nascerr_db_readExperimentPage(qry->page, pageBuf)) {
		LOG_AND_NOTIFY(LOG_DB, LOG_STATUS_WARN, ERR_DB_READ);
		DEBUG_PRINT(LOG_ERROR, "Failed to read experiment page %u\r\n", qry->page);
		DB_UNLOCK();
		return;
	}
	DB_UNLOCK();

	/* Define quantos structs mandar */
	uint16_t total_structs = experimentHeader
							 .experiment_data.experiments[qry->page]
							 .num_block;

	uint16_t n_to_send = (qry->qtd_block == 0u || qry->qtd_block > total_structs)
						 ? total_structs
						 : qry->qtd_block;

	/* Copia n_to_send primeiros slots para TXB */
	uint16_t bytes = n_to_send * sizeof(NASCERR_EXPERIMENT);
	memcpy(g_register_data[TXB].value, pageBuf, bytes);

	/* Zero-pad o resto do TXB */
	if (bytes < g_register_data[TXB].size) {
		memset(g_register_data[TXB].value + bytes, 0, g_register_data[TXB].size - bytes);
	}

	/* IDX → apenas o tamanho real copiado */
	g_register_data[IDX].value[0] = (uint8_t)(bytes + 2);
	g_register_data[IDX].value[1] = (uint8_t)((bytes + 2) >> 8);

	/* Set CRC Response */
	response_crc = calculate_crc16(g_register_data[TXB].value, bytes);
	bytePointer = (uint8_t *)&response_crc;
	g_register_data[TXB].value[bytes] = bytePointer[1];
	g_register_data[TXB].value[bytes + 1] = bytePointer[0];

	DEBUG_PRINT(LOG_DEBUG, "Read Ready %d\r\n", qry->page);
	nascerr_registers_changeExpStatus(nascerr_registers_getStatusSystem().OP.EXP, REQ_EXP_TX_RESULT);
	LOG_AND_NOTIFY(LOG_SUCCESS, LOG_STATUS_SUCCESS, ERR_NONE);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_sch_signal_loggerTask
 *  @brief     : Sinaliza a loggerTask para executar a rotina de gravacao de logs
 *  @param     : None
 *  @retval    : None
 * ******************************************************************************
 */

void nascerr_sch_signal_loggerTask(void)
{
    osThreadFlagsSet(loggerId, 1u);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_schdb_recording_memory_external
 *  @brief        : Transfere os logs armazenados na memoria interna para a memoria externa.
 *  @param         : None
 *  @retval       : true se a operacao foi concluida com sucesso, false caso contrario.
 * ******************************************************************************
 */

uint8_t nascerr_sch_recording_memory_external(void)
{
	uint8_t *init_adress = nascerr_flash_get_log_address();
	uint16_t occupied_memory = nascerr_flash_get_log_occupied_size();
	uint16_t total = (occupied_memory/sizeof(NASCERR_LOG));
	const NASCERR_LOG *src = (const NASCERR_LOG *)init_adress;
	NASCERR_LOG buffer_write;

	DEBUG_PRINT(LOG_INFO, "Iniciando pagina de log \r\n");
	if (!nascerr_db_beginLogPage()) {
		DEBUG_PRINT(LOG_ERROR, "Erro ao iniciar pagina de logs \r\n");
		return false;
	}else{
		DEBUG_PRINT(LOG_DEBUG, "SUCESSO ao iniciar pagina de logs \r\n");
	}

	for(uint16_t i = 0; i < total; i++){

		buffer_write.timestamp = src[i].timestamp;
		buffer_write.type = (src[i].type >= LOG_SUCCESS && src[i].type <= LOG_BOOT_OPERATION) ? src[i].type : 0;
		buffer_write.status = (src[i].status >= LOG_STATUS_SUCCESS && src[i].status <= LOG_STATUS_ERROR) ? src[i].status : 0;
		buffer_write.error_code = (src[i].error_code >= ERR_NONE && src[i].error_code <= WARN_HW_TEMPERATURE_THRESHOLD) ? src[i].error_code: 0;

#if ENABLE_SERIAL_SYS
		DEBUG_PRINT(LOG_DEBUG,
			"LOG[%u] timestamp=%lu - type=0x%02X - status=%u - erro=%u\r\n",
				i,
				src[i].timestamp,
				src[i].type,
				src[i].status,
				src[i].error_code
			);
#endif
        if (!nascerr_db_addLog(&buffer_write)) {
            DEBUG_PRINT(LOG_ERROR, "Erro ao adicionar log %u na regiao \r\n", i);
            return false;
        }else{
#if ENABLE_SERIAL_SYS
        	DEBUG_PRINT(LOG_INFO, "SUCESSO ao adicionar log %u na regiao \r\n", i);
#endif
        }

        i2c_slave_noBlockBus();
        // osDelay(50);
	}

    if (!nascerr_db_commitLogPage()) {
        DEBUG_PRINT(LOG_ERROR, "Erro ao commitar pagina de logs \r\n");
        return false;
    }else{
#if ENABLE_SERIAL_SYS
        DEBUG_PRINT(LOG_DEBUG, "SUCESSO ao commitar pagina de logs \r\n");
#endif
    }

    if (!nascerr_flash_reset_log_buffer()) {
        DEBUG_PRINT(LOG_ERROR, "Erro ao resetar a memoria interna reservada para os logs \r\n");
        return false;
    }else{
#if ENABLE_SERIAL_SYS
        DEBUG_PRINT(LOG_DEBUG, "SUCESSO ao resetar a memoria interna reservada para os logs \r\n");
#endif

    }

    return true;
}

static void update_system_status()
{
	NASCERR_ExperimentStatus_t stat = nascerr_schdb_getExperimentStatus();
	// Checks wheater they are experiments running or scheduled
	if((stat.qtd_scheduler_total > 0) || ((stat.qtd_experiments_total - stat.qtd_experiments_finished) != 0))
	{
		nascerr_registers_changeExpStatus(nascerr_registers_getStatusSystem().OP.EXP, nascerr_registers_getStatusSystem().REQ.EXP);
	}
	else
	{
		nascerr_registers_changeExpStatus(OP_EXP_IDLE, nascerr_registers_getStatusSystem().REQ.EXP);
	}
}

static void img_rx_notify_cb(void)
{
	osThreadFlagsSet(imgUploadId, FLAG_IMG_DMA_RX_DONE);
}
