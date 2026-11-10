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
#include "NASCERR_Control.h"
#include "main.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */
/* Register length */
#define REG_MAX_DATA_LENGHT 32768

/* Register addresses */
#define REG_NONE_ADDR         0x00  /* > No register selected                   		*/
#define REG_STAT_ADDR         0x01  /* > Operation status register              		*/
#define REG_IDX_ADDR          0x02  /* > Number of the data block to transmit   		*/
#define REG_TXB_ADDR          0x03  /* > Transmission buffer register           		*/
#define REG_CMD_ADDR          0x04  /* > Command buffer register                		*/
#define REG_MODO_ADDR         0x05  /* > Operation mode register                		*/
#define REG_SLEEP_ADDR        0x06  /* > SLEEP mode register                    		*/
#define REG_WAKEUP_ADDR       0x07  /* > WAKEUP mode register                   		*/
#define REG_GPS_ADDR          0x08  /* > GPS data register                      		*/
#define REG_ACK_ADDR		  0x0D  /* > Command buffer register                		*/
#define REG_HW_STAT_ADDR	  0x0E  /* > Hardware status register						*/
#define REG_HW_TXB_ADDR	  	  0x0F  /* > Hardware status transmission buffer register	*/
#define REG_DATA_BOOT_ADDR	  0x10  /* > Data boot register							    */
#define REG_STAT_BOOT_ADDR	  0x11  /* > Status boot register							*/
#define REG_LOG_ADDR     	  0x12  /* > Operation Log                    				*/
#define REG_EXP_STAT_ADDR     0x13  /* > Struct NASCERR_ExperimentStatus_t              */
#define REG_EXP_SUM_IDX_ADDR  0x14  /* > Page/offset selector for unread-summary page   */
#define REG_EXP_SUM_DATA_ADDR 0x15  /* > Unread experiments summary page                */
#define REG_DB_CLEAR_ADDR     0x16  /* > Database-erase command register                */

/* Register sizes */
#define REG_NONE_SIZE         0     					/* > No register selected                      		*/
#define REG_STAT_SIZE         11 						/* > Operation status register size            		*/
#define REG_IDX_SIZE          4  						/* > Number of the data block to transmit size 		*/
#define REG_TXB_SIZE       	  2066			        	/* > Transmission buffer register size				*/
#define REG_CMD_SIZE          31  						/* > Command buffer register size              		*/
#define REG_SLEEP_SIZE        3  						/* > SLEEP mode register size                  		*/
#define REG_WAKEUP_SIZE       3  						/* > WAKEUP mode register size                 		*/
#define REG_GPS_SIZE          36  						/* > GPS data register size                    		*/
#define REG_LOG_SIZE     	  128  						/* > Operation Log size                  	   		*/
#define REG_ACK_SIZE		  4							/* > Acknowledgment register size              		*/
#define REG_HW_STAT_SIZE	  3							/* > Hardware Status register size			   		*/
#define REG_HW_TXB_SIZE	  	  22						/* > Hardware Status register size			   		*/
#define REG_DATA_BOOT_SIZE	  263						/* > Data boot register size 						*/
#define REG_STAT_BOOT_SIZE	  15						/* > Status boot register size				   		*/
#define REG_EXP_STAT_SIZE     18   						/* > Struct NASCERR_ExperimentStatus_t size    		*/
#define REG_EXP_SUM_IDX_SIZE  3     					/* > Page/offset selector for unread-summary page 	*/
#define REG_EXP_SUM_DATA_SIZE 216   					/* > Unread experiments summary page size       	*/
#define REG_DB_CLEAR_SIZE     3     					/* > Database-erase command register size       	*/

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/* Protocol command registers */
typedef enum
{
    NONE         = REG_NONE_ADDR,        /* > No register selected                    */
    STAT         = REG_STAT_ADDR,        /* > Operation status register               */
    IDX          = REG_IDX_ADDR,         /* > Number of the data block to transmit    */
    TXB          = REG_TXB_ADDR,         /* > Transmission buffer register            */
    CMD          = REG_CMD_ADDR,         /* > Command buffer register                 */
    SLEEP        = REG_SLEEP_ADDR,       /* > SLEEP mode register                     */
    WAKEUP       = REG_WAKEUP_ADDR,      /* > WAKEUP mode register                    */
    GPS          = REG_GPS_ADDR,         /* > GPS data register                       */
    LOG          = REG_LOG_ADDR,         /* > Operation Log                           */
    ACK          = REG_ACK_ADDR,         /* > Acknowledgment register                 */
    HW_STAT      = REG_HW_STAT_ADDR,     /* > Hardware status register                */
    HW_TXB       = REG_HW_TXB_ADDR,      /* > Hardware status transmission buffer     */
    BOOT_DATA    = REG_DATA_BOOT_ADDR,   /* > Data boot register                      */
    BOOT_STAT    = REG_STAT_BOOT_ADDR,   /* > Status boot register                    */
    EXP_STAT     = REG_EXP_STAT_ADDR,    /* > Struct NASCERR_ExperimentStatus_t       */
    EXP_SUM_IDX  = REG_EXP_SUM_IDX_ADDR, /* > Page/offset selector for unread-summary */
    EXP_SUM_DATA = REG_EXP_SUM_DATA_ADDR,/* > Unread experiments summary page         */
    DB_CLEAR     = REG_DB_CLEAR_ADDR     /* > Database-erase command register         */
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

/* Exportando registrador geral */
extern volatile RegStructTypeDef g_register_data[];

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

//NASCERR_STATUS nascerr_registers_changeExpExecutingStatus(uint32_t current_id, uint8_t current_block);
//NASCERR_STATUS nascerr_registers_changeExpStatus(OP_EXPERIMENT op, REQ_EXPERIMENT req);
//NASCERR_STATUS nascerr_registers_changeHwStatus(OP_HARDWARE op, REQ_HARDWARE req);

#endif /* INC_NASCERR_REGISTERS_H_ */
