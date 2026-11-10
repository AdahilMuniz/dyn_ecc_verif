/**
 * ******************************************************************************
 *  @file    : NASCERR_UartImage.c
 *  @brief   : Image upload via UART — communication layer.
 *             Responsável por: recepção DMA, validação de frame,
 *             gravação na flash e atualização do registrador IMG_STAT.
 *  @author  :
 * ******************************************************************************
 */

#include "NASCERR_UartImage.h"
#include "NASCERR_DB.h"
#include "NASCERR_Flash.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

ImgUploadCtx_t g_img_ctx = {0};
static ImgRxNotifyCallback_t s_notify_cb     = NULL;
static UART_HandleTypeDef   *s_huart         = NULL;
static volatile uint8_t      s_need_restart  = 0u;  /* flag: erro abortou DMA */
static volatile uint8_t		 s_send_nack	 = 0u;

/* Buffer linear — destino da varredura do ring, entrada de process_frame() */
uint8_t g_img_dma_buf[IMG_DMA_BUF_SIZE];

/* ── Ring buffer circular DMA ──────────────────────────────────────────────
 * O DMA escreve em modo CIRCULAR neste buffer sem nunca parar.
 * O software lê a posição de escrita diretamente do registrador CNDTR do DMA.
 * IMG_RING_SIZE deve ser potência de 2 (para uso eficiente de AND bit a bit).
 */
uint8_t          g_uart_ring[IMG_RING_SIZE];     /* buffer DMA circular        */
static uint32_t  s_ring_rd = 0u;                 /* ponteiro de leitura (SW)   */

static ImgStatReg_t s_img_stat = {0};

extern UART_HandleTypeDef huart4;

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

/* ── Utilitários do ring buffer ─────────────────────────────────────────── */

/* Posição atual de escrita do DMA (lida do registrador de hardware CNDTR).
 * O DMA decrementa CNDTR de IMG_RING_SIZE → 0 e volta ao início.
 * write_pos = IMG_RING_SIZE - CNDTR  →  intervalo [0, IMG_RING_SIZE). */
static inline uint32_t ring_wr(void)
{
    return (IMG_RING_SIZE - (uint32_t)__HAL_DMA_GET_COUNTER(s_huart->hdmarx))
           & (IMG_RING_SIZE - 1u);
}

/* Bytes disponíveis para leitura (write_pos - read_pos, com wrap). */
static inline uint32_t ring_avail(void)
{
    return (ring_wr() - s_ring_rd + IMG_RING_SIZE) & (IMG_RING_SIZE - 1u);
}

/* Lê um byte do ring sem avançar o ponteiro (offset relativo a s_ring_rd). */
static inline uint8_t ring_peek(uint32_t off)
{
    return g_uart_ring[(s_ring_rd + off) & (IMG_RING_SIZE - 1u)];
}

/* Avança o ponteiro de leitura em n bytes (consome do ring). */
static inline void ring_advance(uint32_t n)
{
    s_ring_rd = (s_ring_rd + n) & (IMG_RING_SIZE - 1u);
}

/* ── CRC de frame ───────────────────────────────────────────────────────── */

static uint8_t img_frame_crc_valid(const uint8_t *buf, uint16_t total_len)
{
    /* CRC16 cobre tudo exceto os 2 bytes finais de CRC */
    uint16_t crc_recv = ((uint16_t)buf[total_len - 2u] << 8u)
                       |  buf[total_len - 1u];
    return verify_crc(buf, total_len - 2u, crc_recv);
}

uint32_t nascerr_uart_image_size(void)
{
	return experimentHeader.image_data.image_size;
}

/* Em nascerr_uart_image_send_ack_finish — byte distinto para FINISH */
void nascerr_uart_image_send_ack_finish(void)
{
    uint8_t ack = 0x07u;
    HAL_UART_Transmit(s_huart, &ack, 1u, 100u);
}


/*
 * ******************************************************************************
 *  Exported Functions
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: nascerr_uart_image_init
 *  @brief     : Inicializa o módulo — registra o handle UART, restaura
 *               o estado a partir do header da flash e ativa o DMA.
 *  @param     : huart – handle da UART utilizada
 *  @retval    : None
 * ******************************************************************************
 */
void nascerr_uart_image_init()
{
    s_huart   = &huart4;
    s_ring_rd = 0u;

    if (nascerr_uart_image_is_valid())
    {
    	g_img_ctx.total_size = nascerr_uart_image_size();
        nascerr_uart_image_set_stat(IMG_STATE_READY, 100u, IMG_ERR_NONE);
    }
    else{
        nascerr_uart_image_set_stat(IMG_STATE_EMPTY, 0u, IMG_ERR_NONE);
    }

    /* Inicia recepção contínua no ring buffer (DMA deve estar em modo CIRCULAR no IOC) */
    UART_ImgDMA_Init(s_huart, g_uart_ring, IMG_RING_SIZE);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_uart_image_rx_event
 *  @brief     : Chamada pelo HAL_UARTEx_RxEventCallback (em Application).
 *               Verifica o magic e sinaliza a task se for frame de imagem.
 *  @param     : huart – handle da UART
 *               size  – bytes recebidos pelo DMA
 *  @retval    : None
 * ******************************************************************************
 */
void nascerr_uart_image_rx_event(UART_HandleTypeDef *huart, uint16_t size)
{
    (void)size;
    if (huart->Instance != s_huart->Instance) return;

    /* ── 1. Sincronizar: avança até encontrar magic ── */
    uint32_t avail = ring_avail();

    while (avail >= 2u) {
        if (ring_peek(0u) == IMG_FRAME_MAGIC_HI &&
            ring_peek(1u) == IMG_FRAME_MAGIC_LO)
            break;
        ring_advance(1u);  /* descarta byte não-magic */
        avail--;
    }

    /* ── 2. Header incompleto — aguarda próximo IDLE ── */
    if (avail < (uint32_t)sizeof(ImgUartHeader_t)){
    	nascerr_uart_image_send_nack();
        return;
    }

    /* ── 3. Lê payload_len (big-endian, offset 3-4 a partir do magic) ── */
    uint16_t payload_len = ((uint16_t)ring_peek(3u) << 8u) | ring_peek(4u);
    uint16_t frame_total = (uint16_t)sizeof(ImgUartHeader_t) + payload_len + 2u;

    /* Sanidade: payload maior que o buffer → provável lixo, re-sincroniza */
    if (frame_total > (uint16_t)IMG_DMA_BUF_SIZE) {
        ring_advance(1u);
        nascerr_uart_image_send_nack();
        return;
    }

    /* ── 4. Frame ainda incompleto no ring — aguarda próximo IDLE ── */
    if (avail < (uint32_t)frame_total){
    	nascerr_uart_image_send_nack();
        return;
    }

    /* ── 5. Copia frame do ring para buffer linear (sem consumir ainda) ── */
    for (uint32_t i = 0u; i < frame_total; i++)
        g_img_dma_buf[i] = ring_peek(i);

    /* ── 6. Valida CRC ── */
    if (!img_frame_crc_valid(g_img_dma_buf, frame_total)) {
        ring_advance(1u);  /* CRC inválido: pula magic byte e tenta re-sincronizar */
        DEBUG_PRINT(LOG_ERROR, "[UART] CRC de frame invalido (frame_total=%u)\r\n", frame_total);
        nascerr_uart_image_send_nack();
        return;
    }

    /* ── 7. Frame válido: consome do ring e sinaliza a task ── */
    ring_advance(frame_total);
    g_img_ctx.rx_size = frame_total;

    if (s_notify_cb != NULL)
        s_notify_cb();
}

/*
 * Chamada pelo HAL quando NE/FE/ORE aborta o DMA circular.
 * NÃO chama UART_ImgDMA_Init aqui para evitar o loop recursivo:
 *   HAL_DMA_Abort_IT → UART_DMAAbortOnError → HAL_UART_ErrorCallback (loop).
 * Seta flag e sinaliza a task; a task chama restart_dma() de contexto seguro.
 */
void nascerr_uart_image_error_event(UART_HandleTypeDef *huart, uint16_t unused)
{
	DEBUG_PRINT(LOG_ERROR, "[UART] Erro de hardware DMA UART (FE/NE/ORE): ErrorCode=0x%02lX t=%lu\r\n",
	            huart->ErrorCode, HAL_GetTick());
    (void)unused;
    if (huart->Instance != s_huart->Instance) return;

    /* Limpa flags de erro */
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_IDLEF
                                | UART_CLEAR_NEF  | UART_CLEAR_FEF);

    /* Reseta estado do HAL/DMA diretamente — sem passar por Abort_IT */
    huart->RxState       = HAL_UART_STATE_READY;
    if (huart->hdmarx) {
        huart->hdmarx->State = HAL_DMA_STATE_READY;
        huart->hdmarx->Lock  = HAL_UNLOCKED;
    }

    /* Rearma imediatamente aqui mesmo, sem custo de context switch.
     * s_ring_rd permanece onde está — não descartamos dados já recebidos.
     * O ponteiro de escrita do DMA reinicia em 0, então ajustamos s_ring_rd. */
    s_ring_rd = 0u;
    UART_ImgDMA_Init(s_huart, g_uart_ring, IMG_RING_SIZE);

    /* Sinaliza task apenas se já havia um frame pendente para processar */
    if (s_need_restart) {
        /* já sinalizado, não duplica */
        return;
    }
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_uart_image_restart_dma
 *  @brief     : Reativa o DMA após processamento do frame.
 *               Chamada pela task ao final de cada ciclo.
 *  @retval    : None
 * ******************************************************************************
 */
/*
 * Reinicia o DMA circular após um erro de hardware (FE/NE/ORE).
 * Chamada pela task em contexto seguro — evita o loop recursivo do HAL.
 * Reposiciona s_ring_rd na posição de escrita atual para descartar dados
 * corrompidos que possam ter chegado durante o período sem DMA.
 */
void nascerr_uart_image_restart_dma(void)
{
    if (s_huart == NULL || s_huart->Instance == NULL) return;

    s_huart->ErrorCode = HAL_UART_ERROR_NONE;
    __HAL_UART_CLEAR_FLAG(s_huart, UART_CLEAR_OREF | UART_CLEAR_IDLEF
                                 | UART_CLEAR_NEF  | UART_CLEAR_FEF);
    s_huart->hdmarx->State = HAL_DMA_STATE_READY;
    s_huart->hdmarx->Lock  = HAL_UNLOCKED;
    s_huart->RxState       = HAL_UART_STATE_READY;
    s_need_restart = 0u;

    s_ring_rd = ring_wr();  /* descarta só o que ficou corrompido */
    UART_ImgDMA_Init(s_huart, g_uart_ring, IMG_RING_SIZE);
}
/*
 * ******************************************************************************
 *  Function Name: nascerr_uart_image_process_frame
 *  @brief     : Processa o frame recebido no buffer DMA.
 *               Chamada pela imgUploadTask após acordar.
 *  @retval    : STATUS_SUCCESS ou ERR_COMM
 * ******************************************************************************
 */
SYSTEM_STATUS nascerr_uart_image_process_frame(void)
{
    uint8_t *buf = g_img_dma_buf;
    uint16_t size = g_img_ctx.rx_size;

    if (size < sizeof(ImgUartHeader_t) + 2u)
    {
        nascerr_uart_image_send_nack();
        return ERR_COMM;
    }

    /* Extrai header (big-endian) */
    ImgUartHeader_t hdr;
    uint16_t magic_be;
    memcpy(&magic_be,        buf,     2u);
    hdr.magic       = __builtin_bswap16(magic_be);
    hdr.cmd         = buf[2u];
    uint16_t len_be;
    memcpy(&len_be,          buf + 3u, 2u);
    hdr.payload_len = __builtin_bswap16(len_be);

    uint16_t frame_total = sizeof(ImgUartHeader_t)
                         + hdr.payload_len
                         + 2u;  /* CRC16 */

    if (frame_total > size || !img_frame_crc_valid(buf, frame_total))
    {
        nascerr_uart_image_send_nack();
        nascerr_uart_image_set_stat(IMG_STATE_ERROR, 0u, IMG_ERR_CRC_FRAME);
        return ERR_COMM;
    }

    uint8_t *payload = buf + sizeof(ImgUartHeader_t);

    switch ((ImgUartCmd_t)hdr.cmd)
    {
        /* ── START ─────────────────────────────────────────────── */
        case IMG_UART_START:
        {
            if (hdr.payload_len != sizeof(ImgStartPayload_t))
            {
                nascerr_uart_image_send_nack();
                nascerr_uart_image_set_stat(IMG_STATE_ERROR, 0u, IMG_ERR_BAD_FRAME);
                return ERR_COMM;
            }

            ImgStartPayload_t s;
            memcpy(&s, payload, sizeof(s));
            s.total_size = __builtin_bswap32(s.total_size);
            s.crc16      = __builtin_bswap16(s.crc16);

            if (s.total_size == 0u || s.total_size > FLASH_IMAGE_MAX_BYTES)
            {
                nascerr_uart_image_send_nack();
                nascerr_uart_image_set_stat(IMG_STATE_ERROR, 0u, IMG_ERR_SIZE);
                return ERR_COMM;
            }

            g_img_ctx.total_size     = s.total_size;
            g_img_ctx.crc16_expected = s.crc16;
            g_img_ctx.bytes_done       = 0u;
            g_img_ctx.next_expected_id = 0u;

            if (!nascerr_db_clearImageArea())
            {
                nascerr_uart_image_send_nack();
                nascerr_uart_image_set_stat(IMG_STATE_ERROR, 0u, IMG_ERR_FLASH_WRITE);
                return ERR_COMM;
            }

            if (!nascerr_db_setImageSize(s.total_size))
            {
            	nascerr_uart_image_send_nack();
            	return ERR_FLASH;
            }

            nascerr_uart_image_set_stat(IMG_STATE_UPLOADING, 0u, IMG_ERR_NONE);
            nascerr_uart_image_send_ack();
            break;
        }

        /* ── CHUNK ──────────────────────────────────────────────── */
        case IMG_UART_CHUNK:
        {
            if (hdr.payload_len < 4u)
            {
                nascerr_uart_image_send_nack();
                nascerr_uart_image_set_stat(IMG_STATE_ERROR, 0u, IMG_ERR_BAD_FRAME);
                return ERR_COMM;
            }

            uint32_t chunk_id_be;
            memcpy(&chunk_id_be, payload, 4u);
            uint32_t chunk_id = __builtin_bswap32(chunk_id_be);
            uint16_t data_len = hdr.payload_len - 4u;
            uint8_t *data     = payload + 4u;

            /* Duplicado: chunk já escrito em tentativa anterior → ACK idempotente */
            if (chunk_id < g_img_ctx.next_expected_id)
            {
                DEBUG_PRINT(LOG_WARN, "CHUNK dup: id=%lu esperado=%lu — ACK sem escrever\r\n",
                            chunk_id, g_img_ctx.next_expected_id);
                nascerr_uart_image_send_ack();
                break;
            }

            /* Fora de ordem: gap entre o esperado e o recebido */
            if (chunk_id > g_img_ctx.next_expected_id)
            {
                DEBUG_PRINT(LOG_ERROR,
                    "CHUNK fora de ordem: id=%lu esperado=%lu — NACK+id\r\n",
                    chunk_id, g_img_ctx.next_expected_id);

                nascerr_uart_image_send_nack();
                nascerr_uart_image_set_stat(IMG_STATE_ERROR, 0u, IMG_ERR_BAD_FRAME);
                return ERR_COMM;
            }

            /* chunk_id == next_expected_id: valida tamanho antes de escrever */
            if (g_img_ctx.bytes_done + data_len > g_img_ctx.total_size)
            {
                DEBUG_PRINT(LOG_ERROR, "CHUNK tamanho excede imagem: offset=%lu + len=%u > total=%lu — NACK\r\n",
                            g_img_ctx.bytes_done, data_len, g_img_ctx.total_size);
                nascerr_uart_image_send_nack();
                nascerr_uart_image_set_stat(IMG_STATE_ERROR, 0u, IMG_ERR_SIZE);
                return ERR_COMM;
            }

            if (!nascerr_db_writeImageChunk(g_img_ctx.bytes_done, data, data_len))
            {
                nascerr_uart_image_send_nack();
                nascerr_uart_image_set_stat(IMG_STATE_ERROR, 0u, IMG_ERR_FLASH_WRITE);
                return ERR_COMM;
            }

            g_img_ctx.bytes_done += data_len;
            g_img_ctx.next_expected_id++;
            uint8_t pct = (uint8_t)((g_img_ctx.bytes_done * 100u)
                                    / g_img_ctx.total_size);
            nascerr_uart_image_set_stat(IMG_STATE_UPLOADING, pct, IMG_ERR_NONE);
            nascerr_uart_image_send_ack();
            break;
        }
        /* ── FINISH ─────────────────────────────────────────────── */
        case IMG_UART_FINISH:
        {
            nascerr_uart_image_set_stat(IMG_STATE_VALIDATING, 100u, IMG_ERR_NONE);

            if (!nascerr_db_validateImage(g_img_ctx.total_size,
                                          g_img_ctx.crc16_expected))
            {
                DEBUG_PRINT(LOG_WARN, "Imagem nao validada enviando nack\r\n");
                nascerr_uart_image_send_nack();
                nascerr_uart_image_set_stat(IMG_STATE_ERROR, 100u, IMG_ERR_CRC_GLOBAL);
                return ERR_COMM;
            }

            DEBUG_PRINT(LOG_INFO, "Iniciando processo de escrita da validacao na flash\r\n");
            experimentHeader.image_data.header.in_use     = FLASH_IN_USE_BYTES;
            experimentHeader.image_data.header.start_addr = W25N01G_BLOCK_TO_LINEAR(FLASH_IMAGE_BLOCK);
            experimentHeader.image_data.image_size = g_img_ctx.total_size;
            experimentHeader.image_data.crc16      = g_img_ctx.crc16_expected;
            experimentHeader.image_data.valid      = 0xAAu;
            nascerr_db_updateHeader();

//            nascerr_uart_image_set_stat(IMG_STATE_READY, 100u, IMG_ERR_NONE);
            nascerr_uart_image_set_stat(IMG_STATE_READY, 100u, IMG_ERR_NONE);
            nascerr_uart_image_send_ack_finish();   /* 0x07 em vez de 0x06 */
            break;
            break;
        }

        /* ── ABORT ──────────────────────────────────────────────── */
        case IMG_UART_ABORT:
        {
            nascerr_db_clearImageArea();

            nascerr_uart_image_set_stat(IMG_STATE_EMPTY, 0u, IMG_ERR_NONE);
            nascerr_uart_image_send_ack();
            break;
        }

        default:
            nascerr_uart_image_send_nack();
            nascerr_uart_image_set_stat(IMG_STATE_ERROR, 0u, IMG_ERR_BAD_FRAME);
            return ERR_COMM;
    }

    return STATUS_SUCCESS;
}

/*
 * Registra o callback fornecido pelo scheduler
 */
void nascerr_uart_image_register_notify(ImgRxNotifyCallback_t cb)
{
    s_notify_cb = cb;
}

/*
 * Verifica se há necessidade de reiniciar o DMA (erro de hardware ocorreu).
 * Consultado pela imgUploadTask antes de tentar extrair frames.
 */
uint8_t nascerr_uart_image_need_restart(void)
{
    return s_need_restart;
}

uint8_t nascerr_uart_image_need_send_nack(void)
{
	return s_send_nack;
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_uart_image_set_stat
 *  @brief     : Atualiza o registrador IMG_STAT para feedback ao Master via I2C.
 * ******************************************************************************
 */
void nascerr_uart_image_set_stat(ImgUploadState_t state, uint8_t pct, ImgErrorCode_t err)
{
	s_img_stat.valid        = (state == IMG_STATE_READY) ? 0xAAu : 0x00u;
	s_img_stat.upload_state = (uint8_t)state;
	s_img_stat.progress_pct = pct;
	s_img_stat.image_size = g_img_ctx.total_size;
	s_img_stat.error_code   = (uint8_t)err;
	memcpy(g_register_data[IMAGE_STAT].value,&s_img_stat,sizeof(ImgStatReg_t));
}

ImgStatReg_t nascerr_uart_image_get_stat(void)
{
    return s_img_stat;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_uart_image_is_valid
 *  @brief     : Verifica se há imagem válida gravada consultando o header.
 * ******************************************************************************
 */
uint8_t nascerr_uart_image_is_valid(void)
{
    return (experimentHeader.image_data.valid == 0xAAu);
}

void nascerr_uart_image_send_ack(void)
{
    uint8_t ack = 0x06u;
    HAL_UART_Transmit(s_huart, &ack, 1u, 100u);
}

void nascerr_uart_image_send_nack(void)
{
	uint32_t next_id = g_img_ctx.next_expected_id;
    uint8_t buf[3] = {
        0x15u,
        (uint8_t)((next_id >> 8u) & 0xFFu),
        (uint8_t)( next_id        & 0xFFu)
    };
    HAL_UART_Transmit(s_huart, buf, sizeof(buf), 100u);
}


