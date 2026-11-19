/**
 * ******************************************************************************
 *  @file    : flash_saboteur.h
 *  @brief   : Biblioteca para controlar com ocorre a sabotagem dos dados na Flash
 *  @author  : Cicero Oliveira
 *  @note    :
 *
 * ******************************************************************************
 */

#ifndef FLASH_SABOTEUR_H
#define FLASH_SABOTEUR_H

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include <stdint.h>
#include "NASCERR_Config.h"

/*
 * ******************************************************************************
 *  Exported define
 * ******************************************************************************
 */

#define FLASH_RATE_OF_ERROR 0.05

/*
 * ******************************************************************************
 *  Exported type
 * ******************************************************************************
 */

typedef enum
{
	FLASH_OP_NONE,
    FLASH_OP_WRITE,
    FLASH_OP_READ,
    FLASH_OP_ERASE,
    FLASH_OP_FLUSH
} FlashOperationType;

typedef enum
{
	No_Fault_Running,
	Program_Fail,
	Erase_Fail,
	Read_Corruption,
	Timeout_Fail,
}FlashFaultType;

typedef struct {
    uint8_t enabled;
    uint8_t error_injection_attempts;
    float rate_of_error;

    FlashFaultType fault_type_running;
    FlashFaultType last_fault_type;

    FlashOperationType op_in_fault;
    FlashOperationType last_op_in_fault;

    uint32_t program_fail_injections;
    uint32_t erase_fail_injections;
    uint32_t read_corruption_injections;
    uint32_t timeout_fail_injections;

    uint32_t injection_attempt;
} FlashFaultConfig;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

void flash_fault_try_injection(FlashOperationType op); // decides based on probability if it will inject fault

uint8_t flash_fault_sabotage_state(uint8_t state);// forces either program fail, erase fail or forces busy to timeout_fail

void flash_fault_corrupt_read(uint8_t *buffer, uint32_t length); // forces a bit flips

void flash_fault_check_attempt_count(void); // checks if the fault injection should ceise

#endif
