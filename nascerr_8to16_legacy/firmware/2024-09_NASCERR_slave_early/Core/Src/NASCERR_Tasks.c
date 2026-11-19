#include <NASCERR_Tasks.h>
#include "NASCERR_Protocol.h"
#include "NASCERR_Registers.h"
#include "NASCERR_Control.h"
#include "NASCERR_Experiment.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>

extern osThreadId_t defaultTaskHandle;
extern I2C_HandleTypeDef hi2c2;
uint8_t *recepcao;
NASCERR_CTRL crtl_data;

void recive_dataResp(void *pvParameters){
	osThreadId_t nascerr_experimentHandle;
	for(;;){
		if(nascerr_protocol_receivedNewCommand()){
			recepcao = g_register_data[CMD].value;
			nascerr_casting(&crtl_data, recepcao);
			nascerr_experimentHandle = osThreadNew(nascerr_experiment, NULL, NULL);
			osThreadSuspend(osThreadGetId());
		  }
		osDelay(100);
	}
}

void send_dataResp(void *pvParameters){
	for(;;){

	}
}

void nascerr_experiment(void *pvParameters){
	for(;;){
		NASCERR_RESPONSE resposta;
		resposta = nascerr_experiment_test(crtl_data);

	}
}
