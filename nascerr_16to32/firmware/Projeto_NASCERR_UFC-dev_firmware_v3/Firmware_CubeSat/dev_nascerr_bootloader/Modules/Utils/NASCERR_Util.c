/**
 * ******************************************************************************
 *  @file    : nascerr_util.c
 *  @brief   : Biblioteca para gravar dados no SCHEDULER
 *  @author  : Marcus Anderson; João Manoel
 *  @note    :
 *
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "NASCERR_Util.h"

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: log_level_to_string
 *  @brief     : Converte o nivel de log para a string correspondente.
 *  @param     : level - nivel de log a ser convertido.
 *  @retval    : Ponteiro para string representando o nivel de log.
 * ******************************************************************************
 */

const char* log_level_to_string(LogLevel level)
{
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "UNKNOWN";
    }
}

/*
 * ******************************************************************************
 *  Function Name: debug_log
 *  @brief     : Log function that displays debug messages with file, line,
 *               function, and log level information
 *  @param     : level - log message level (using LogLevel)
 *               file - name of the file where the log was called
 *               line - line number where the log was called
 *               func - function name where the log was called
 *               fmt  - message format
 *               ...  - variable arguments for the format message
 *  @retval    : None
 * ******************************************************************************
 */
void debug_log(LogLevel level, const char *file, int line, const char *func, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("[%s] %s:%d (%s): ", log_level_to_string(level), file, line, func);
    vprintf(fmt, args);
    va_end(args);
}

/*
 * ******************************************************************************
 *  Function Name: calculate_crc16
 *  @brief     : Calcula o checksum CRC16 para um array de bytes.
 *  @param     : data   - Ponteiro para o array de bytes para o qual o CRC16 sera
 *  					  calculado.
 *  @param     : length - Numero de bytes no array.
 *  @retval    : Valor CRC16 como inteiro de 16 bits sem sinal.
 * ******************************************************************************
 */
uint16_t calculate_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;  // Valor inicial do CRC
    uint16_t poly = 0xA001; // Polinômio gerador (CRC-16)

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i]; // XOR com o próximo byte de dados

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ poly; // Desloca e XOR com o polinômio
            } else {
                crc >>= 1; // Apenas desloca
            }
        }
    }

    return crc; // Retorna o CRC calculado
}

/*
 * ******************************************************************************
 *  Function Name: verify_crc
 *  @brief     : Verifica se o CRC16 calculado corresponde ao CRC16 recebido.
 *  @param     : data         - Ponteiro para o array de bytes a ser verificado.
 *  @param     : len          - Quantidade de bytes em *data* (sem incluir o CRC).
 *  @param     : crc_received - CRC16 recebido para comparação.
 *  @retval    : 1 se os CRCs coincidirem, 0 caso contrário.
 * ******************************************************************************
 */
uint8_t verify_crc(const uint8_t *data, uint32_t len, uint16_t crc_received)
{
    uint16_t crc_calculated = calculate_crc16(data, len);
    return (crc_calculated == crc_received);
}


/*
 * ******************************************************************************
 *  Function Name: check_reset_source
 *  @brief     : Checks if the last system reset was caused by the independent watchdog
 *               and logs an error if detected.
 *  @param     : void
 *  @retval    : void
 * ******************************************************************************
 */
void check_reset_source(void)
{
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
    {

        DEBUG_PRINT(LOG_ERROR, "RESET POR WATCHDOG | POSSIVEL TRAVAMENTO I2C \r\n");
        __HAL_RCC_CLEAR_RESET_FLAGS();
    }
}

void blink_led(uint16_t delay_ms){
	HAL_GPIO_TogglePin(LED_DEBUG_GPIO_Port, LED_DEBUG_Pin);
	HAL_Delay(delay_ms);
}
