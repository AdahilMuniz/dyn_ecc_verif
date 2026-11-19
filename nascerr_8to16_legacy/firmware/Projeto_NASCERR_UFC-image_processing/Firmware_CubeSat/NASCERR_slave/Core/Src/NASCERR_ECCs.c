/*
 * NASCERR_ECCs.c
 *
 *  Created on: Sep 16, 2024
 *      Author: Clara
 */


#include "NASCERR_ECCs.h"


/*
 * ******************************************************************************
 *  Function Name: nascerr_eccs_edac_encoder
 *  @brief       : Encodes 8-bit data using EDAC (Error Detection And Correction) encoding.
 *                 Produces a 16-bit word that includes the original data and parity bits.
 *  @param       : data_in - The 8-bit input data to be encoded (the least significant 8 bits are used).
 *  @retval      : A 16-bit encoded word including the input data and the ECC.
 * ******************************************************************************
 */
uint16_t nascerr_eccs_edac_encoder(uint16_t data_in) {
	uint8_t W = data_in & 0xFF;  // Mascarar os 8 bits menos significativos
	uint16_t V = 0;

	// Calcula os bits de paridade (V[0] a V[7])
	V |= ((W >> 2) ^ (W >> 4) ^ (W >> 6)) & 1;              // V[0]
	V |= (((W >> 7) ^ (W >> 0)) & 1) << 1;                   // V[1]
	V |= (((W >> 2) ^ (W >> 4) ^ (W >> 5)) & 1) << 2;        // V[2]
	V |= (((W >> 7) ^ (W >> 1) ^ (W >> 2) ^ (W >> 5) ^ (W >> 6)) & 1) << 3; // V[3]
	V |= (((W >> 2) ^ (W >> 4) ^ (W >> 7) ^ (W >> 0)) & 1) << 4;  // V[4]
	V |= (((W >> 7) ^ (W >> 6) ^ (W >> 2) ^ (W >> 5) ^ (W >> 4)) & 1) << 5;  // V[5]
	V |= (((W >> 6) ^ (W >> 3) ^ (W >> 4) ^ (W >> 5)) & 1) << 6;  // V[6]
	V |= (((W >> 7) ^ (W >> 1) ^ (W >> 4) ^ (W >> 5) ^ (W >> 6) ^ (W >> 3)) & 1) << 7;  // V[7]

	// Calcula os bits ECC (V[8] a V[15])
	V |= (((V >> 0) ^ (V >> 4) ^ (V >> 5) ^ (V >> 2)) & 1) << 8;    // V[8]
	V |= (((V >> 0) ^ (V >> 1) ^ (V >> 2) ^ (V >> 4) ^ (V >> 5) ^ (V >> 6) ^ (V >> 7)) & 1) << 9; // V[9]
	V |= (((V >> 0) ^ (V >> 2) ^ (V >> 3) ^ (V >> 6) ^ (V >> 7)) & 1) << 10;  // V[10]
	V |= (((V >> 1) ^ (V >> 3) ^ (V >> 4) ^ (V >> 7)) & 1) << 11;   // V[11]
	V |= (((V >> 0) ^ (V >> 1) ^ (V >> 2) ^ (V >> 3) ^ (V >> 4) ^ (V >> 6) ^ (V >> 7)) & 1) << 12;  // V[12]
	V |= (((V >> 1) ^ (V >> 2) ^ (V >> 4)) & 1) << 13;  // V[13]
	V |= (((V >> 0) ^ (V >> 1) ^ (V >> 4)) & 1) << 14;  // V[14]
	V |= (((V >> 0) ^ (V >> 1) ^ (V >> 4) ^ (V >> 5) ^ (V >> 2)) & 1) << 15; // V[15]

	return V;
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_eccs_hamming_encoder
 *  @brief       : Encodes 8-bit data using Hamming encoding. The resulting 16-bit
 *                 word includes the original data and Hamming parity bits.
 *  @param       : data_in - The 8-bit input data to be encoded.
 *  @retval      : A 16-bit encoded word containing both the original data and Hamming parity bits.
 * ******************************************************************************
 */
uint16_t nascerr_eccs_hamming_encoder(uint16_t data_in) {
    uint16_t encoded_data = 0;
    uint8_t parity_bits[4];

    // Assume que data_in é o valor de 8 bits (os 8 bits menos significativos)
    uint8_t W = data_in & 0xFF; // Extrai os 8 bits

    // Calcula os bits de paridade
    parity_bits[0] = (W & 0x01) ^ ((W >> 1) & 0x01) ^ ((W >> 3) & 0x01) ^ ((W >> 4) & 0x01) ^ ((W >> 6) & 0x01);
    parity_bits[1] = (W & 0x01) ^ ((W >> 2) & 0x01) ^ ((W >> 3) & 0x01) ^ ((W >> 5) & 0x01) ^ ((W >> 6) & 0x01);
    parity_bits[2] = ((W >> 1) & 0x01) ^ ((W >> 2) & 0x01) ^ ((W >> 3) & 0x01) ^ ((W >> 7) & 0x01);
    parity_bits[3] = ((W >> 4) & 0x01) ^ ((W >> 5) & 0x01) ^ ((W >> 6) & 0x01) ^ ((W >> 7) & 0x01);

    // Cria os dados codificados com os bits de paridade
    encoded_data |= (parity_bits[0] << 0);  // encoded_data[0]
    encoded_data |= (parity_bits[1] << 1);  // encoded_data[1]
    encoded_data |= ((W & 0x01) << 2);       // encoded_data[2] (data_in[0])
    encoded_data |= (parity_bits[2] << 3);   // encoded_data[3]
    encoded_data |= ((W >> 1) & 0x01) << 4;  // encoded_data[4] (data_in[1])
    encoded_data |= ((W >> 2) & 0x01) << 5;  // encoded_data[5] (data_in[2])
    encoded_data |= ((W >> 3) & 0x01) << 6;  // encoded_data[6] (data_in[3])
    encoded_data |= (parity_bits[3] << 7);   // encoded_data[7]
    encoded_data |= ((W >> 4) & 0x01) << 8;  // encoded_data[8] (data_in[4])
    encoded_data |= ((W >> 5) & 0x01) << 9;  // encoded_data[9] (data_in[5])
    encoded_data |= ((W >> 6) & 0x01) << 10; // encoded_data[10] (data_in[6])
    encoded_data |= ((W >> 7) & 0x01) << 11; // encoded_data[11] (data_in[7])

    // Os bits encoded_data[15:12] são zero
    encoded_data &= 0x0FFF;  // Limpa os 4 bits mais significativos

    return encoded_data;
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_eccs_tbec_encoder
 *  @brief       : Encodes 8-bit data using Triple Bit Error Correction (TBEC) encoding.
 *                 Produces a 16-bit encoded word that includes original data and error correction.
 *  @param       : data_in - The 8-bit input data to be encoded.
 *  @retval      : A 16-bit encoded word including the TBEC encoded data.
 * ******************************************************************************
 */
uint16_t nascerr_eccs_tbec_encoder(uint16_t data_in) {
    uint8_t signal[4][2];
    uint8_t Cb[2][2];
    uint8_t P[2];
    uint8_t Di[2];
    uint16_t data_out = 0;

    // Assume que data_in é um valor de 8 bits (os 8 bits menos significativos)
    uint8_t W = data_in & 0xFF; // Extrai os 8 bits

    // Separar o dado de entrada em sinais de 2 bits cada
    signal[0][0] = (W >> 0) & 1;
    signal[1][0] = (W >> 1) & 1;
    signal[2][0] = (W >> 2) & 1;
    signal[3][0] = (W >> 3) & 1;
    signal[0][1] = (W >> 4) & 1;
    signal[1][1] = (W >> 5) & 1;
    signal[2][1] = (W >> 6) & 1;
    signal[3][1] = (W >> 7) & 1;

    // Cálculo de Cb
    Cb[0][0] = signal[0][0] ^ signal[2][0];
    Cb[0][1] = signal[1][0] ^ signal[3][0];
    Cb[1][0] = signal[0][1] ^ signal[2][1];
    Cb[1][1] = signal[1][1] ^ signal[3][1];

    // Cálculo de P
    P[0] = signal[0][0] ^ signal[0][1] ^ signal[1][0] ^ signal[1][1];
    P[1] = signal[2][0] ^ signal[2][1] ^ signal[3][0] ^ signal[3][1];

    // Cálculo de Di
    Di[0] = signal[0][0] ^ signal[1][1] ^ signal[2][0] ^ signal[3][1];
    Di[1] = signal[0][1] ^ signal[1][0] ^ signal[2][1] ^ signal[3][0];

    // Montar o dado de saída
    data_out |= (Cb[1][1] << 15) | (Cb[1][0] << 14) |
                (Cb[0][1] << 13) | (Cb[0][0] << 12) |
                (P[1] << 11) | (P[0] << 10) |
                (Di[1] << 9) | (Di[0] << 8) |
                (signal[3][1] << 7) | (signal[2][1] << 6) |
                (signal[1][1] << 5) | (signal[0][1] << 4) |
                (signal[3][0] << 3) | (signal[2][0] << 2) |
                (signal[1][0] << 1) | (signal[0][0] << 0);

    return data_out;
}


/*
 * ******************************************************************************
 *  Function Name: nascerr_eccs_encoded_buffer
 *  @brief       : Encodes a buffer of data using the specified ECC mode.
 *  @param       : write_buffer - Pointer to the input buffer containing data to be encoded.
 *  @param       : encoded_buffer - Pointer to the buffer where encoded data will be stored.
 *  @param       : ecc - The mode of ECC to be used for encoding. Possible values:
 *                        ECC_MOD1 - Hamming encoding
 *                        ECC_MOD2 - TBEC encoding
 *                        ECC_MOD3 - EDAC encoding
 *  @param       : length - The number of elements to encode in the buffers.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_eccs_encoded_buffer(uint16_t* write_buffer, uint16_t* encoded_buffer, NASCERR_MODE ecc, nascerr_block length){
	//	uint16_t encoded_buffer[length];

	switch(ecc){

		// Codifica 0x00.
	case ECC_MOD1:
		for(int i = 0; i < length; i++){
			encoded_buffer[i] = nascerr_eccs_hamming_encoder(write_buffer[i]);
		}
		break;

		// Preenche o buffer com 0xFFFF.
	case ECC_MOD2:
		for(int i = 0; i < length; i++){
			encoded_buffer[i] = nascerr_eccs_tbec_encoder(write_buffer[i]);
		}
		break;

		// Codifica o buffer com o codifcador do EDAC
	case ECC_MOD3:
		for(int i = 0; i < length; i++){
			encoded_buffer[i] = nascerr_eccs_edac_encoder(write_buffer[i]);
		}
		break;
	default:
		break;
	}

}
