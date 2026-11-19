/**
 * ******************************************************************************
 *  @file    : nascerr_util.h
 *  @brief   : Header for nascerr_util.c file.
 *  @author  : Marcus Anderson; João Manoel
 * ******************************************************************************
 */

#ifndef UTIL_H_
#define UTIL_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "main.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "NASCERR_System.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

/* Macro to display debug messages with contextual information */
#if ENABLE_LOGS
    #define DEBUG_PRINT(level, fmt, ...) \
        debug_log(level, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(level, fmt, ...) // No output, debug is disabled
#endif

#define MIN(a,b) ((a)<(b)?(a):(b))

/* > Macros de log e notificacao */
#define LOG_AND_NOTIFY(_lvl, _status, _err)                 \
    do {                                                    \
        nascerr_log_add((_lvl), (_status), (_err));         \
    } while (0)

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
	LOG_LOG,
    LOG_ERROR
} LogLevel;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

const char* log_level_to_string(LogLevel level);

void debug_log(LogLevel level, const char *file, int line, const char *func, const char *fmt, ...);

uint16_t calculate_crc16(const uint8_t *data, size_t length);

uint16_t update_crc16(uint16_t crc, const uint8_t *data, size_t length);

uint8_t verify_crc(const uint8_t *data, uint32_t len, uint16_t crc_received);

void check_reset_source(void);

#endif /* UTIL_H_ */
