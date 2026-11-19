/*
 * NASCERR_ECCs_32bits.c
 *
 *  Created on: Nov 1, 2024
 *      Author: Clara
 */

#include "NASCERR_ECCs_32bits.h"


/*
 * ******************************************************************************
 *  Function Name: nascerr_eccs_tbec_encoder
 *  @brief       : Encodes 32-bit input data using Triple Bit Error Correction (TBEC).
 *                 Produces a 32-bit encoded word that includes the original data
 *                 and error correction bits.
 *  @param       : data_in - The 32-bit input data to be encoded.
 *  @retval      : A 32-bit encoded word containing the TBEC encoded data.
 * ******************************************************************************
 */
uint32_t nascerr_eccs_tbec_encoder(uint32_t data_in) {
    uint8_t signal[4][4];
    uint8_t Cb[4][2];
    uint8_t P[4];
    uint8_t Di[4];
    uint32_t data_out = 0;

    // Extrair 16 bits de entrada
    uint16_t W = data_in & 0xFFFF;

    // Dividir os dados de entrada em blocos de 4 bits (signal[x][y])
    signal[0][0] = (W >> 15) & 1;
    signal[0][1] = (W >> 14) & 1;
    signal[0][2] = (W >> 13) & 1;
    signal[0][3] = (W >> 12) & 1;
    signal[1][0] = (W >> 11) & 1;
    signal[1][1] = (W >> 10) & 1;
    signal[1][2] = (W >> 9) & 1;
    signal[1][3] = (W >> 8) & 1;
    signal[2][0] = (W >> 7) & 1;
    signal[2][1] = (W >> 6) & 1;
    signal[2][2] = (W >> 5) & 1;
    signal[2][3] = (W >> 4) & 1;
    signal[3][0] = (W >> 3) & 1;
    signal[3][1] = (W >> 2) & 1;
    signal[3][2] = (W >> 1) & 1;
    signal[3][3] = (W >> 0) & 1;

    // Cálculo de Cb (soma de bits de controle)
    for (int b = 0; b < 4; ++b) {
        Cb[b][0] = signal[b][0] ^ signal[b][2];
        Cb[b][1] = signal[b][1] ^ signal[b][3];
    }

    // Cálculo de P (bits de paridade)
    P[0] = signal[0][0] ^ signal[0][1] ^ signal[1][0] ^ signal[1][1];
    P[1] = signal[2][0] ^ signal[2][1] ^ signal[3][0] ^ signal[3][1];
    P[2] = signal[0][2] ^ signal[0][3] ^ signal[1][2] ^ signal[1][3];
    P[3] = signal[2][2] ^ signal[2][3] ^ signal[3][2] ^ signal[3][3];

    // Cálculo de Di (dados de paridade diagonal)
    Di[0] = signal[0][0] ^ signal[1][1] ^ signal[2][0] ^ signal[3][1];
    Di[1] = signal[0][1] ^ signal[1][0] ^ signal[2][1] ^ signal[3][0];
    Di[2] = signal[0][2] ^ signal[1][3] ^ signal[2][2] ^ signal[3][3];
    Di[3] = signal[0][3] ^ signal[1][2] ^ signal[2][3] ^ signal[3][2];

    // Montagem do dado de saída
    data_out |= (signal[0][0] << 31) | (signal[1][0] << 30) | (signal[2][0] << 29) | (signal[3][0] << 28);
    data_out |= (signal[0][1] << 27) | (signal[1][1] << 26) | (signal[2][1] << 25) | (signal[3][1] << 24);
    data_out |= (signal[0][2] << 23) | (signal[1][2] << 22) | (signal[2][2] << 21) | (signal[3][2] << 20);
    data_out |= (signal[0][3] << 19) | (signal[1][3] << 18) | (signal[2][3] << 17) | (signal[3][3] << 16);
    data_out |= (Di[0] << 15) | (Di[3] << 14) | (Di[1] << 13) | (Di[2] << 12);
    data_out |= (P[0] << 11) | (P[3] << 10) | (P[1] << 9) | (P[2] << 8);
    data_out |= (Cb[0][0] << 7) | (Cb[0][1] << 6) | (Cb[1][0] << 5) | (Cb[1][1] << 4);
    data_out |= (Cb[2][0] << 3) | (Cb[2][1] << 2) | (Cb[3][0] << 1) | (Cb[3][1] << 0);

    return data_out;
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_eccs_mrsc_encoder
 *  @brief       : Encodes 32-bit input data using Multiple Redundant Source Coding (MRSC).
 *                 Produces a 32-bit encoded word that includes the original data
 *                 and error correction bits.
 *  @param       : data_in - The 32-bit input data to be encoded.
 *  @retval      : A 32-bit encoded word containing the MRSC encoded data.
 * ******************************************************************************
 */
uint32_t nascerr_eccs_mrsc_encoder(uint32_t data_in) {
    uint8_t signal[4][4];
    uint8_t Cb[4][2];
    uint8_t P[4];
    uint8_t Di[4];
    uint32_t data_out = 0;

    // Extrair os 16 bits de entrada
    uint16_t W = data_in & 0xFFFF;

    // Dividir os dados de entrada em blocos de 4 bits (signal[x][y])
    signal[0][0] = (W >> 15) & 1;
    signal[0][1] = (W >> 14) & 1;
    signal[0][2] = (W >> 13) & 1;
    signal[0][3] = (W >> 12) & 1;
    signal[1][0] = (W >> 11) & 1;
    signal[1][1] = (W >> 10) & 1;
    signal[1][2] = (W >> 9) & 1;
    signal[1][3] = (W >> 8) & 1;
    signal[2][0] = (W >> 7) & 1;
    signal[2][1] = (W >> 6) & 1;
    signal[2][2] = (W >> 5) & 1;
    signal[2][3] = (W >> 4) & 1;
    signal[3][0] = (W >> 3) & 1;
    signal[3][1] = (W >> 2) & 1;
    signal[3][2] = (W >> 1) & 1;
    signal[3][3] = (W >> 0) & 1;

    // Cálculo de Cb (soma de bits de controle)
    for (int b = 0; b < 4; ++b) {
        Cb[b][0] = signal[b][0] ^ signal[b][2];
        Cb[b][1] = signal[b][1] ^ signal[b][3];
    }

    // Cálculo de P (bits de paridade)
    P[0] = signal[0][0] ^ signal[1][0] ^ signal[2][0] ^ signal[3][0];
    P[1] = signal[0][1] ^ signal[1][1] ^ signal[2][1] ^ signal[3][1];
    P[2] = signal[0][2] ^ signal[1][2] ^ signal[2][2] ^ signal[3][2];
    P[3] = signal[0][3] ^ signal[1][3] ^ signal[2][3] ^ signal[3][3];

    // Cálculo de Di (dados de paridade diagonal)
    Di[0] = signal[0][0] ^ signal[1][1] ^ signal[2][0] ^ signal[3][1];
    Di[1] = signal[0][1] ^ signal[1][0] ^ signal[2][1] ^ signal[3][0];
    Di[2] = signal[0][2] ^ signal[1][3] ^ signal[2][2] ^ signal[3][3];
    Di[3] = signal[0][3] ^ signal[1][2] ^ signal[2][3] ^ signal[3][2];

    // Montagem do dado de saída
    data_out |= (signal[0][0] << 31) | (signal[1][0] << 30) | (signal[2][0] << 29) | (signal[3][0] << 28);
    data_out |= (signal[0][1] << 27) | (signal[1][1] << 26) | (signal[2][1] << 25) | (signal[3][1] << 24);
    data_out |= (signal[0][2] << 23) | (signal[1][2] << 22) | (signal[2][2] << 21) | (signal[3][2] << 20);
    data_out |= (signal[0][3] << 19) | (signal[1][3] << 18) | (signal[2][3] << 17) | (signal[3][3] << 16);
    data_out |= (Di[0] << 15) | (Di[3] << 14) | (Di[1] << 13) | (Di[2] << 12);
    data_out |= (P[0] << 11) | (P[3] << 10) | (P[1] << 9) | (P[2] << 8);
    data_out |= (Cb[0][0] << 7) | (Cb[0][1] << 6) | (Cb[1][0] << 5) | (Cb[1][1] << 4);
    data_out |= (Cb[2][0] << 3) | (Cb[2][1] << 2) | (Cb[3][0] << 1) | (Cb[3][1] << 0);

    return data_out;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_eccs_encoded_buffer
 *  @brief       : Encodes a buffer of 32-bit data using the specified ECC mode,
 *                 producing a 32-bit encoded output for each input.
 *  @param       : write_buffer - Pointer to the input buffer containing 32-bit data
 *                                 to be encoded.
 *  @param       : encoded_buffer - Pointer to the buffer where the 32-bit encoded data
 *                                   will be stored.
 *  @param       : ecc - The ECC mode to be used for encoding. Possible values:
 *                        TBEC - TBEC ECC encoding
 *                        TBEC_1_TO_3 - TBEC ECC encoding with 1 to 3 errors
 *                        TBEC_1_TO_7 - TBEC ECC encoding with 1 to 7 errors
 *                        MRSC - MRSC encoding
 *  @param       : length - The number of elements to encode in the buffers.
 *  @retval      : None.
 * ******************************************************************************
 */
void nascerr_eccs_encoded_buffer(uint32_t* write_buffer, uint32_t* encoded_buffer, NASCERR_MODE ecc, nascerr_block length) {
	switch(ecc) {
	// Codifica usando o codificador Hamming para saída de 32 bits
	case TBEC:
		for(int i = 0; i < length; i++) {
			encoded_buffer[i] = nascerr_eccs_tbec_encoder(write_buffer[i]);
		}
		break;

	case TBEC_1_TO_3:
		for(int i = 0; i < length; i++) {
			encoded_buffer[i] = nascerr_eccs_tbec_encoder(write_buffer[i]);
		}
		break;

	case TBEC_1_TO_7:
		for(int i = 0; i < length; i++) {
			encoded_buffer[i] = nascerr_eccs_tbec_encoder(write_buffer[i]);
		}
		break;

		// Codifica usando o codificador TBEC para saída de 32 bits
	case MRSC:
		for(int i = 0; i < length; i++) {
			encoded_buffer[i] = nascerr_eccs_mrsc_encoder(write_buffer[i]);
		}
		break;

	default:
		break;
	}
}

