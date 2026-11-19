#ifndef UTIL_H_
#define UTIL_H_

#include "main.h"
#include <stdarg.h>
#include <stdio.h>
#include "NASCERR_System.h"

/* Macro to display debug messages with contextual information */
#if ENABLE_LOGS
    #define DEBUG_PRINT(level, fmt, ...) \
        debug_log(level, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(level, fmt, ...) // No output, debug is disabled
#endif

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

const char* log_level_to_string(LogLevel level);
void debug_log(LogLevel level, const char *file, int line, const char *func, const char *fmt, ...);

uint16_t calculate_crc16(uint8_t *data, size_t length);

uint8_t verify_crc(uint8_t *data, uint16_t crc_received);

#endif /* UTIL_H_ */
