/**
 * ******************************************************************************
 *  @file    : arach_W25Nxx.h
 *  @brief   : Header for arach_W25Nxx.c file.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

#ifndef INC_ARACH_W25Nxx_H_
#define INC_ARACH_W25Nxx_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "main.h"
#include "spi.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "NASCERR_System.h"
#include "NASCERR_SysTime.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

/* W25Nxx Instruction descriptions */
#define W25Nxx_CMD_READSFDP         0x5A      /* Comando para ler SFDP */
#define W25Nxx_CMD_JEDECID          0x9F      /* TODO FICA:Comando de ID JEDEC */
#define W25Nxx_CMD_ID               0x90      /* Comando de ID */
#define W25Nxx_CMD_UNIQUEID         0x4B      /* Comando de ID único */
#define W25Nxx_CMD_WRITEDISABLE     0x04      /* TODO FICA:Comando para desabilitar escrita */
#define W25Nxx_CMD_READSTATUS1      0x05      /* Ler status 1 */
#define W25Nxx_CMD_READSTATUS2      0x35      /* Ler status 2 */
#define W25Nxx_CMD_READSTATUS3      0x15      /* Ler status 3 */
#define W25Nxx_CMD_WRITESTATUSEN    0x50      /* Habilitar escrita de status */
#define W25Nxx_CMD_WRITESTATUS1     0x01      /* Escrever status 1 */
#define W25Nxx_CMD_WRITESTATUS2     0x31      /* Escrever status 2 */
#define W25Nxx_CMD_WRITESTATUS3     0x11      /* Escrever status 3 */
#define W25Nxx_CMD_WRITEENABLE      0x06      /* TODO FICA:Habilitar escrita */
#define W25Nxx_CMD_ADDR4BYTE_EN     0xB7      /* Habilitar endereço de 4 bytes */
#define W25Nxx_CMD_ADDR4BYTE_DIS    0xE9      /* Desabilitar endereço de 4 bytes */
#define W25Nxx_CMD_PAGEPROG3ADD     0x02      /* Programar pagina (3 bytes) */
#define W25Nxx_CMD_PAGEPROG         0x02      /* TODO FICA:Programar pagina (2 bytes) */
#define W25Nxx_CMD_PAGEPROG4ADD     0x12      /* Programar pagina (4 bytes) */
#define W25Nxx_CMD_READDATA3ADD     0x03      /* Ler dados (3 bytes) */
#define W25Nxx_CMD_READDATA4ADD     0x13      /* Ler dados (4 bytes) */
#define W25Nxx_CMD_FASTREAD3ADD     0x0B      /* Leitura rapida (3 bytes) */
#define W25Nxx_CMD_FASTREAD4ADD     0x0C      /* Leitura rapida (4 bytes) *//
#define W25Nxx_CMD_BLOCKERASE3ADD   0xD8      /* Apagar bloco (3 bytes) */
#define W25Nxx_CMD_BLOCKERASE		0xD8      /* TODO FICA:Apagar bloco (2 bytes) */
#define W25Nxx_CMD_BLOCKERASE4ADD   0xDC      /* Apagar bloco (4 bytes) */
#define W25Nxx_CMD_CHIPERASE1       0x60      /* Apagar chip (comando 1) */
#define W25Nxx_CMD_CHIPERASE2       0xC7      /* Apagar chip (comando 2) */
#define W25Nxx_CMD_SUSPEND          0x75      /* Suspender */
#define W25Nxx_CMD_RESUME           0x7A      /* Retomar */
#define W25Nxx_CMD_POWERDOWN        0xB9      /* Desligar energia */
#define W25Nxx_CMD_RELEASE          0xAB      /* Liberar do modo de desligamento */
#define W25Nxx_CMD_FRAMSERNO        0xC3      /* Numero de serie do frame */

#define W25Nxx_DUMMY_BYTE 			0xA5

#define W25Nxx_STATUS1_BUSY 		(1 << 0)
#define W25Nxx_STATUS1_WEL 			(1 << 1)
#define W25Nxx_STATUS1_BP0 			(1 << 2)
#define W25Nxx_STATUS1_BP1 			(1 << 3)
#define W25Nxx_STATUS1_BP2 			(1 << 4)
#define W25Nxx_STATUS1_TP 			(1 << 5)
#define W25Nxx_STATUS1_SEC 			(1 << 6)
#define W25Nxx_STATUS1_SRP0 		(1 << 7)

#define W25Nxx_STATUS2_SRP1 		(1 << 0)
#define W25Nxx_STATUS2_QE 			(1 << 1)
#define W25Nxx_STATUS2_RESERVE1 	(1 << 2)
#define W25Nxx_STATUS2_LB0 			(1 << 3)
#define W25Nxx_STATUS2_LB1 			(1 << 4)
#define W25Nxx_STATUS2_LB2 			(1 << 5)
#define W25Nxx_STATUS2_CMP 			(1 << 6)
#define W25Nxx_STATUS2_SUS 			(1 << 7)

#define W25Nxx_STATUS3_RESERVE1 	(1 << 0)
#define W25Nxx_STATUS3_RESERVE2 	(1 << 1)
#define W25Nxx_STATUS3_WPS 			(1 << 2)
#define W25Nxx_STATUS3_RESERVE3 	(1 << 3)
#define W25Nxx_STATUS3_RESERVE4 	(1 << 4)
#define W25Nxx_STATUS3_DRV0 		(1 << 5)
#define W25Nxx_STATUS3_DRV1 		(1 << 6)
#define W25Nxx_STATUS3_HOLD 		(1 << 7)

/* RTS Config */
#define W25Nxx_RTOS_DISABLE         0
#define W25Nxx_RTOS_CMSIS_V1        1
#define W25Nxx_RTOS_CMSIS_V2        2

/* Config: W25Nxx */
#define W25Nxx_RTOS      		    W25Nxx_RTOS_DISABLE
#define W25Nxx_MFR_ID				0xEF
#define W25Nxx_SIZE				    W25Nxx_SIZE_1GBIT
#define W25Nxx_MEMTYPE				0xAA

/* Config: W25Nxx memory parameters */
#define W25Nxx_FLASH_SIZE           0x8000000 /* 1 Gbit => 128 MBytes        */
#define W25Nxx_BLOCK_SIZE           0x20000   /* 1024 blocks of 128 KBytes   */
#define W25Nxx_PAGE_SIZE            0x800     /* 65536 pages of 2048 Bytes   */

#define W25Nxx_NUM_PAGES     		65536     /* Total number of pages       */
#define W25Nxx_NUM_BLOCKS     		1024      /* Total number of blocks      */
#define W25Nxx_TIME_ERASE_BLOCK 	10        /* Tempo para apagar bloco[ms] */
#define W25Nxx_TIME_WRITE_PAGE   	10        /* Tempo para escrever uma pagina [ms] */


/* Config: ST32 SPI PORT */
#define W25Nxx_SPIHANDLE            &hspi3
#define W25Nxx_CS_PORT				MCU_SPI3_CS_GPIO_Port
#define W25Nxx_CS_PIN				MCU_SPI3_CS_Pin

/* Memory Equation */
#define W25Nxx_PageToBlock(PageNumber)       ((PageNumber * W25Nxx_PAGE_SIZE) / W25Nxx_BLOCK_SIZE)
#define W25Nxx_BlockToPage(BlockNumber)      ((BlockNumber * W25Nxx_BLOCK_SIZE) / W25Nxx_PAGE_SIZE)
#define W25Nxx_PageToAddress(PageNumber)     (PageNumber * W25Nxx_PAGE_SIZE)
#define W25Nxx_BlockToAddress(BlockNumber)   (BlockNumber * W25Nxx_BLOCK_SIZE)
#define W25Nxx_AddressToPage(Address)        (Address / W25Nxx_PAGE_SIZE)
#define W25Nxx_AddressToBlock(Address)       (Address / W25Nxx_BLOCK_SIZE)

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/* W25Nxx memory size type */
typedef enum
{
  W25Nxx_SIZE_ERROR   = 0,
  W25Nxx_SIZE_1MBIT   = 0x11,
  W25Nxx_SIZE_2MBIT   = 0x12,
  W25Nxx_SIZE_4MBIT   = 0x13,
  W25Nxx_SIZE_8MBIT   = 0x14,
  W25Nxx_SIZE_16MBIT  = 0x15,
  W25Nxx_SIZE_32MBIT  = 0x16,
  W25Nxx_SIZE_64MBIT  = 0x17,
  W25Nxx_SIZE_128MBIT = 0x18,
  W25Nxx_SIZE_256MBIT = 0x19,
  W25Nxx_SIZE_512MBIT = 0x20,
  W25Nxx_SIZE_1GBIT   = 0x21,
} W25Nxx_SizeTypeDef;

/* W25Nxx handler type */
typedef struct
{
  SPI_HandleTypeDef      *HSpi;
  GPIO_TypeDef           *Gpio;
  W25Nxx_SizeTypeDef     Size;
  uint8_t                Manufactor;
  uint8_t                Inited;
  uint8_t                MemType;
  uint8_t                Lock;
  uint8_t                Reserved;
  uint32_t               Pin;
  uint32_t               PageCnt;
  uint32_t               BlockCnt;
} W25Nxx_HandleTypeDef;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

SYSTEM_STATUS W25Nxx_Init(void);

SYSTEM_STATUS W25Nxx_EraseBlock(uint32_t Block);

SYSTEM_STATUS W25Nxx_WriteAddress(uint32_t Address, uint8_t *Data, uint32_t Size);
SYSTEM_STATUS W25Nxx_WritePage(uint32_t PageNumber, uint8_t *Data, uint32_t Size, uint32_t Offset);
SYSTEM_STATUS W25Nxx_WriteBlock(uint32_t BlockNumber, uint8_t *Data, uint32_t Size, uint32_t Offset);

SYSTEM_STATUS W25Nxx_ReadAddress(uint32_t Address, uint8_t *Data, uint32_t Size);
SYSTEM_STATUS W25Nxx_ReadPage(uint32_t PageNumber, uint8_t *Data, uint32_t Size, uint32_t Offset);
SYSTEM_STATUS W25Nxx_ReadBlock(uint32_t BlockNumber, uint8_t *Data, uint32_t Size, uint32_t Offset);

#endif /* INC_ARACH_W25Nxx_H_ */
