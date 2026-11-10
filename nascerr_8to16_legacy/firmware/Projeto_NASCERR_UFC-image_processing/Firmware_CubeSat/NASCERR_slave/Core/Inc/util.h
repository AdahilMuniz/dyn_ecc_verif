#include "main.h"
#include <stdio.h>

uint8_t calculate_crc16(uint8_t *data, size_t length);

uint8_t verify_crc(uint8_t *data, uint16_t crc_received);
