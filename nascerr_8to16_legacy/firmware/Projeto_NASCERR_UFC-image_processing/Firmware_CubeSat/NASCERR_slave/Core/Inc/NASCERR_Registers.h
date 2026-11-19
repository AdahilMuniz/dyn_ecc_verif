/**
 * ******************************************************************************
 *  @file    : NASCERR_Registers.h
 *  @brief   : Header for NASCERR_Registers.c file.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

#ifndef INC_NASCERR_REGISTERS_H_
#define INC_NASCERR_REGISTERS_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "NASCERR_System.h"
#include "main.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */
/* Register length */
#define REG_MAX_DATA_LENGHT 1024

/* Register addresses */
#define REG_NONE_ADDR         0x00  /* > No register selected                   */
#define REG_STAT_ADDR         0x01  /* > Operation status register              */
#define REG_IDX_ADDR          0x02  /* > Number of the data block to transmit   */
#define REG_TXB_ADDR          0x03  /* > Transmission buffer register           */
#define REG_CMD_ADDR          0x04  /* > Command buffer register                */
#define REG_MODO_ADDR         0x05  /* > Operation mode register                */
#define REG_SLEEP_ADDR        0x06  /* > SLEEP mode register                    */
#define REG_WAKEUP_ADDR       0x07  /* > WAKEUP mode register                   */
#define REG_GPS_ADDR          0x08  /* > GPS data register                      */
#define REG_TIME_ADDR         0x09  /* > TIMESTAMP data register                */
#define REG_TEMP_ADDR         0x0A  /* > TEMP data register                     */
#define REG_ERROR_OP_ADDR     0x0B  /* > Operation Error Log                    */
#define REG_ERROR_EPS_ADDR    0x0C  /* > Power Supply Error Log                 */

/* Register sizes */
#define REG_NONE_SIZE         0     /* > No register selected                      */
#define REG_STAT_SIZE         2  	/* > Operation status register size            */
#define REG_IDX_SIZE          2  	/* > Number of the data block to transmit size */
#define REG_TXB_SIZE       	  (8 * REG_MAX_DATA_LENGHT)  /* > Transmission buffer register size         */
#define REG_CMD_SIZE          128  /* > Command buffer register size              */
#define REG_MODO_SIZE         1  	/* > Operation mode register size              */
#define REG_SLEEP_SIZE        1  	/* > SLEEP mode register size                  */
#define REG_WAKEUP_SIZE       1  	/* > WAKEUP mode register size                 */
#define REG_GPS_SIZE          28  	/* > GPS data register size                    */
#define REG_TIME_SIZE         6  	/* > TIMESTAMP data register size              */
#define REG_TEMP_SIZE         2  	/* > TEMP data register size                   */
#define REG_ERROR_OP_SIZE     2  	/* > Operation Error Log size                  */
#define REG_ERROR_EPS_SIZE    2  	/* > Power Supply Error Log size               */


/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/* Protocol command registers */
typedef enum
{
    NONE = 0,
    STAT,                   		/* > Operation status register               */
    IDX,                    		/* > Number of the data block to transmit    */
    TXB,                    		/* > Transmission buffer register            */
    CMD,                    		/* > Command buffer register                 */
    MODO,                   		/* > Operation mode register                 */
    SLEEP,                  		/* > SLEEP mode register                     */
    WAKEUP,                 		/* > WAKEUP mode register                    */
    GPS,                    		/* > GPS data register                       */
    TIME,                   		/* > TIMESTAMP data register                 */
    TEMP,                   		/* > TEMP data register                      */
    ERROR_OP,               		/* > Operation Error Log                     */
    ERROR_EPS               		/* > Power Supply Error Log                  */
} RegistersTypeDef;

/* Access to registers */
typedef enum
{
    WRITE_ONLY = 0,         		/* > Write-only access                       */
    READ_ONLY,         				/* > Read-only access                        */
    FULL_ACCESS         			/* > Full read and write access              */
} RegModeTypeDef;

/* Register structure */
typedef struct
{
    uint8_t      access;     		/* > Access mode of the register             */
    uint8_t      reg_addr;   		/* > Address of the register                 */
    uint16_t     size; 		        /* > Type of value stored in the register    */
    uint8_t      *value;      		/* > Current value of the register           */
} RegStructTypeDef;


extern volatile RegStructTypeDef g_register_data[];

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

NASCERR_STATUS nascerr_registers_changeStatus(OP_LOGS new_operation, REQ_LOGS new_request);
//void nascerr_resiters_getValueReg(RegistersTypeDef nascerr_register, uint8_t* buff_read);

#endif /* INC_NASCERR_REGISTERS_H_ */
