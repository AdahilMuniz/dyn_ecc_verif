/**
 * ******************************************************************************
 *  @file    : i2c_slave.h
 *  @brief   : Header for i2c_slave.c file.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

#ifndef INC_I2C_SLAVE_H_
#define INC_I2C_SLAVE_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "NASCERR_Protocol.h"
#include "NASCERR_System.h"
#include "main.h"

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

SYSTEM_STATUS i2c_slave_init(void);
void i2c_slave_reset(void);
SYSTEM_STATUS i2c_slave_receive(uint8_t *pData, uint16_t Size);
SYSTEM_STATUS i2c_slave_send(uint8_t *pData, uint16_t Size);
void i2c_slave_noBlockBus(void);

#endif /* INC_I2C_SLAVE_H_ */
