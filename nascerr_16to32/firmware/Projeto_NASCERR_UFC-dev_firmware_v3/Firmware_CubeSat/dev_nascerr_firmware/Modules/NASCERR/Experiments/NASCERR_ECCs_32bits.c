/**
 * ******************************************************************************
 * @file    : NASCERR_ECCs_32bits.c
 * @brief   : This file implements Error Correction Code (ECC) selection,
 *            as well as various encoding and decoding schemes for 32-bit
 *            data words.
 * @author  : Clara; Joao Manoel; Cicero Oliveira
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "NASCERR_ECCs_32bits.h"

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */
uint32_t nascerr_eccs_tbec_encoder(uint16_t data_in);
uint32_t nascerr_eccs_mrsc_encoder(uint16_t data_in);
ECC_DECODER nascerr_eccs_tbec_decoder(uint32_t code_in);
ECC_DECODER nascerr_eccs_mrsc_decoder(uint32_t data_in);

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
uint32_t nascerr_ecc_encode(uint16_t data, NASCERR_MODE ecc)
{
	uint32_t aux;
	switch(ecc) {
	// Codifica usando o codificador Hamming para saída de 32 bits
	case TBEC:
		aux = nascerr_eccs_tbec_encoder(data);
		break;

	case MRSC:
		aux = nascerr_eccs_mrsc_encoder(data);
		break;

	default:
		break;
	}
	return aux;
}

/*
 * ******************************************************************************
 * Function Name: nascerr_ecc_decode
 * @brief       : Dispatches to the appropriate ECC decoding function based on the
 *                specified ECC mode. It takes a 32-bit data word and an ECC
 *                mode identifier, then calls the corresponding decoder.
 * @param       : data - The 32-bit encoded data word to be decoded.
 * @param       : ecc - An enumerator of type NASCERR_MODE, indicating the
 *                specific ECC algorithm to be used for decoding (e.g., MRSC, TBEC).
 * @retval      : An ECC_DECODER struct containing the decoding results:
 * - data_raw: The originally received 16-bit data before correction.
 * - data_corrected: The 16-bit data after error correction (if successful).
 * - uncorrectable: A flag (1 if errors remain or were uncorrectable,
 * 0 otherwise).
 * Returns a zero-initialized ECC_DECODER struct if the mode is
 * not supported or if the specific decoder is not yet implemented (e.g., TBEC).
 * ******************************************************************************
 */
ECC_DECODER nascerr_ecc_decode(uint32_t data, NASCERR_MODE ecc)
{
	ECC_DECODER aux = {0,0,0};
	switch(ecc) {
	case TBEC:
		aux = nascerr_eccs_tbec_decoder(data);
		break;

	case MRSC:
		aux = nascerr_eccs_mrsc_decoder(data);
		break;

	default:
		break;
	}
	return aux;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_select_ecc
 *  @brief       : Configures the Error Correcting Code (ECC) selection using GPIO pins.
 *  @param       : ecc - An 8-bit unsigned integer specifying the ECC type to be selected.
 *                        Possible values:
 *                        1 - BYPASS_1 - Bypass to memory 1
 *                        2 - BYPASS_2 - Bypass to memory 2
 *                        3 - TBEC - TBEC ECC
 *                        4 - TBEC_1_TO_3 - TBEC ECC with 1 to 3 errors
 *                        5 - TBEC_1_TO_7 - TBEC ECC with 1 to 7 errors
 *                        6 - BYPASS_1_TO_7 - Bypass with 1 to 7 errors
 *                        7 - MRSC - MRSC ECC
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_select_ecc(NASCERR_MODE ecc)
{
    // Desired logic level for each ECC_SEL_x pin (0 → RESET, 1 → SET)
    GPIO_PinState desired0 = GPIO_PIN_RESET;
    GPIO_PinState desired1 = GPIO_PIN_RESET;
    GPIO_PinState desired2 = GPIO_PIN_RESET;

    switch (ecc) {
        case BYPASS_1:
            // { ECC_SEL_0 = 0, ECC_SEL_1 = 0, ECC_SEL_2 = 0 }
            desired0 = GPIO_PIN_RESET;
            desired1 = GPIO_PIN_RESET;
            desired2 = GPIO_PIN_RESET;
            break;

        case BYPASS_2:
            // { ECC_SEL_0 = 1, ECC_SEL_1 = 0, ECC_SEL_2 = 0 }
            desired0 = GPIO_PIN_SET;
            desired1 = GPIO_PIN_RESET;
            desired2 = GPIO_PIN_RESET;
            break;

        case TBEC:
            // { ECC_SEL_0 = 0, ECC_SEL_1 = 1, ECC_SEL_2 = 0 }
            desired0 = GPIO_PIN_RESET;
            desired1 = GPIO_PIN_SET;
            desired2 = GPIO_PIN_RESET;
            break;

        case MRSC:
            // { ECC_SEL_0 = 1, ECC_SEL_1 = 1, ECC_SEL_2 = 0 }
            desired0 = GPIO_PIN_SET;
            desired1 = GPIO_PIN_SET;
            desired2 = GPIO_PIN_RESET;
            break;

//        case DISABLED:
//        	desired0 = GPIO_PIN_SET;
//			desired1 = GPIO_PIN_SET;
//			desired2 = GPIO_PIN_SET;
//			break;

        default:
            // Invalid mode: DISABLES
        	desired0 = GPIO_PIN_SET;
			desired1 = GPIO_PIN_SET;
			desired2 = GPIO_PIN_SET;
            return;
    }

    GPIO_PinState current0 = nascerr_io_read_pin(GPIO1, ECC_SEL_0);
    GPIO_PinState current1 = nascerr_io_read_pin(GPIO1, ECC_SEL_1);
    GPIO_PinState current2 = nascerr_io_read_pin(GPIO1, ECC_SEL_2);

    if (current0 != desired0) {
        if (desired0 == GPIO_PIN_SET) {
            nascerr_io_pin_set(GPIO1, ECC_SEL_0);
        } else {
            nascerr_io_pin_reset(GPIO1, ECC_SEL_0);
        }
    }
    if (current1 != desired1) {
        if (desired1 == GPIO_PIN_SET) {
            nascerr_io_pin_set(GPIO1, ECC_SEL_1);
        } else {
            nascerr_io_pin_reset(GPIO1, ECC_SEL_1);
        }
    }
    if (current2 != desired2) {
        if (desired2 == GPIO_PIN_SET) {
            nascerr_io_pin_set(GPIO1, ECC_SEL_2);
        } else {
            nascerr_io_pin_reset(GPIO1, ECC_SEL_2);
        }
    }

    const uint16_t timeout = 10000;
    uint16_t counter = 0;
    while (1) {
        current0 = nascerr_io_read_pin(GPIO1, ECC_SEL_0);
        current1 = nascerr_io_read_pin(GPIO1, ECC_SEL_1);
        current2 = nascerr_io_read_pin(GPIO1, ECC_SEL_2);

        if (current0 == desired0 &&
            current1 == desired1 &&
            current2 == desired2) {
            break;
        }

        if (++counter >= timeout) {
            break;
        }
    }
    i2c_slave_noBlockBus();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_ecc_check_encoders_decoders
 *  @brief       : Verifies the consistency of the ECC encoders and decoders
 *                 between the MCU and the FPGA. It encodes data with the MCU,
 *                 compares it with the FPGA encoding, and validates decoding
 *                 for supported ECC modes (TBEC, MRSC).
 *  @param       : None
 *  @retval      : ECC_CHECK_STATUS struct containing encoding and decoding
 *                 results for each ECC mode:
 *                   - encode: ENCODE_SUCESS or ENCODE_FAIL
 *                   - decode: DECODE_SUCESS or DECODE_FAIL
 * ******************************************************************************
 */
ECC_CHECK_STATUS nascerr_ecc_check_encoders_decoders(void)
{
	ECC_CHECK_STATUS stat;
	ECC_ENCODER_STATUS stat_enc = {ENCODE_SUCESS, ENCODE_SUCESS};
	ECC_DECODER_STATUS stat_dec = {DECODE_SUCESS, DECODE_SUCESS};

	uint32_t encoded_mcu;
	uint32_t encoded_fpga;
	uint16_t MSB, LSB;
	uint32_t check = 0;

	const NASCERR_MODE valid_eccs[] = {TBEC, MRSC};

	ECC_COMPOSED_STATUS *status_fields[] = {
		&stat.tbec,
		&stat.msrc
	};

	ENCODING_STAT *stat_fields_enc[] = {
		&stat_enc.tbec,
		&stat_enc.msrc
	};
	DECODING_STAT *stat_fields_dec[] = {
		&stat_dec.tbec,
		&stat_dec.msrc
	};


	for (uint8_t i = 0; i < sizeof(valid_eccs)/sizeof(NASCERR_MODE); i++)
	{
		NASCERR_MODE ecc = valid_eccs[i];

		for (uint16_t j = 0; j < 255; j++)
		{
			//j age como offset e dado escrito
			nascerr_memory_write_block_cell(j, ecc, j, 1);
			encoded_mcu = nascerr_ecc_encode(j, ecc);

			nascerr_select_ecc(BYPASS_1);
			MSB = *nascerr_memory_read(j);
			nascerr_select_ecc(BYPASS_2);
			LSB = *nascerr_memory_read(j);
			encoded_fpga = (MSB << 16) | LSB;

			check = encoded_fpga ^ encoded_mcu;

			if (check != 0)
			{
				*stat_fields_enc[i] = ENCODE_FAIL;
				printf("Fail");
			}
		}

		nascerr_select_ecc(ecc);

		for (uint16_t j = 0; j < 255; j++)
		{
			if( j != (*nascerr_memory_read(j)))
			{
				*stat_fields_dec[i] = DECODE_FAIL;
			}
		}
	}

	for (int i = 0; i < sizeof(valid_eccs)/sizeof(NASCERR_MODE); i++)
	{
		status_fields[i]->encode = *stat_fields_enc[i];
		status_fields[i]->decode = *stat_fields_dec[i];
	}

	//nascerr_select_ecc(DISABLED);
	return stat;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_get_current_ecc
 *  @brief       : Reads the GPIO pin states and determines the currently active
 *                 ECC mode configured in the system.
 *  @param       : None
 *  @retval      : NASCERR_MODE value representing the active ECC mode:
 *                   - BYPASS_1
 *                   - BYPASS_2
 *                   - TBEC
 *                   - MRSC
 *                 Returns -1 if the pin states do not correspond to any valid mode.
 * ******************************************************************************
 */
NASCERR_MODE nascerr_get_current_ecc(void)
{
    GPIO_PinState state0 = nascerr_io_read_pin(GPIO1, ECC_SEL_0);
    GPIO_PinState state1 = nascerr_io_read_pin(GPIO1, ECC_SEL_1);
    GPIO_PinState state2 = nascerr_io_read_pin(GPIO1, ECC_SEL_2);

    if (state0 == GPIO_PIN_RESET && state1 == GPIO_PIN_RESET && state2 == GPIO_PIN_RESET) {
        return BYPASS_1;
    } else if (state0 == GPIO_PIN_SET && state1 == GPIO_PIN_RESET && state2 == GPIO_PIN_RESET) {
        return BYPASS_2;
    } else if (state0 == GPIO_PIN_RESET && state1 == GPIO_PIN_SET && state2 == GPIO_PIN_RESET) {
        return TBEC;
    } else if (state0 == GPIO_PIN_SET && state1 == GPIO_PIN_SET && state2 == GPIO_PIN_RESET) {
        return MRSC;
    } else {
        return -1; // Valor inválido
    }
}

/*
 * ******************************************************************************
 *  Local Functions Definitions
 * ******************************************************************************
 */

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
uint32_t nascerr_eccs_tbec_encoder(uint16_t data_in)
{
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
uint32_t nascerr_eccs_mrsc_encoder(uint16_t data_in)
{
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
 * Function Name: nascerr_eccs_tbec_decoder
 * @brief       : Decodes a 32-bit TBEC (Triple burst error correction) encoded word.
 *                It attempts to detect and correct errors within the data.
 *                The function extracts the original 16-bit data and provides
 *                information about the correction status.
 * @param       : data_in - The 32-bit TBEC encoded word to be decoded.
 * @retval      : An ECC_DECODER struct containing:
 *              - data_raw: The originally received 16-bit data before any correction.
 *              - data_corrected: The 16-bit data after applying error correction.
 *              - uncorrectable: A flag (1 if errors remain after correction, 0 otherwise).
 * ******************************************************************************
 */
ECC_DECODER nascerr_eccs_tbec_decoder(uint32_t code_in)
{
    uint8_t signal[4][4];      /* matriz A,B,C,D                */
    uint8_t Cb[4][2];          /* pares XA..XD  (red[8…15])     */
    uint8_t P [4];             /* P1…P4   (red[4…7])            */
    uint8_t Di[4];             /* Di1…Di4 (red[0…3])            */

    /* ---------- 1. Desmontar o code-word — mesma ordem usada no encoder --- */

    /* dados (bits 31…16)                                                     */
    int pos = 31;
    for (int col = 0; col < 4; ++col)                /* colunas 0-3            */
        for (int row = 0; row < 4; ++row)            /* linhas A-D             */
            signal[row][col] = GETBIT(code_in, pos--);

    /* dado bruto obtido da memoria (sem a aplicacao da correcao)*/
    uint16_t raw = 0;
	raw |= signal[0][0] << 15;  raw |= signal[0][1] << 14;
	raw |= signal[0][2] << 13;  raw |= signal[0][3] << 12;

	raw |= signal[1][0] << 11;  raw |= signal[1][1] << 10;
	raw |= signal[1][2] <<  9;  raw |= signal[1][3] <<  8;

	raw |= signal[2][0] <<  7;  raw |= signal[2][1] <<  6;
	raw |= signal[2][2] <<  5;  raw |= signal[2][3] <<  4;

	raw |= signal[3][0] <<  3;  raw |= signal[3][1] <<  2;
	raw |= signal[3][2] <<  1;  raw |= signal[3][3] <<  0;

    /* Di (bit 15 14 13 12) → Di1,Di4,Di2,Di3                                 */
    Di[0] = GETBIT(code_in, 15);
    Di[3] = GETBIT(code_in, 14);
    Di[1] = GETBIT(code_in, 13);
    Di[2] = GETBIT(code_in, 12);

    /* P  (bit 11 10  9  8) → P1,P4,P2,P3                                     */
    P[0]  = GETBIT(code_in, 11);
    P[3]  = GETBIT(code_in, 10);
    P[1]  = GETBIT(code_in,  9);
    P[2]  = GETBIT(code_in,  8);

    /* Cb pares (bit 7 … 0)                                                   */
    Cb[0][0] = GETBIT(code_in, 7); Cb[0][1] = GETBIT(code_in, 6);
    Cb[1][0] = GETBIT(code_in, 5); Cb[1][1] = GETBIT(code_in, 4);
    Cb[2][0] = GETBIT(code_in, 3); Cb[2][1] = GETBIT(code_in, 2);
    Cb[3][0] = GETBIT(code_in, 1); Cb[3][1] = GETBIT(code_in, 0);

    /* ---------- 2. Re-calcular síndromes  --------------------------------- */
    uint8_t linsin[4][2];
    for (int b = 0; b < 4; ++b) {
        linsin[b][0] = signal[b][0] ^ signal[b][2] ^ Cb[b][0];
        linsin[b][1] = signal[b][1] ^ signal[b][3] ^ Cb[b][1];
    }

    uint8_t SP[4];
    SP[0] = signal[0][0] ^ signal[0][1] ^ signal[1][0] ^ signal[1][1] ^ P[0];
    SP[1] = signal[2][0] ^ signal[2][1] ^ signal[3][0] ^ signal[3][1] ^ P[1];
    SP[2] = signal[0][2] ^ signal[0][3] ^ signal[1][2] ^ signal[1][3] ^ P[2];
    SP[3] = signal[2][2] ^ signal[2][3] ^ signal[3][2] ^ signal[3][3] ^ P[3];

    uint8_t SDi[4];
    SDi[0] = signal[0][0] ^ signal[1][1] ^ signal[2][0] ^ signal[3][1] ^ Di[0];
    SDi[1] = signal[0][1] ^ signal[1][0] ^ signal[2][1] ^ signal[3][0] ^ Di[1];
    SDi[2] = signal[0][2] ^ signal[1][3] ^ signal[2][2] ^ signal[3][3] ^ Di[2];
    SDi[3] = signal[0][3] ^ signal[1][2] ^ signal[2][3] ^ signal[3][2] ^ Di[3];

    int sumSL  = linsin[0][0]+linsin[0][1]+linsin[1][0]+linsin[1][1]+
                 linsin[2][0]+linsin[2][1]+linsin[3][0]+linsin[3][1];

    int sumSP  = SP[0]+SP[1]+SP[2]+SP[3];
    int sumSDi = SDi[0]+SDi[1]+SDi[2]+SDi[3];

    /* Verificacao das limitacoes do tbec */
    uint8_t flag = 0;

    /*Condição especial b  (SP=1, SDi=1, ΣSX≥2) */
    if (sumSP == 1 && sumSDi == 1 && sumSL >= 2)
    	flag |= 1;

    /*Mais de 3 bit-flips na mesma região*/
    if (sumSL > 3)      /* ΣSX conta quantos bits de dados acusaram erro */
        flag   |= 1;

    /* ---------- 3. Lógica de localização / correção ----------------------- */
    int anySP  = sumSP  > 0;
    int anySDi = sumSDi > 0;

    int cond_err = ( (anySP && anySDi) || (sumSL > 1) ) &&
                   !((sumSDi==0) && (sumSP==1) && (sumSL>=2));

    if (cond_err)
    {
        int quad1 = SP[0] + SP[1] + SDi[0] + SDi[1];
        int quad2 = SP[2] + SP[3] + SDi[2] + SDi[3];

        if (quad1 > quad2)                          /* primeiro quadrante */
        {
            for (int b = 0; b < 4; ++b)
                if (linsin[b][0] | linsin[b][1]) {
                    signal[b][0] ^= linsin[b][0];
                    signal[b][1] ^= linsin[b][1];
                }
        }
        else if (quad1 < quad2)                     /* segundo quadrante */
        {
            for (int b = 0; b < 4; ++b)
                if (linsin[b][0] | linsin[b][1]) {
                    signal[b][2] ^= linsin[b][0];
                    signal[b][3] ^= linsin[b][1];
                }
        }
        else if ( (quad1 == quad2) &&
                  (SP[0] | SP[1] | SDi[0] | SDi[1]) )   /* quadrante central */
        {
            for (int b = 0; b < 4; ++b)
                if (linsin[b][0] | linsin[b][1]) {
                    signal[b][1] ^= linsin[b][1];
                    signal[b][2] ^= linsin[b][0];
                }
        }
        else
        {
        	flag |= 1;
        }
    }

    /* ---------- 4. Reconstruir o payload de 16 bits ----------------------- */
    uint16_t payload = 0;
    payload |= signal[0][0] << 15;  payload |= signal[0][1] << 14;
    payload |= signal[0][2] << 13;  payload |= signal[0][3] << 12;

    payload |= signal[1][0] << 11;  payload |= signal[1][1] << 10;
    payload |= signal[1][2] <<  9;  payload |= signal[1][3] <<  8;

    payload |= signal[2][0] <<  7;  payload |= signal[2][1] <<  6;
    payload |= signal[2][2] <<  5;  payload |= signal[2][3] <<  4;

    payload |= signal[3][0] <<  3;  payload |= signal[3][1] <<  2;
    payload |= signal[3][2] <<  1;  payload |= signal[3][3] <<  0;

    ECC_DECODER parametros_ecc = {raw, payload, flag};
    return parametros_ecc;
}

/*
 * ******************************************************************************
 * Function Name: nascerr_eccs_mrsc_decoder
 * @brief       : Decodes a 32-bit MRSC (Multiple Redundant Source Coding) encoded word.
 *                It attempts to detect and correct errors within the data.
 *                The function extracts the original 16-bit data and provides
 *                information about the correction status.
 * @param       : data_in - The 32-bit MRSC encoded word to be decoded.
 * @retval      : An ECC_DECODER struct containing:
 *              - data_raw: The originally received 16-bit data before any correction.
 *              - data_corrected: The 16-bit data after applying error correction.
 *              - uncorrectable: A flag (1 if errors remain after correction, 0 otherwise).
 * ******************************************************************************
 */
ECC_DECODER nascerr_eccs_mrsc_decoder(uint32_t data_in)
{
    uint8_t  signal[4][4];
    uint8_t  linsin[4][2];
    uint8_t  SP[4], SDi[4];
    int      sumSL = 0, sumSP = 0, sumSDi = 0;
    uint8_t  err_quadr;
    int      quad1 = 0, quad2 = 0;

    /* 1) Carrega os 16 bits de dados (matriz 4×4) */
    signal[0][0] = (data_in >> 31) & 1u;
    signal[1][0] = (data_in >> 30) & 1u;
    signal[2][0] = (data_in >> 29) & 1u;
    signal[3][0] = (data_in >> 28) & 1u;

    signal[0][1] = (data_in >> 27) & 1u;
    signal[1][1] = (data_in >> 26) & 1u;
    signal[2][1] = (data_in >> 25) & 1u;
    signal[3][1] = (data_in >> 24) & 1u;

    signal[0][2] = (data_in >> 23) & 1u;
    signal[1][2] = (data_in >> 22) & 1u;
    signal[2][2] = (data_in >> 21) & 1u;
    signal[3][2] = (data_in >> 20) & 1u;

    signal[0][3] = (data_in >> 19) & 1u;
    signal[1][3] = (data_in >> 18) & 1u;
    signal[2][3] = (data_in >> 17) & 1u;
    signal[3][3] = (data_in >> 16) & 1u;

    uint16_t data_raw = 0;
    data_raw |= signal[0][0]<<15 | signal[0][1]<<14 | signal[0][2]<<13 | signal[0][3]<<12;
    data_raw |= signal[1][0]<<11 | signal[1][1]<<10 | signal[1][2]<< 9 | signal[1][3]<< 8;
    data_raw |= signal[2][0]<< 7 | signal[2][1]<< 6 | signal[2][2]<< 5 | signal[2][3]<< 4;
    data_raw |= signal[3][0]<< 3 | signal[3][1]<< 2 | signal[3][2]<< 1 | signal[3][3]<< 0;

    /* 2) Síndromes lineares por linha (linsin) */
    linsin[0][0] = signal[0][0] ^ signal[0][2] ^ ((data_in >> 7) & 1u);
    linsin[0][1] = signal[0][1] ^ signal[0][3] ^ ((data_in >> 6) & 1u);
    linsin[1][0] = signal[1][0] ^ signal[1][2] ^ ((data_in >> 5) & 1u);
    linsin[1][1] = signal[1][1] ^ signal[1][3] ^ ((data_in >> 4) & 1u);
    linsin[2][0] = signal[2][0] ^ signal[2][2] ^ ((data_in >> 3) & 1u);
    linsin[2][1] = signal[2][1] ^ signal[2][3] ^ ((data_in >> 2) & 1u);
    linsin[3][0] = signal[3][0] ^ signal[3][2] ^ ((data_in >> 1) & 1u);
    linsin[3][1] = signal[3][1] ^ signal[3][3] ^ ((data_in >> 0) & 1u);

    sumSL = linsin[0][0] + linsin[0][1]
          + linsin[1][0] + linsin[1][1]
          + linsin[2][0] + linsin[2][1]
          + linsin[3][0] + linsin[3][1];


    /* --- 3) Síndromes de paridade de colunas (SP) --- */
	SP[0] = (signal[0][0]^signal[1][0]^signal[2][0]^signal[3][0]) ^ ((data_in >> 11) & 1u);
	SP[1] = (signal[0][1]^signal[1][1]^signal[2][1]^signal[3][1]) ^ ((data_in >>  9) & 1u);
	SP[2] = (signal[0][2]^signal[1][2]^signal[2][2]^signal[3][2]) ^ ((data_in >>  8) & 1u);
	SP[3] = (signal[0][3]^signal[1][3]^signal[2][3]^signal[3][3]) ^ ((data_in >> 10) & 1u);
	for (int i = 0; i < 4; ++i) sumSP += SP[i];

	/* --- 4) Síndromes diagonais (SDi) --- */
	SDi[0] = (signal[0][0]^signal[1][1]^signal[2][0]^signal[3][1]) ^ ((data_in >> 15) & 1u);
	SDi[1] = (signal[0][1]^signal[1][0]^signal[2][1]^signal[3][0]) ^ ((data_in >> 13) & 1u);
	SDi[2] = (signal[0][2]^signal[1][3]^signal[2][2]^signal[3][3]) ^ ((data_in >> 12) & 1u);
	SDi[3] = (signal[0][3]^signal[1][2]^signal[2][3]^signal[3][2]) ^ ((data_in >> 14) & 1u);
	for (int i = 0; i < 4; ++i) sumSDi += SDi[i];

    /* 5) Detecta erro em quadrante */
    err_quadr = (((sumSP>0) && (sumSDi>0)) || (sumSL>1));

    /* 6) Prepara máscaras e hint de erro */
    uint16_t flip_mask = 0;

    /* 7) Localiza o quadrante e corrige os bits marcados em linsin */
    if (err_quadr) {
    	quad1 = SP[0]+SP[1]+SDi[0]+SDi[1];
		quad2 = SP[2]+SP[3]+SDi[2]+SDi[3];

		if (quad1 > quad2) {
			/* primeiro quadrante: corrige colunas 0–1 */
			for (int r = 0; r < 4; ++r) {
				if (linsin[r][0]) FLIP_BIT(r,0);
				if (linsin[r][1]) FLIP_BIT(r,1);
			}
		}
		else if (quad1 < quad2) {
			/* segundo quadrante: corrige colunas 2–3 */
			for (int r = 0; r < 4; ++r) {
				if (linsin[r][0]) FLIP_BIT(r,2);
				if (linsin[r][1]) FLIP_BIT(r,3);
			}
		}
		else if ((quad1 == quad2) && (SP[0]||SP[1]||SDi[0]||SDi[1])) {
			/* quadrante central: corrige colunas 1–2 (bits trocados) */
			for (int r = 0; r < 4; ++r) {
				if (linsin[r][1]) FLIP_BIT(r,1);
				if (linsin[r][0]) FLIP_BIT(r,2);
			}
		}

    }

    uint8_t sp_chk = 0, sdi_chk = 0, linsin_chk = 0;

    /* Recalcula SP  */
    sp_chk  = (signal[0][0]^signal[1][0]^signal[2][0]^signal[3][0]) ^ ((data_in>>11)&1u);
    sp_chk |= (signal[0][1]^signal[1][1]^signal[2][1]^signal[3][1]) ^ ((data_in>> 9)&1u);
    sp_chk |= (signal[0][2]^signal[1][2]^signal[2][2]^signal[3][2]) ^ ((data_in>> 8)&1u);
    sp_chk |= (signal[0][3]^signal[1][3]^signal[2][3]^signal[3][3]) ^ ((data_in>>10)&1u);

    /* Recalcula SDi */
    sdi_chk  = (signal[0][0]^signal[1][1]^signal[2][0]^signal[3][1]) ^ ((data_in>>15)&1u);
    sdi_chk |= (signal[0][1]^signal[1][0]^signal[2][1]^signal[3][0]) ^ ((data_in>>13)&1u);
    sdi_chk |= (signal[0][2]^signal[1][3]^signal[2][2]^signal[3][3]) ^ ((data_in>>12)&1u);
    sdi_chk |= (signal[0][3]^signal[1][2]^signal[2][3]^signal[3][2]) ^ ((data_in>>14)&1u);

    /* Recalcula linsin */
    for (int r=0; r<4; ++r){
        uint8_t p0 = (data_in>>(7-2*r)) & 1u;
        uint8_t p1 = (data_in>>(6-2*r)) & 1u;
        linsin_chk |= (signal[r][0]^signal[r][2]^p0);
        linsin_chk |= (signal[r][1]^signal[r][3]^p1);
    }

    /* 7.b) verificação pós-correção */
    /* flag final */
    uint8_t uncorrectable =
          (sp_chk | sdi_chk | linsin_chk)        /* sobrou síndrome   */
       || (err_quadr && flip_mask == 0);         /* detectou mas não flipou */

    /* 9) Empacota data_out corrigido */
    uint16_t data_out = 0;

    /* Linha 0: bits 15..12 */
    data_out |= (signal[0][0] & 1u) << 15;
    data_out |= (signal[0][1] & 1u) << 14;
    data_out |= (signal[0][2] & 1u) << 13;
    data_out |= (signal[0][3] & 1u) << 12;

    /* Linha 1: bits 11..8 */
    data_out |= (signal[1][0] & 1u) << 11;
    data_out |= (signal[1][1] & 1u) << 10;
    data_out |= (signal[1][2] & 1u) <<  9;
    data_out |= (signal[1][3] & 1u) <<  8;

    /* Linha 2: bits 7..4 */
    data_out |= (signal[2][0] & 1u) <<  7;
    data_out |= (signal[2][1] & 1u) <<  6;
    data_out |= (signal[2][2] & 1u) <<  5;
    data_out |= (signal[2][3] & 1u) <<  4;

    /* Linha 3: bits 3..0 */
    data_out |= (signal[3][0] & 1u) <<  3;
    data_out |= (signal[3][1] & 1u) <<  2;
    data_out |= (signal[3][2] & 1u) <<  1;
    data_out |= (signal[3][3] & 1u) <<  0;


    /* 10) Preenche o struct e retorna */
    ECC_DECODER res;
    res.data_raw = data_raw;      			/* palavra bruta (signal antes dos flips) */
    res.data_corrected = data_out;    		/* palavra após correção (signal depois)  */
    res.uncorrectable = uncorrectable; 		/* 1 se restou síndrome != 0 */
    return res;
}
