/**
 * ******************************************************************************
 *  @file    : NASCERR_UartImage.h
 *  @brief   : Header for NASCERR_UartImage.c file.
 *             Handles image upload via UART using DMA + idle line detection.
 *  @author  :
 * ******************************************************************************
 */

#ifndef INC_NASCERR_UART_IMAGE_H_
#define INC_NASCERR_UART_IMAGE_H_

/*
 * ******************************************************************************
 *  Includes
 * ******************************************************************************
 */

#include "main.h"
#include "uart_handler.h"
#include "NASCERR_System.h"
#include "NASCERR_Util.h"
#include "NASCERR_Registers.h"
/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

#define IMG_FRAME_MAGIC_HI     0xA5u
#define IMG_FRAME_MAGIC_LO     0xC3u
#define IMG_FRAME_MAGIC        0xA5C3u

/* Tamanho máximo de chunk de dados por frame */
#define IMG_CHUNK_DATA_MAX     2112u

/* [ MAGIC:2 | CMD:1 | LEN:2 | PAYLOAD:N | CRC16:2 ] */
#define IMG_FRAME_OVERHEAD     (2u + 1u + 2u + 2u)   /* magic+cmd+len+crc */
#define IMG_DMA_BUF_SIZE       (IMG_CHUNK_DATA_MAX + IMG_FRAME_OVERHEAD + 4u)

/*
 * Ring buffer circular de recepção DMA.
 * Deve ser potência de 2 e caber pelo menos 2 frames CHUNK completos.
 * 8192 ≈ 3.8× o maior frame possível (2123 B).
 */
#define IMG_RING_SIZE          8192u   /* potência de 2 obrigatória */

/* Registrador I2C de feedback ao Master */
#define REG_IMG_STAT_ADDR      0x1Bu
#define REG_IMG_STAT_SIZE      4u

extern uint8_t g_img_dma_buf[IMG_DMA_BUF_SIZE];   /* buffer linear p/ process_frame */
extern uint8_t g_uart_ring[IMG_RING_SIZE];          /* buffer circular DMA             */
/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/* Comandos do protocolo UART de imagem */
typedef enum
{
    IMG_UART_START  = 0x01,   /* Início: informa tamanho total + CRC32 */
    IMG_UART_CHUNK  = 0x02,   /* Chunk: offset(4B) + dados             */
    IMG_UART_FINISH = 0x03,   /* Fim: confirma integridade global       */
    IMG_UART_ABORT  = 0x04,   /* Cancela e limpa a região de imagem     */
} ImgUartCmd_t;

/* Estados do upload — espelhados no registrador IMG_STAT */
typedef enum
{
    IMG_STATE_EMPTY      = 0x00,
    IMG_STATE_UPLOADING  = 0x01,
    IMG_STATE_VALIDATING = 0x02,
    IMG_STATE_READY      = 0x03,
    IMG_STATE_ERROR      = 0x04,
} ImgUploadState_t;

/* Códigos de erro — espelhados no registrador IMG_STAT */
typedef enum
{
    IMG_ERR_NONE         = 0x00,
    IMG_ERR_CRC_FRAME    = 0x01,   /* CRC16 do frame falhou        */
    IMG_ERR_CRC_GLOBAL   = 0x02,   /* CRC32 total não confere      */
    IMG_ERR_FLASH_WRITE  = 0x03,   /* Falha ao gravar na flash     */
    IMG_ERR_SIZE         = 0x04,   /* Tamanho excede o limite      */
    IMG_ERR_BAD_FRAME    = 0x05,   /* Frame malformado             */
    IMG_ERR_TIMEOUT      = 0x06,   /* Timeout entre chunks         */
} ImgErrorCode_t;

/* Conteúdo do registrador IMG_STAT (4 bytes, READ_ONLY via I2C) */
#pragma pack(push, 1)
typedef struct
{
    uint8_t  valid;         /* 0xAA = imagem válida gravada      		*/
    uint8_t  upload_state;  /* ImgUploadState_t                  		*/
    uint8_t  progress_pct;  /* 0-100%                            		*/
    uint32_t image_size;	/* Total size of uploaded or uploading image*/
    uint8_t  error_code;    /* ImgErrorCode_t                    		*/
} ImgStatReg_t;

/* Header de frame UART */
typedef struct
{
    uint16_t magic;         /* IMG_FRAME_MAGIC (big-endian)      */
    uint8_t  cmd;           /* ImgUartCmd_t                      */
    uint16_t payload_len;   /* Tamanho do payload (big-endian)   */
} ImgUartHeader_t;

/* Payload de IMG_UART_START */
typedef struct
{
    uint32_t total_size;    /* Tamanho total da imagem (big-endian) */
    uint16_t crc16;         /* CRC32 esperado (big-endian)          */
} ImgStartPayload_t;

/* Payload de IMG_UART_CHUNK */
typedef struct
{
    uint32_t chunk_id;                    /* Offset na imagem (big-endian)    */
    uint8_t  data[IMG_CHUNK_DATA_MAX];  /* Dados do chunk                   */
} ImgChunkPayload_t;
#pragma pack(pop)

/* Contexto interno do módulo */
typedef struct
{
    uint32_t total_size;       /* Tamanho total declarado no START  */
    uint16_t crc16_expected;   /* CRC32 declarado no START          */
    uint32_t bytes_done;       /* Bytes já gravados na flash        */
    uint16_t rx_size;          /* Bytes recebidos no DMA buf        */
    uint32_t next_expected_id;
} ImgUploadCtx_t;

/*
 * ******************************************************************************
 *  Exported global variables
 * ******************************************************************************
 */

typedef void (*ImgRxNotifyCallback_t)(void);
/*
 * ******************************************************************************
 *  Exported global functions
 * ******************************************************************************
 */
void nascerr_uart_image_register_notify(ImgRxNotifyCallback_t cb);
/* Inicialização — chamada uma vez após boot */
void nascerr_uart_image_init();

/* Chamada pelo HAL_UARTEx_RxEventCallback em Application */
void nascerr_uart_image_rx_event(UART_HandleTypeDef *huart, uint16_t size);

/* Chamada pelo HAL_UART_ErrorCallback quando NE/FE/ORE aborta o DMA */
void nascerr_uart_image_error_event(UART_HandleTypeDef *huart, uint16_t unused);

/* Retorna 1 se o DMA foi abortado por erro e precisa ser reiniciado */
uint8_t nascerr_uart_image_need_restart(void);
uint8_t nascerr_uart_image_need_send_nack(void);

/* Reinicia o DMA circular quando HAL o aborta por erro (chamada pela task) */
void nascerr_uart_image_restart_dma(void);

/* Processa o frame já extraído em g_img_dma_buf — chamada pela task */
SYSTEM_STATUS nascerr_uart_image_process_frame(void);

/* Atualiza o registrador IMG_STAT (feedback ao Master via I2C) */
void nascerr_uart_image_set_stat(ImgUploadState_t state, uint8_t pct, ImgErrorCode_t err);
ImgStatReg_t nascerr_uart_image_get_stat(void);

/* Verifica se há imagem válida gravada */
uint8_t nascerr_uart_image_is_valid(void);


/* Envia ACK/NACK pela UART */
void nascerr_uart_image_send_ack(void);
void nascerr_uart_image_send_nack(void);

#endif /* INC_NASCERR_UART_IMAGE_H_ */
