/**
 * ******************************************************************************
 *  @file    : NASCERR_Tasks.c
 *  @brief   : Tasks FREERTOS NASCERR
 *  @author  : Marcus Anderson; Joao Manoel
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "NASCERR_Tasks.h"

/*
 * ******************************************************************************
 *  Global Variables
 * ******************************************************************************
 */

#define   NUMERO_DE_ERROS_MOCK 32000  //TODO: MOCK DELETE

extern osThreadId_t defaultTaskHandle;

uint8_t *recepcao;

osThreadId_t nascerr_experimentHandle;
osThreadId_t nascerr_senddataHandle;

NASCERR_CTRL crtl_data;
NASCERR_CTRL command_queue[MAX_COMMAND_BUFFER];
int i = 0;

NASCERR_RESPONSE resposta;
NASCERR_RESPONSE response_queue[MAX_COMMAND_BUFFER];

ExpImageCtrl image_command;

ResDescTypeDef result_array[NUMERO_DE_ERROS_MOCK]; //TODO: MOCK DELETE

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
 * *****************************************************************************
 * Function Name: receive_dataResp
 * @brief  :
 * @param  :
 * @retval :
 *******************************************************************************
 */
void receive_dataResp(void *pvParameters)
{
	/* Loop Infinito */
	for(;;){
		if(nascerr_protocol_receivedNewCommand()){
			/* Processando o novo comando (casting) */
			nascerr_protocol_processCommand();
			recepcao = g_register_data[CMD].value;
			/* Verificação quanto ao tamanho do dado */
			if(!nascerr_control_castingCMD(&crtl_data, recepcao)){
				nascerr_registers_changeStatus(OP_FAIL, REQ_NOREQUEST);
			} else{
			/* Mudando Status do sistema */
				nascerr_registers_changeStatus(OP_RECEIVE_COMMAND, REQ_NOREQUEST);
				command_queue[i++] = crtl_data;

				if (i > MAX_COMMAND_BUFFER) i = 0;

				/* Chamando Task de Novo experimento */
				//Ainda não estão implementados os experimentos de imagem
				if (nascerr_experimentHandle == NULL) {
					/* Cria a task se ela ainda não foi criada */
					nascerr_experimentHandle = osThreadNew(nascerr_experiment, NULL, NULL);
				} else {
					/* Se a task já existe, a retoma */
					osThreadResume(nascerr_experimentHandle);
				}
			}
		}

		/* Verificar Barramento I2C */
		i2c_slave_noBlockBus();

//		TODO: MOCK DELETE
//		CompTypeDef compressao;
//		NASCERR_MODE modo;
//		int caso = 10;
//		image_command.image = IMG_3;
//		switch(caso){
//		case 5:
//			compressao = LOSS_FF;
//			modo = TBEC_1_TO_3;
//			break;
//
//		case 6:
//			compressao = LOSSLESS;
//			modo = TBEC_1_TO_3;
//			break;
//		case 7:
//			compressao = LOSS_FF;
//			modo = TBEC_1_TO_7;
//			break;
//
//		case 8:
//			compressao = LOSSLESS;
//			modo = TBEC_1_TO_7;
//			break;
//		case 9:
//			compressao = LOSS_FF;
//			modo = BYPASS_1_TO_7;
//			break;
//
//		case 10:
//			compressao = LOSSLESS;
//			modo = BYPASS_1_TO_7;
//			break;
//
//		default:
//			break;
//		}
//
//		image_command.compression = compressao;
//		crtl_data.mode = modo;


//		image_command.compression = LOSS_FF;
//		image_command.filter = FILTER_0;
//		image_command.image = IMG_0;
//
//
//		crtl_data.bytes_msg = 17;
//		crtl_data.id = 170;
//		crtl_data.cmd_type = WRITE_CMD;
//		crtl_data.data_lenght = 129;
//		crtl_data.repeat = 1;
//		crtl_data.write_type = WRITE_ZEROS;
//		crtl_data.data = BOCK_BY_BOCK;
//		crtl_data.mode = BYPASS_1_TO_7;
//		crtl_data.delay = 25;
//		crtl_data.imagem_exp = image_command;
//		crtl_data.crc16 = 0;
//		if (nascerr_experimentHandle == NULL) {
//			/* Cria a task se ela ainda não foi criada */
//			nascerr_experimentHandle = osThreadNew(nascerr_experiment, NULL, NULL);
//		} else {
//			/* Se a task já existe, a retoma */
//			osThreadResume(nascerr_experimentHandle);
//		}
		osDelay(100);
	}
}

/*
 * *****************************************************************************
 * Function Name: send_dataResp
 * @brief  :
 * @param  :
 * @retval :
 *******************************************************************************
 */
void send_dataResp(void *pvParameters){
	for(;;){
		/* Processando resposta */

		nascerr_protocol_requestTxResult_byPackages(resposta, ERROR_BY_PACKAGE, 100000);
		/* Mudando Status do sistema */
		//nascerr_registers_changeStatus(OP_FINISH, REQ_TXRESULT);
		osThreadSuspend(osThreadGetId());
	}
}

/*
 * *****************************************************************************
 * Function Name: nascerr_experiment
 * @brief  :
 * @param  :
 * @retval :
 *******************************************************************************
 */
void nascerr_experiment(void *pvParameters){
	for(;;){
		/* Executando novo experimento */
//		if(crtl_data.cmd_type == EXPIMAGE_CMD){
//			resposta = nascerr_run_experiment(crtl_data);
//		} else{
//			resposta = nascerr_experiment_test(crtl_data);
//		}
//		resposta.cmd_type = crtl_data.cmd_type;
//		TODO: MOCK DELETE
		nascerr_registers_changeStatus(OP_EXPERIMENT, REQ_NOREQUEST);
		resposta.id = crtl_data.id;
		resposta.data_lenght = crtl_data.data_lenght;
		resposta.number_errors = crtl_data.data_lenght;
		resposta.corrected_data = crtl_data.data_lenght;
		resposta.percent_error = (uint8_t)10;
		for (uint32_t i = 0; i < resposta.number_errors; i++) {
			result_array[i].addr = 0xAAAA;  // Endereço de erro simulado
			result_array[i].result = (i % 2 == 0) ? 0xAAAAAAAA : 0x55555555;  // Erro alternado entre 1 (erro) e 0 (sem erro)
			osDelay(crtl_data.delay);
		}
		resposta.result = result_array;
		resposta.cmd_type = WRITE_CMD;

		/* Chamando Task de Novo experimento */
        if (nascerr_senddataHandle == NULL) {
            /* Cria a task se ela ainda não foi criada */
        	nascerr_senddataHandle = osThreadNew(send_dataResp, NULL, NULL);
        } else {
            /* Se a task já existe, a retoma */
            osThreadResume(nascerr_senddataHandle);
        }

		osThreadSuspend(osThreadGetId());
	}
}
