/**
 * ******************************************************************************
 *  @file    : NASCERR_Memory_16_to_32.c
 *  @brief   : Library for NASCERR memory-related functions, specifically for 16-bit
 *             to 32-bit memory operations. Provides functions for memory manipulation,
 *             including reading and writing to SRAM with error correction codes (ECC).
 *  @author  : Clara
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "NASCERR_Memory_32bits.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

volatile uint16_t * ptr = (uint16_t *)NASCERR_SRAM_ADDRESS;

uint8_t restore_robust_mem_info = false;

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: nascerr_inject_error_u16
 *  @brief       : Injects a single-bit error into a 16-bit data word based on
 *                 a specified address word and error configuration.
 *  @param addr_word : The address word used to determine if an error should be injected.
 *  @param data      : The original 16-bit data word.
 *  @retval          : The potentially modified 16-bit data word with an injected error.
 * ******************************************************************************
 */

uint16_t nascerr_inject_error_u16(uint32_t addr_word, uint16_t data)
{
#if ERROR_ENABLE
    if ((addr_word % ERROR_STRIDE_WORDS) == 0u) {
        data ^= (uint16_t)(1u << ERROR_BIT_TO_FLIP); // inverte 1 bit
    }
#endif
    return data;
}

/*
 * ******************************************************************************
 *  Function Name: print_bit_errors
 *  @brief       : Prints the positions of bit errors detected in a memory read
 *                 operation, along with the expected and read values.
 *  @param address   : The memory address where the error was detected.
 *  @param expected  : The expected data value.
 *  @param read_value: The actual data value read from memory.
 *  @param diff      : The difference mask indicating which bits are erroneous.
 *  @param mem_name  : The name of the memory being accessed (for logging purposes).
 *  @param num_bits  : The number of bits in the data word (16 or 32).
 *  @param ecc_result: The result of the ecc corrector
 *  @retval          : None
 * ******************************************************************************
 */
void print_bit_errors(uint32_t address, uint32_t expected, uint32_t read_value, uint32_t diff, const char *mem_name, int num_bits, int8_t ecc_result)
{
#if ENABLE_ERROR_SRAM_DEBUG
    int bits[32];
    int count = 0;

    for (int bit = 0; bit < num_bits; bit++) {
        if ((diff >> bit) & 1) {
            bits[count++] = bit;
        }
    }

    if (count == 0)
        return;

    printf("REC: POSIÇÃO(ÕES) DO(S) BIT(S) ERRADO(S) NO ENDEREÇO 0x%08" PRIX32 " DA MEMÓRIA %s: ",
           address, mem_name);

    for (int i = 0; i < count; i++) {
        printf("%d", bits[i]);
        if (i < count - 1)
            printf(", ");
    }

    if (num_bits == 16)
        printf(" PADRÃO: 0x%04X, LIDO DA MEMÓRIA %s: 0x%04X\r\n",
               (uint16_t)expected, mem_name, (uint16_t)read_value);
    else
    	printf(" PADRÃO: 0x%08" PRIX32 ", LIDO DA MEMÓRIA %s: 0x%08" PRIX32 "\r\n",
    	       expected, mem_name, read_value);

    if (ecc_result == 0)  		printf(" [ECC: CORRIGIDO]\r\n");
    else if (ecc_result == 1) 	printf(" [ECC: NAO CORRIGIDO]\r\n");

#endif
}

/*
 * ******************************************************************************
 *  Function Name: print_bit_errors
 *  @brief       : Prints the positions of bit errors detected in a memory read
 *                 operation, along with the expected and read values.
 *  @param address   : The memory address where the error was detected.
 *  @param expected  : The expected data value.
 *  @param read_value: The actual data value read from memory.
 *  @param diff      : The difference mask indicating which bits are erroneous.
 *  @param mem_name  : The name of the memory being accessed (for logging purposes).
 *  @param num_bits  : The number of bits in the data word (16 or 32).
 *  @param ecc_result: The result of the ecc corrector
 *  @retval          : None
 * ******************************************************************************
 */
void print_bit_errors_ecc(uint32_t address, uint32_t expected, uint32_t read_value, uint32_t diff, const char *mem_name, int num_bits, int8_t ecc_result, uint16_t data_expected, uint16_t data_actual)
{
#if ENABLE_ERROR_SRAM_DEBUG
	int bits[32];
	int count = 0;

	for (int bit = 0; bit < num_bits; bit++) {
		if ((diff >> bit) & 1) {
			bits[count++] = bit;
		}
	}

	if (count == 0)
		return;

	printf("REC: POSIÇÃO(ÕES) DO(S) BIT(S) ERRADO(S) NO ENDEREÇO 0x%08" PRIX32 " DA MEMÓRIA %s: ",
		   address, mem_name);

	for (int i = 0; i < count; i++) {
		printf("%d", bits[i]);
		if (i < count - 1)
			printf(", ");
	}

	if (num_bits == 16)
		printf(" PADRÃO: 0x%04X, LIDO DA MEMÓRIA %s: 0x%04X ",
			   (uint16_t)expected, mem_name, (uint16_t)read_value);
	else
		printf(" PADRÃO: 0x%08" PRIX32 ", LIDO DA MEMÓRIA %s: 0x%08" PRIX32 " ",
			   expected, mem_name, read_value);

	if (ecc_result == 0) {
		printf(" [ECC: CORRIGIDO]\r\n");
	}
	else if (ecc_result == 1){
		printf(" [ECC: NAO CORRIGIDO] ");
		uint16_t data_diff = data_expected ^ data_actual;
				if (data_diff) {
					printf(" | DADO ENTREGUE DIVERGE: PADRAO=0x%04X, ENTREGUE=0x%04X\r\n", data_expected, data_actual);
				}
		}
#endif
}



/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_write_sram_block
 *  @brief       : Fills a buffer with a specified data pattern, then writes the buffer
 *                 as a block to consecutive memory cells, applying the selected ECC.
 *  @param write_buffer : Pointer to the buffer that will be filled and written to memory.
 *  @param offset_addr : Starting offset address for the block write operation.
 *  @param ecc   : ECC mode to apply to the data block.
 *  @param write_type : Type of data pattern to be used in filling the buffer.
 *  @param length : Number of data words in the block to be written.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_write_sram_block(W_DataTypeDef write_type, uint32_t offset_addr, NASCERR_MODE ecc, uint32_t length)
{
	uint16_t data = nascerr_write_type(write_type);
	nascerr_memory_write_block_cell(data, ecc, offset_addr, length);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_write_block_cell
 *  @brief       : Writes a block of data to consecutive memory cells, starting from
 *                 a specified address and applying a selected ECC configuration.
 *  @param write_buffer : Pointer to buffer containing data to be written.
 *  @param ecc   : ECC mode to be applied for the write operation.
 *  @param offset_addr : Starting offset address in memory for the block write.
 *  @param length : Number of data words to be written from the buffer.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_write_block_cell(uint16_t data, NASCERR_MODE ecc, uint32_t offset_addr, uint32_t length)
{
	if(((uint32_t)ptr + offset_addr + length) <= NASCERR_SRAM_END)
	{
		if(ecc != DUAL_BYPASS) {
			/* 1. Escreve normalmente pelo codificador — FPGA gera a codeword completa */
			nascerr_select_ecc(ecc);
			for(uint32_t i = 0; i < length; i++) {
				*(ptr + offset_addr + i) = data;
			}

#if ERROR_ENABLE
			/* 2. Injeção pós-codificação: lê codeword via bypass, inverte bits desejados e grava de volta */
			for(uint32_t i = 0; i < length; i++) {
				if ((i % ERROR_STRIDE_WORDS) != 0u) continue;

				/* Lê dado (BYPASS_1=MSB [31:16]) e ECC (BYPASS_2=LSB [15:0]) */
				nascerr_select_ecc(BYPASS_1);
				uint16_t bp1 = *(ptr + offset_addr + i);

				nascerr_select_ecc(BYPASS_2);
				uint16_t bp2 = *(ptr + offset_addr + i);

				/* Monta a codeword de 32 bits e aplica o padrão de flip */
				uint32_t codeword = ((uint32_t)bp1 << 16) | bp2;
				codeword ^= ERROR_CODEWORD_FLIP_MASK;

				/* Grava codeword corrompida de volta via bypass */
				nascerr_select_ecc(BYPASS_1);
				*(ptr + offset_addr + i) = (uint16_t)(codeword >> 16);

				nascerr_select_ecc(BYPASS_2);
				*(ptr + offset_addr + i) = (uint16_t)(codeword & 0xFFFFu);
			}
#endif
		}
		else {
			/* DUAL_BYPASS: escrita direta nas duas memórias sem codificador */
			nascerr_select_ecc(BYPASS_1);
			for(uint32_t i = 0; i < length; i++) {
				*(ptr + offset_addr + i) = data;
			}
			nascerr_select_ecc(BYPASS_2);
			for (uint32_t i = 0; i < length; i++) {
				*(ptr + offset_addr + i) = data;
			}
		}
	}
	else{
		DEBUG_PRINT(LOG_ERROR, "[FMC] OVERFLOW: escrita interrompida fora da regiao SRAM (offset=0x%08lX, len=%lu)\r\n", offset_addr, length);
	}
}
/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_write_image_buffer
 *  @brief       : Writes an arbitrary buffer of 16-bit pixel words to consecutive
 *                 SRAM cells under the selected ECC mode. Unlike write_block_cell,
 *                 which fills memory with a single repeated value, this function
 *                 writes each word individually from the provided buffer, enabling
 *                 image data (non-uniform pixel values) to be stored for radiation
 *                 experiment testing.
 *  @param buffer     : Pointer to the source buffer of 16-bit words.
 *  @param ecc        : ECC mode to apply during the write.
 *  @param offset_addr: Starting word offset from NASCERR_SRAM_ADDRESS.
 *  @param length     : Number of words to write.
 *  @retval           : None
 * ******************************************************************************
 */
void nascerr_memory_write_image_buffer(uint16_t *buffer, NASCERR_MODE ecc, uint32_t offset_addr, uint32_t length)
{
	if (((uint32_t)(ptr + offset_addr + length)) > NASCERR_SRAM_END)
	{
		DEBUG_PRINT(LOG_ERROR, "[FMC] OVERFLOW: escrita de imagem fora da regiao SRAM (offset=0x%08lX, len=%lu)\r\n", offset_addr, length);
		return;
	}
	nascerr_select_ecc(ecc);
	for (uint32_t i = 0; i < length; i++)
	{
		*(ptr + offset_addr + i) = buffer[i];
	}
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_inject_postenc_errors
 *  @brief       : Post-encoding error injection for ECC mode testing.
 *                 Sequence per word:
 *                   1. Select BYPASS_1 (raw access to data bank, no FPGA codec)
 *                   2. Read the current 16-bit raw value
 *                   3. Flip ERROR_BIT_TO_FLIP
 *                   4. Write the corrupted value back through BYPASS_1
 *                 The codeword stored in SRAM now has a real single-bit error
 *                 in the data bank — exactly what the ECC decoder is meant to
 *                 catch and correct on read.
 *  @param length: Number of words from offset 0 to scan for injection points.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_memory_inject_postenc_errors(uint32_t length)
{
#if ERROR_ENABLE
    if (length == 0u) return;

    for (uint32_t i = 0; i < length; i++) {
        if ((i % ERROR_STRIDE_WORDS) != 0u) continue;

        /* Lê dado (BYPASS_1=MSB [31:16]) e ECC (BYPASS_2=LSB [15:0]) */
        nascerr_select_ecc(BYPASS_1);
        uint16_t bp1 = *(ptr + i);

        nascerr_select_ecc(BYPASS_2);
        uint16_t bp2 = *(ptr + i);

        /* Monta a codeword de 32 bits e aplica o padrão de flip */
        uint32_t codeword = ((uint32_t)bp1 << 16) | bp2;
        codeword ^= ERROR_CODEWORD_FLIP_MASK;

        /* Grava codeword corrompida de volta via bypass */
        nascerr_select_ecc(BYPASS_1);
        *(ptr + i) = (uint16_t)(codeword >> 16);

        nascerr_select_ecc(BYPASS_2);
        *(ptr + i) = (uint16_t)(codeword & 0xFFFFu);
    }
#endif
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_write_type
 *  @brief       : Converts a specified write type to its corresponding 16-bit value.
 *  @param write_type : Type of data pattern to be converted.
 *  @retval      : 16-bit value corresponding to the specified write type.
 * ******************************************************************************
 */
uint16_t nascerr_write_type(W_DataTypeDef write_type)
{
	uint16_t data;
	switch(write_type){

	case WRITE_ZEROS:
		data = 0x0000;
		break;

	case WRITE_ONES:
		data = 0xFFFF;
		break;

	case WRITE_x55h:
		data = 0x5555;
		break;

	case WRITE_xAAh:
		data = 0xAAAA;
		break;

	default:
		break;
	}
	return data;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_sram_init
 *  @brief     : Initialize the NASCERR SRAM by erasing both banks and verifying
 *  @param     : None
 *  @retval    : uint8_t
 *               1: Initialization and verification succeeded
 *               0: Initialization or verification failed
 * ******************************************************************************
 */
uint8_t nascerr_sram_init(void)
{
	nascerr_erase_sram(BOTH);
	return nascerr_verify_sram();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_verify_sram
 *  @brief     : Verify that all locations in both SRAM banks read back as erased (0xFFFF)
 *  @param     : None
 *  @retval    : uint8_t
 *               1: All half-words in both banks are 0xFFFF (success)
 *               0: A half-word differs from 0xFFFF (failure)
 * ******************************************************************************
 */
uint8_t nascerr_verify_sram(void)
{
    volatile uint16_t *p       = (volatile uint16_t*)NASCERR_SRAM_ADDRESS;
    volatile uint16_t *p_end   = (volatile uint16_t*)NASCERR_SRAM_END;

	nascerr_select_ecc(BYPASS_1);
    for (; p < p_end; ++p) {
        if (*p != 0xFFFFu)
            return false;
    }

    p = (volatile uint16_t*)NASCERR_SRAM_ADDRESS;

    nascerr_select_ecc(BYPASS_2);
    for (; p < p_end; ++p) {
        if (*p != 0xFFFFu)
            return false;
    }

	return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_erase_sram
 *  @brief     : Erase one or both SRAM banks by writing 0xFFFF to every half-word
 *  @param     : MemorySelect memory
 *               MEMORY_1: erase bank 1 only
 *               MEMORY_2: erase bank 2 only
 *               BOTH    : erase both bank 1 and bank 2
 *  @retval    : uint8_t
 *               1: Erase operation completed successfully
 *               0: Invalid memory selection parameter
 * ******************************************************************************
 */
uint8_t nascerr_erase_sram(MemorySelect memory)
{
    volatile uint16_t *p       = (volatile uint16_t*)NASCERR_SRAM_ADDRESS;
    volatile uint16_t *p_end   = (volatile uint16_t*)NASCERR_SRAM_END;

	switch(memory)
	{
	case MEMORY_1:
		nascerr_select_ecc(BYPASS_1);
		for(; p < p_end; ++p)
			*(p) = 0xFFFF;

		break;

	case MEMORY_2:
		nascerr_select_ecc(BYPASS_2);
		for(; p < p_end; ++p)
			*(p) = 0xFFFF;

		break;

	case BOTH:
		nascerr_select_ecc(BYPASS_1);
		for(; p < p_end; ++p)
			*(p) = 0xFFFF;

		nascerr_select_ecc(BYPASS_2);
		p = (volatile uint16_t*)NASCERR_SRAM_ADDRESS;
		for(; p < p_end; ++p)
			*(p) = 0xFFFF;

		break;

	default:
		return false;
		break;
	}

	return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_read
 *  @brief       : Reads a single memory cell at a given offset address.
 *  @param       : offset_addr - Offset address (in 16-bit words) from the base
 *                                SRAM address.
 *  @retval      : Pointer to the 16-bit memory location. Returns NULL if the
 *                 offset is out of range.
 * ******************************************************************************
 */
uint16_t *nascerr_memory_read(uint32_t offset_addr)
{
    uint32_t addr = NASCERR_SRAM_ADDRESS + (offset_addr*2); 			//addr convertido em bytes
    if (NASCERR_SRAM_ADDRESS + offset_addr >= NASCERR_SRAM_END) { 		//verifição por palavras
        return NULL;
    }
    return (uint16_t *)(addr);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_memory_read_block
 *  @brief       : Reads a block of memory words from SRAM, using the specified
 *                 ECC mode. The ECC mode is applied before accessing memory.
 *  @param       : offset_addr - Offset address (in 16-bit words) from the base
 *                                SRAM address.
 *  @param       : ecc - ECC mode to be selected before reading (BYPASS_1,
 *                       BYPASS_2, TBEC, MRSC, etc).
 *  @retval      : Pointer to the beginning of the 16-bit memory block.
 * ******************************************************************************
 */
uint16_t *nascerr_memory_read_block(uint32_t offset_addr, NASCERR_MODE ecc)
{
	nascerr_select_ecc(ecc);
	return nascerr_memory_read(offset_addr);
}

/**
 * ******************************************************************************
 * @brief  Testa os encoders no padrão de escrita TBEC e MRSC em regioes
 * aleatorias da memoria.
 * @note   Executa testes de escrita/leitura e compara resultados.
 * @param : void
 * @retval: Robust_Memory_Health contendo o registro do teste.
 * ******************************************************************************
 */
Robust_Memory_Health nascerr_memory_encoder_decoders_selfcheck(void)
{
	Robust_Memory_Health robust_mem_health;

	memcpy(&robust_mem_health, g_register_data[ROBUST_MEM_HEALTH].value, sizeof(Robust_Memory_Health));

    const W_DataTypeDef patterns[2] = {
        WRITE_xAAh, WRITE_x55h
    };
    const NASCERR_MODE modes[2] = {
    	TBEC, MRSC
    };

    const uint32_t total_words = (NASCERR_SRAM_END - NASCERR_SRAM_ADDRESS); //logica já indexada em palavras

//    DEBUG_PRINT(LOG_INFO, "[MEM SELFTEST] Iniciando verificacao dos encoders/decoders...\r\n");
//    DEBUG_PRINT(LOG_INFO, "[MEM SELFTEST] Amostragem de %lu palavras\r\n", MEMORY_TEST_FRACTION);

    uint32_t errors_per_mode[2] = {0, 0};
    uint32_t errors_total = 0, total_checked = 0;

    for (uint8_t m = 0; m < 2; m++)
    {
        NASCERR_MODE ecc = modes[m];
        nascerr_select_ecc(ecc);

       for (uint8_t p = 0; p < 2; p++)
        {
            uint16_t data_ref = nascerr_write_type(patterns[p]);

            for (uint32_t k = 0; k < MEMORY_TEST_FRACTION; k++)
            {
            	uint32_t rand_index = (uint32_t)(((uint64_t)rand() * total_words) / (RAND_MAX + 1ULL));

				uint8_t  attempts = 0;
				uint16_t val;

				*(ptr + rand_index) = data_ref;

				do
				{
					if (nascerr_registers_getStatusSystem().OP.EXP == OP_EXP_RUNNING)
					{
						DEBUG_PRINT(LOG_WARN, "[ENC/DEC SELFTEST] Abortado: experimento ativo\r\n");
						goto finalize;
					}

					val = *(ptr + rand_index);
					attempts++;

				} while ((val != data_ref) && (attempts < 2)); // Single trasient event could've happened

                total_checked++;

				if (val != data_ref)
				{
					errors_total++;
					errors_per_mode[m]++;
				}
            }
        }
    }

    finalize:
		robust_mem_health.enc_dec_health.errors_tbec = (errors_per_mode[0] > (uint32_t)(0xFFFFu - robust_mem_health.enc_dec_health.errors_tbec)) ?
				0xFFFFu : (uint16_t)(robust_mem_health.enc_dec_health.errors_tbec + errors_per_mode[0]);

		robust_mem_health.enc_dec_health.errors_mrsc = (errors_per_mode[1] > (uint32_t)(0xFFFFu - robust_mem_health.enc_dec_health.errors_mrsc)) ?
				0xFFFFu : (uint16_t)(robust_mem_health.enc_dec_health.errors_mrsc + errors_per_mode[1]);

		/* Total de testes = palavras verificadas */
		robust_mem_health.enc_dec_health.total_tests_run = (total_checked > (uint32_t)(0xFFFFFFFFu - robust_mem_health.enc_dec_health.total_tests_run)) ?
				0xFFFFFFFFu : (robust_mem_health.enc_dec_health.total_tests_run + total_checked);

		memcpy(g_register_data[ROBUST_MEM_HEALTH].value, &robust_mem_health, sizeof(Encoder_Decoder_Health));

		memcpy(&robust_mem_health, g_register_data[ROBUST_MEM_HEALTH].value, sizeof(Robust_Memory_Health));

    return robust_mem_health;
}

/**
 * ******************************************************************************
 * @brief  Escreve toda a memoria no modo bypass 1 e 2. Vefirica se houve um latch
 * @note   Executa testes de escrita/leitura e compara resultados.
 * @param : void
 * @retval: Robust_Memory_Health contendo o registro do teste.
 * ******************************************************************************
 */
Robust_Memory_Health nascerr_memory_complete_selfcheck(void)
{
	Robust_Memory_Health robust_mem_health = {0};

    volatile uint16_t *p_end   = (volatile uint16_t*)NASCERR_SRAM_END;

    const W_DataTypeDef patterns[2] = {
            WRITE_xAAh, WRITE_x55h
        };

    const NASCERR_MODE modes_test[2] = {
    		BYPASS_1, BYPASS_2
    	};

    uint32_t errors_per_mode[2] = {0, 0}; // only test bypass1 and bypass2
	uint32_t errors_total = 0, total_checked = 0;

	for(uint8_t i = 0; i < 2; i++)
	{
		nascerr_select_ecc(modes_test[i]);
		for(uint8_t j = 0; j < 2; j++)
		{
			volatile uint16_t *p = (volatile uint16_t*)NASCERR_SRAM_ADDRESS;

			uint16_t data_ref = nascerr_write_type(patterns[i]);
			for(; p < p_end; ++p)
			{
				uint8_t attempts = 0;
				uint16_t val;

				do
				{
					*(p) = data_ref;
					val = *(p);
					attempts++;

					if(nascerr_registers_getStatusSystem().OP.EXP == OP_EXP_RUNNING)
					{
						 DEBUG_PRINT(LOG_WARN, "[MEM SELFTEST] Abortado: experimento ativo\r\n");
						 goto finalize;
					}
				} while((val != data_ref) && (attempts < 2));

				total_checked++;

				if (val != data_ref)
				{
					errors_total++;
					errors_per_mode[j]++;

					uintptr_t addr   = (uintptr_t)p; // gets the address being pointed to
					uintptr_t offset = addr - NASCERR_SRAM_ADDRESS; // mappes to the correct offset
					uint8_t block = offset >> BLOCK_SHIFT; // mappes to the corrupted block

					if (block < 64)
					{
						if(i == 0)
						{
							robust_mem_health.srams_health.mem_1_affected_blocks |= (1ULL << block); // sets the mapped block as corrupted
						}
						else
						{
							robust_mem_health.srams_health.mem_2_affected_blocks |= (1ULL << block); // sets the mapped block as corrupted
						}
					}
				}
			}
		}
	}

	finalize:
	robust_mem_health.srams_health.errors_mem_1 = (errors_per_mode[0] > (uint32_t)(0xFFFFu - robust_mem_health.srams_health.errors_mem_1)) ?
			0xFFFFu : (uint16_t)(robust_mem_health.srams_health.errors_mem_1 + errors_per_mode[0]);

	robust_mem_health.srams_health.errors_mem_2 = (errors_per_mode[1] > (uint32_t)(0xFFFFu - robust_mem_health.srams_health.errors_mem_2)) ?
			0xFFFFu : (uint16_t)(robust_mem_health.srams_health.errors_mem_2 + errors_per_mode[1]);


	memcpy(g_register_data[ROBUST_MEM_HEALTH].value + 8, &robust_mem_health.srams_health, sizeof(SRAMs_Health));

	memcpy(&robust_mem_health, g_register_data[ROBUST_MEM_HEALTH].value, sizeof(Robust_Memory_Health));

	uint8_t health_status = (errors_total == 0) ? false : true;

	nascerr_memory_alert_block_corruption(health_status); // sets the system flag if any block is corrupted

	return robust_mem_health;
}

/*
 * ******************************************************************************
 *  Function Name : nascerr_memory_setCorruptedBlocks
 *  @brief        : Updates the corrupted memory info
 *  @param        : uint8_t corrupted_block_flag - indicates if a block was
 *  corrupted
 *  @retval       : none
 * ******************************************************************************
 */
void nascerr_memory_alert_block_corruption(uint8_t corrupted_block_flag)
{
	if(corrupted_block_flag){
		g_register_data[STAT].value[4] |= 1U << 6;
	}
	else{
		g_register_data[STAT].value[4] &= ~(1U << 6);
	}
	return;
}

/*
 * ******************************************************************************
 *  Function Name : nascerr_memory_clear_health_info
 *  @brief        : Clears the corrupted memory info
 *  @param        : none
 *  @retval       : none
 * ******************************************************************************
 */
void nascerr_memory_clear_health_info(void)
{
	for(uint8_t i = 0; i < g_register_data[ROBUST_MEM_HEALTH].size - 2; i++)
	{
		g_register_data[ROBUST_MEM_HEALTH].value[i] = 0;
	}
}
