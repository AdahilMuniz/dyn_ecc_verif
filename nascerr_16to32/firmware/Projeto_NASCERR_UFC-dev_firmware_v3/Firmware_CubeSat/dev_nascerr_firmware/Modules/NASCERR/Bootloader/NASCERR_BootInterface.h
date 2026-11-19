/**
 * ******************************************************************************
 *  @file    : NASCERR_BootInterface.h
 *  @brief   : Header for NASCERR_BootInterface.c file.
 *  @author  : Joao Manoel
 * ******************************************************************************
 */
#ifndef NASCERR_BOOTINTERFACE_H
#define NASCERR_BOOTINTERFACE_H

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include <stdint.h>
#include "uart_handler.h"
#include "NASCERR_Registers.h"
#include "NASCERR_Protocol.h"

/* ------------------------------------------------------------------------- */
/*           Definições de interface de comunicação do bootloader            */
/* ------------------------------------------------------------------------- */
typedef enum {
  BS_IDLE,
  BS_WAIT_PKT,
  BS_DATA_TRANSFER,
  BS_DOWNLOAD_FINISH,
  BS_ERROR,
  BS_DONE
} BootSM_State;

/* ------------------------------------------------------------------------- */
/* Tipos de função de callback                                               */
/* ------------------------------------------------------------------------- */

typedef uint8_t (*nascerr_comm_receive_func_t)(BOOT_DATA_PACKET *packet);

typedef uint8_t (*nascerr_comm_send_func_t)(NASCERR_BOOT_STATUS *packet);

typedef struct {
	nascerr_comm_receive_func_t  receive;		/**< Função de recepção */
	nascerr_comm_send_func_t     send;			/**< Função de envio    */
} nascerr_boot_comm_handler_t;

/* ------------------------------------------------------------------------- */
/* 					Funcoes para interface de comunicacao                    */
/* ------------------------------------------------------------------------- */
void nascerr_boot_comm_init(BootCommInterface_t interface);

uint8_t nascerr_boot_comm_receive(BOOT_DATA_PACKET *packet);

uint8_t nascerr_boot_comm_send(NASCERR_BOOT_STATUS *packet);

#endif /* COMM_INTERFACE_H */
