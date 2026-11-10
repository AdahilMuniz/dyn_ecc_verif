/**
 * ******************************************************************************
 *  @file    : NASCERR_Protocol.c
 *  @brief   : Manages power state transitions and communication readiness
 *  for the NASCERR system.
 *  @author  : Cicero
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */
#include "NASCERR_System.h"
#include "main.h"
#include "gpio.h"
#include "dma.h"
#include "usart.h"
#include "i2c.h"
#include "spi.h"
#include "fmc.h"
#include "iwdg.h"
#include "i2c_slave.h"

/*
 * ******************************************************************************
 *  Global Variables
 * ******************************************************************************
 */


/*
 * ******************************************************************************
 *  Local Variables
 * ******************************************************************************
 */
extern I2C_HandleTypeDef hi2c1;


/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

void nascerr_system_reinitialization();

/*
 * ******************************************************************************
 *  External Function (to be accessed through other files)
 * ******************************************************************************
 */

extern void SystemClock_Config(void);

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */
/*
 * *******************************************************************************
 *  Function Name: nascerr_system_reinitialization
 *  @brief       : Re-initializes clocks, peripherals and communication interfaces
 *  @param       : none
 *  @retval      : none
 * *******************************************************************************
 */
void nascerr_system_reinitialization()
{
    // 1. Reconfigure the system clock
    SystemClock_Config();

    // 2. Re-initialize all necessary peripherals
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_UART4_Init();
    MX_I2C1_Init();
    MX_SPI3_Init();
    MX_FMC_Init();

    // 4. Resume communication layer
    i2c_slave_init();

#if !SOFTWARE_DEBUG
    MX_IWDG_Init();
#endif
}



