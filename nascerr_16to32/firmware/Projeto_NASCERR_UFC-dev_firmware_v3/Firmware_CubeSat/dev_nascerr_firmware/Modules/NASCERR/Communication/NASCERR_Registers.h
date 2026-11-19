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
#define REG_NONE_ADDR          0x00  /* > No register selected                   		*/
#define REG_STAT_ADDR          0x01  /* > Operation status register              		*/
#define REG_IDX_ADDR           0x02  /* > Number of the data block to transmit   		*/
#define REG_TXB_ADDR           0x03  /* > Transmission buffer register           		*/
#define REG_CMD_ADDR           0x04  /* > Command buffer register                		*/
#define REG_ACK_ADDR           0x05  /* > Operation mode register                		*/
#define REG_SLEEP_ADDR         0x06  /* > SLEEP mode register                    		*/
#define REG_WAKEUP_ADDR        0x07  /* > WAKEUP mode register                   		*/
#define REG_GPS_ADDR           0x08  /* > GPS data register                      		*/
#define REG_MODO_ADDR		   0x0D  /* > Command buffer register                		*/
#define REG_HW_STAT_ADDR	   0x0E  /* > Hardware status register						*/
#define REG_HW_TXB_ADDR	  	   0x0F  /* > Hardware status transmission buffer register	*/
#define REG_DATA_BOOT_ADDR	   0x10  /* > Data boot register							*/
#define REG_STAT_BOOT_ADDR	   0x11  /* > Status boot register							*/
#define REG_EXP_STAT_ADDR      0x13  /* > Struct NASCERR_ExperimentStatus_t             */
#define REG_EXP_SUM_IDX_ADDR   0x14  /* > Page/offset selector for unread-summary page  */
#define REG_EXP_SUM_DATA_ADDR  0x15  /* > Unread experiments summary page               */
#define REG_DB_CLEAR_ADDR      0x16  /* > Database-erase command register               */
#define REG_LOG_SUM_ADDR	   0x17  /*  > Sumário da área de logs						*/
#define REG_LOG_SET_PAGE_ADDR  0x18  /*  > Página de logs a ler							*/
#define REG_LOG_READ_PAGE_ADDR 0x19  /*  > Conteúdo da página selecionada				*/
#define REG_ROBUST_MEM_HEALTH_ADDR   0X1A  /*  > Contains info about corrupted blocks on SRAM */
#define REG_IMAGE_STAT_ADDR	   0x1B  /* > Image stat register							*/

/* Register sizes */
#define REG_NONE_SIZE         0     					/* > No register selected                      		*/
#define REG_STAT_SIZE         7 						/* > Operation status register size            		*/
#define REG_IDX_SIZE          4  						/* > Number of the data block to transmit size 		*/
#define REG_TXB_SIZE       	  2066			        	/* > Transmission buffer register size				*/
#define REG_CMD_SIZE          31  						/* > Command buffer register size              		*/
#define REG_SLEEP_SIZE        3  						/* > SLEEP mode register size                  		*/
#define REG_WAKEUP_SIZE       3  						/* > WAKEUP mode register size                 		*/
#define REG_GPS_SIZE          36  						/* > GPS data register size                    		*/
#define REG_LOG_SIZE     	  128  						/* > Operation Log size                  	   		*/
#define REG_ACK_SIZE		  3							/* > Acknowledgment register size              		*/
#define REG_HW_STAT_SIZE	  3							/* > Hardware Status register size			   		*/
#define REG_HW_TXB_SIZE	  	  22						/* > Hardware Status register size			   		*/
#define REG_DATA_BOOT_SIZE	  263						/* > Data boot register size 						*/
#define REG_STAT_BOOT_SIZE	  15						/* > Status boot register size				   		*/
#define REG_EXP_STAT_SIZE     18   						/* > Struct NASCERR_ExperimentStatus_t size    		*/
#define REG_EXP_SUM_IDX_SIZE  3     					/* > Page/offset selector for unread-summary page 	*/
#define REG_EXP_SUM_DATA_SIZE 216   					/* > Unread experiments summary page size       	*/
#define REG_DB_CLEAR_SIZE     3     					/* > Database-erase command register size       	*/
#define REG_LOG_SUM_SIZE	   16  						/* > 13 bytes payload + 2 bytes CRC					*/
#define REG_LOG_SET_PAGE_SIZE  3  						/* > 1 byte page + 2 bytes CRC						*/
#define REG_LOG_READ_PAGE_SIZE 2109						/* > ( 301 x 7 ) + Bytes de CRC						*/
#define REG_ROBUST_MEM_HEALTH_SIZE   34					/* > ROBUST MEMORY HEALTH register size			    */

#define REG_IMAGE_STAT_SIZE	   10						/* > Image Stat register size						*/

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/* Protocol command registers */
typedef enum
{
    NONE         = REG_NONE_ADDR,        	/* > No register selected                    */
    STAT         = REG_STAT_ADDR,        	/* > Operation status register               */
    IDX          = REG_IDX_ADDR,         	/* > Number of the data block to transmit    */
    TXB          = REG_TXB_ADDR,         	/* > Transmission buffer register            */
    CMD          = REG_CMD_ADDR,         	/* > Command buffer register                 */
    SLEEP        = REG_SLEEP_ADDR,       	/* > SLEEP mode register                     */
    WAKEUP       = REG_WAKEUP_ADDR,      	/* > WAKEUP mode register                    */
    GPS          = REG_GPS_ADDR,         	/* > GPS data register                       */
    ACK          = REG_ACK_ADDR,         	/* > Acknowledgment register                 */
    HW_STAT      = REG_HW_STAT_ADDR,     	/* > Hardware status register                */
    HW_TXB       = REG_HW_TXB_ADDR,      	/* > Hardware status transmission buffer     */
    BOOT_DATA    = REG_DATA_BOOT_ADDR,   	/* > Data boot register                      */
    BOOT_STAT    = REG_STAT_BOOT_ADDR,   	/* > Status boot register                    */
    EXP_STAT     = REG_EXP_STAT_ADDR,    	/* > Struct NASCERR_ExperimentStatus_t       */
    EXP_SUM_IDX  = REG_EXP_SUM_IDX_ADDR, 	/* > Page/offset selector for unread-summary */
    EXP_SUM_DATA = REG_EXP_SUM_DATA_ADDR,	/* > Unread experiments summary page         */
    DB_CLEAR     = REG_DB_CLEAR_ADDR,     	/* > Database-erase command register         */
	LOG_SUM 	  = REG_LOG_SUM_ADDR,	   	/* > Sumário da memória de logs				 */
	LOG_SET_PAGE  = REG_LOG_SET_PAGE_ADDR, 	/* > Setar página a ler						 */
	LOG_READ_PAGE = REG_LOG_READ_PAGE_ADDR, /* > Dados da página selecionada			 */
	ROBUST_MEM_HEALTH   = REG_ROBUST_MEM_HEALTH_ADDR,    /* > Dados dos blocos corrompidos			 */
	IMAGE_STAT    = REG_IMAGE_STAT_ADDR 	/* > Image status							 */
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

NASCERR_STATUS nascerr_registers_getStatusSystem(void);
NASCERR_STATUS nascerr_registers_changeExpStatus(OP_EXPERIMENT op, REQ_EXPERIMENT req);
NASCERR_STATUS nascerr_registers_changeHwStatus(OP_HARDWARE op, REQ_HARDWARE req);
NASCERR_STATUS nascerr_registers_changeHwFailStatus(uint8_t fail);
NASCERR_STATUS nascerr_register_changeQueueStatus(uint8_t queue_size);

void nascerr_register_changeAckPhase(ACK_PHASE phase);


#endif /* INC_NASCERR_REGISTERS_H_ */
