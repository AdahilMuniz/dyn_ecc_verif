#include "main.h"
#include <stdio.h>

/**
******************************************************************************
  Function Name: calculate_crc16
  @brief     : Calculates the CRC16 checksum for a given byte array in C.
  @param     : data - Pointer to the byte array for which CRC16 will be calculated.
               length - Number of bytes in the array.
  @retval    : CRC16 value as a 16-bit unsigned integer.
******************************************************************************
  @details   : This function computes the CRC16 checksum using the standard
               CRC-16 Modbus algorithm. It processes each byte in the input array,
               performing a bitwise XOR operation with the CRC, and applies the
               polynomial 0xA001.

               CRC16 Calculation process:
               - Initializes CRC to 0xFFFF.
               - For each byte in the array, XORs with the CRC and processes
                 each bit of the byte.
               - The polynomial 0xA001 is applied during each shift.
               - Returns the final 16-bit checksum.
******************************************************************************
 */
uint16_t calculate_crc16(uint8_t *data, size_t length) {
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

/**
******************************************************************************
  Function Name: verify_crc
  @brief     : Verifies if the calculated CRC16 matches the received CRC16.
  @param     : data - Pointer to the byte array for which CRC16 needs to be verified.
               length - Number of bytes in the array.
               crc_received - The received CRC16 to compare against.
  @retval    : 1 if the CRCs match, 0 otherwise.
******************************************************************************
  @details   : This function calculates the CRC16 for the given byte array using
               the `calculate_crc16` function. It then compares the calculated CRC
               with the provided `crc_received`.

               CRC Verification process:
               - The function first calculates the CRC using the input data.
               - It then compares the calculated CRC with the provided value.
               - Returns 1 if they match, 0 otherwise.
******************************************************************************
 */
uint8_t verify_crc(uint8_t *data, uint16_t crc_received) {
    uint16_t crc_calculated = calculate_crc16(data, sizeof(data));
    return crc_calculated == crc_received;
}
