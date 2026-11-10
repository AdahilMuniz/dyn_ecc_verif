/**
 * ******************************************************************************
 *  @file    : flash_saboteur.c
 *  @brief   : Biblioteca para controlar com ocorre a sabotagem dos dados na Flash
 *  @author  : Cicero Oliveira
 *  @note    :
 *
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "flash_saboteur.h"
#include "NASCERR_Util.h"
#include <stdlib.h>

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

volatile FlashFaultConfig faultCfg = {
    .enabled = 0,
    .error_injection_attempts = 0,

#if ENABLE_SABOTAGE_FLASH
    .rate_of_error = FLASH_RATE_OF_ERROR,
#else
    .rate_of_error = 0,
#endif

    .fault_type_running = No_Fault_Running,
	.last_fault_type = No_Fault_Running,

    .op_in_fault = FLASH_OP_NONE,
	.last_op_in_fault = FLASH_OP_NONE,

    .program_fail_injections = 0,
    .erase_fail_injections = 0,
    .read_corruption_injections = 0,
    .timeout_fail_injections = 0,
	.injection_attempt = 0
};

/*
 * ******************************************************************************
 *  Local functions
 * ******************************************************************************
 */

/*
 *** signatures ***
*/

void flash_fault_refresh(void);
void flash_fault_increase_fault_couting(FlashFaultType fault_type_running);

/*
 *** implementation ***
*/

/**
 * ******************************************************************************
 *  Function Name: flash_fault_refresh
 *  @brief       : Reinicializa o estado da injecao de falhas, limpando a falha
 *                 ativa e os contadores associados para a proxima operacao.
 *  @param       : None
 *  @retval      : None
 * ******************************************************************************
 */
void flash_fault_refresh()
{
	faultCfg.enabled = 0;
	faultCfg.fault_type_running = No_Fault_Running;
	faultCfg.error_injection_attempts = 0;
	faultCfg.op_in_fault = FLASH_OP_NONE;

}

/**
 * ******************************************************************************
 *  Function Name: flash_fault_increase_fault_couting
 *  @brief       : Incrementa o contador correspondente ao tipo de falha
 *                 atualmente injetada, para fins de monitoramento e estatística.
 *  @param       : fault_type_running - Tipo de falha atualmente em execução.
 *  @retval      : None
 * ******************************************************************************
 */
void flash_fault_increase_fault_couting(FlashFaultType fault_type_running)
{
	switch (fault_type_running) {
		case Program_Fail:
			faultCfg.program_fail_injections += 1;
			break;
		case Erase_Fail:
			faultCfg.erase_fail_injections += 1;
			break;

		case Read_Corruption:
			faultCfg.read_corruption_injections += 1;
			break;
		case Timeout_Fail:
			faultCfg.timeout_fail_injections += 1;
			break;
		default:
			flash_fault_refresh();
			break;
	}
}


/*
 * ******************************************************************************
 *  Exported global functions
 * ******************************************************************************
 */

/**
 * ******************************************************************************
 *  Function Name: flash_fault_try_injection
 *  @brief       : Probabilistically decides whether fault injection should be
 *                 activated based on rate_of_error. If activated, randomly
 *                 selects a valid fault depending on the flash operation.
 *  @param       : op - Current flash operation being executed
 *  @retval      : None
 * ******************************************************************************
 */
void flash_fault_try_injection(FlashOperationType op)
{
	int r = rand();
    faultCfg.enabled = (r < faultCfg.rate_of_error * RAND_MAX) ? 1 : 0;

    faultCfg.injection_attempt += 1;

    if (!faultCfg.enabled)
    {
    	flash_fault_refresh();
        return;
    }

    FlashFaultType fault_types[3];
    uint8_t num_faults = 0;
    faultCfg.op_in_fault = op;
    faultCfg.last_op_in_fault = op;
    switch(op)
    {
        case FLASH_OP_WRITE:
            fault_types[0] = Program_Fail;
            fault_types[1] = Timeout_Fail;
            num_faults = 2;
            break;

        case FLASH_OP_READ:
            fault_types[0] = Timeout_Fail;
            num_faults = 1;
            break;

        case FLASH_OP_ERASE:
            fault_types[0] = Erase_Fail;
            fault_types[1] = Timeout_Fail;
            num_faults = 2;
            break;

        case FLASH_OP_FLUSH:
            fault_types[0] = Program_Fail;
            fault_types[1] = Timeout_Fail;
            fault_types[2] = Read_Corruption;
            num_faults = 3;
            break;

        default:
        	flash_fault_refresh();
            return;
    }

    faultCfg.fault_type_running = fault_types[rand() % num_faults];
    faultCfg.last_fault_type = faultCfg.fault_type_running;

    flash_fault_increase_fault_couting(faultCfg.fault_type_running);
}

/**
 * ******************************************************************************
 *  Function Name: flash_fault_sabotage_state
 *  @brief       : Forces error bits to be set according to the currently
 *                 selected fault injection type.
 *  @param       : state - Variable containing the value of the flash memory
 *                 status register.
 *  @retval      : Modified status register value with injected fault bits
 *                 when applicable.
 * ******************************************************************************
 */
uint8_t flash_fault_sabotage_state(uint8_t state)
{
	if ((!faultCfg.enabled) || (faultCfg.error_injection_attempts >= 2))
	{
		return state;
	}

	switch (faultCfg.fault_type_running)
	{
		case Timeout_Fail:
		    return state | (1 << 0);

		case Erase_Fail:
			return state | (1 << 2);

	    case Program_Fail:
	        return state | (1 << 3);

	    default:
	    	flash_fault_refresh();
	        return state;
	}
}

/**
 * ******************************************************************************
 *  Function Name: flash_fault_corrupt_read
 *  @brief       : Injects random bit corruption into the provided buffer to
 *                 simulate flash read faults. The corruption may occur in
 *                 different modes (e.g., single-bit, multi-bit, or burst errors)
 *                 according to predefined probabilities.
 *  @param       : buffer - Set of bytes that may be partially corrupted
 *  @param       : length - Size of the buffer in bytes
 *  @retval      : None
 * ******************************************************************************
 */
void flash_fault_corrupt_read(uint8_t *buffer, uint32_t length)
{
    if ((!faultCfg.enabled) ||
		(faultCfg.fault_type_running != Read_Corruption) ||
		(length == 0) ||
		(buffer == NULL))
    {
    	flash_fault_refresh();
    	return;
    }

    int r = rand() % 100;

    if (r < 70)
    {
        uint32_t byte_index = rand() % length;
        uint8_t bit_index = rand() % 8;

        buffer[byte_index] ^= (1 << bit_index);
    }
    else if (r < 95)
    {
        int flips = (rand() % 3) + 2; // 2–4 bit flips

        for (int i = 0; i < flips; i++)
        {
            uint32_t byte_index = rand() % length;
            uint8_t bit_index = rand() % 8;

            buffer[byte_index] ^= (1 << bit_index);
        }
    }
    else
    {
        uint32_t start = rand() % length;
        int burst_size = (rand() % 4) + 2; // 2–5 bytes

        for (int i = 0; i < burst_size; i++)
        {
            uint32_t idx = (start + i) % length;

            uint8_t mask = (1 << (rand() % 8)) |
                           (1 << (rand() % 8));

            buffer[idx] ^= mask;
        }
    }
}

/**
 * ******************************************************************************
 *  Function Name: flash_fault_check_attempt_count
 *  @brief       : Checks whether fault injection can still be attempted in the
 *                 current operation. If the maximum number of attempts is
 *                 exceeded or fault injection is disabled, the fault state
 *                 is reset.
 *  @param       : None
 *  @retval      : None
 * ******************************************************************************
 */
void flash_fault_check_attempt_count(void)
{
	if(!faultCfg.enabled)
	{
		flash_fault_refresh();
		return;
	}
	if(faultCfg.error_injection_attempts >= 2)
	{
		flash_fault_refresh();
		return;
	}
	faultCfg.error_injection_attempts += 1;
}
