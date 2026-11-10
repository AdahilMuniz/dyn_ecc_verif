/**
 * *******************************************************************************
 * @file    NASCERR_Experiment.c
 * @brief   NASCERR communication protocol: block allocation, SRAM write/read,
 *          and ECC/bypass error metrics collection.
 * @author  Cicero Oliveira, Joao Manoel
 * *******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */
#include "NASCERR_Experiment.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */
extern IWDG_HandleTypeDef hiwdg;

static uint16_t *read;                   /**< Ponteiro para leitura de dados SRAM */

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

uint16_t contar_bits_setados(uint16_t valor);
void nascerr_count_region(ECC_Telemetry_t *telemetry, uint16_t diff);
ECC_Telemetry_t nascerr_ecc_avaliation(NASCERR_MODE ecc, uint32_t length, W_DataTypeDef write_type, uint32_t offset_addr);
ECC_Telemetry_t nascerr_bypass_avaliation(NASCERR_MODE ecc, uint32_t length, W_DataTypeDef write_type, uint32_t offset_addr);
uint8_t nascerr_check_command(NASCERR_SCHEDULER command);
uint32_t nascerr_block_to_page(uint8_t block);
uint32_t nascerr_page_to_words(uint16_t page);

/*
 * ******************************************************************************
 *  Function Name: nascerr_experiment_init
 *  @brief     : Write blocks of words to SRAM at the address derived from the
 *               start block index specified in command.
 *  @param     : command - NASCERR_CTRL structure containing write parameters
 *  @retval    : Operation status (uint8_t: 1 = success, 0 = failure)
 * ******************************************************************************
 */
uint8_t nascerr_experiment_init(NASCERR_SCHEDULER command)
{
	if(nascerr_check_command(command))
	{
		uint32_t offset_page = nascerr_block_to_page(command.block);
		uint32_t offset_addr = nascerr_page_to_words(offset_page);

		uint32_t num_pages =  NUM_PAGES_PER_BLOCK; //* command.qtd_block;
		uint32_t data_words = nascerr_page_to_words(num_pages);

		for(uint8_t i = 0; i < command.qtd_block; i++)
		{
			nascerr_memory_write_sram_block(command.write_type, offset_addr, command.ecc, data_words);
			offset_addr += data_words;
			i2c_slave_noBlockBus();
		}
//		printf("Experimento id %u escrito com sucesso\r\n", command.id);
		return true;
	}
	return false;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_experiment_read_result
 *  @brief     : Read a data block from memory and return its ECC metrics.
 *  @param     : command - NASCERR_CTRL structure containing write parameters
 *  @retval    : ECC metrics for the block (NASCERR_ECC_METRICS)
 * ******************************************************************************
 */
ECC_Telemetry_t nascerr_experiment_read_result(NASCERR_SCHEDULER command)
{
	ECC_Telemetry_t res = {0, 0, 0, 0, 0, 0, 0x7fff, 0, ECC_ERR_UNKNOWN};
	if(!(nascerr_check_command(command)))
		return res;

	uint32_t offset_page = nascerr_block_to_page(command.block);
	uint32_t offset_addr = nascerr_page_to_words(offset_page);

	uint32_t num_pages =  NUM_PAGES_PER_BLOCK;
	uint32_t data_words = nascerr_page_to_words(num_pages);

	read = nascerr_memory_read_block(offset_addr, command.ecc);

	if (command.ecc == TBEC || command.ecc == MRSC)
		res = nascerr_ecc_avaliation(command.ecc, data_words, command.write_type, (uint32_t)read);
	else
		res = nascerr_bypass_avaliation(command.ecc, data_words, command.write_type, (uint32_t)read);

//	printf("experimento id %u lido com sucesso | ecc_status = %d \r\n", command.id, res.ecc_status);
//	if (res.ecc_status != ECC_OK)
//	{
//		if(res.ecc_status == ECC_WARN) TODO: COMENTEI PARA TESTE
//			DEBUG_PRINT(LOG_INFO, "ECC_WARN \r\n");
//		else if (res.ecc_status == ECC_FAIL)
//			DEBUG_PRINT(LOG_INFO, "ECC_FAIL \r\n");
//		else
//			DEBUG_PRINT(LOG_INFO, "ECC_ERR_UNKNOWN \r\n");
//	}
	i2c_slave_noBlockBus();
	return res;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_experiment_number_errors
 *  @brief     : Assembles all error that have been correcteded or not
 *  @param     : buffer_read - ECC_Telemetry_t structure containing the errors
 *  @retval    : nascerr_data - Quantity of errors
 * ******************************************************************************
 */
nascerr_data nascerr_experiment_number_errors(ECC_Telemetry_t buffer_read)
{
	return (
			buffer_read.single_bit_errors +
			buffer_read.double_bit_errors +
			buffer_read.triple_bit_errors);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_experiment_number_corrected
 *  @brief     : Assembles all error that are correctable
 *  @param     : buffer_read - ECC_Telemetry_t structure containing the errors
 *  @retval    : nascerr_data - Quantity of correctable errors
 * ******************************************************************************
 */
nascerr_data nascerr_experiment_number_corrected(ECC_Telemetry_t buffer_read)
{
	return (buffer_read.single_bit_errors +
			buffer_read.double_bit_errors +
			buffer_read.triple_bit_errors -
			buffer_read.uncorrectable_errors);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_experiment_synthesise_error_info
 *  @brief     : Calculates the percent of number of error by the number of
 *  corrected errors in a experiment read cycle
 *  @param     : number_of_errors - total quantity of errors
 *  @param 	   : number_of_corrected - total quantity of corrected errors
 *  @retval    : float - percent of number of error by the number of
 *  corrected errors
 * ******************************************************************************
 */
float nascerr_experiment_synthesise_error_info(nascerr_data number_of_errors, nascerr_data number_of_corrected)
{
	if (number_of_errors == 0) {
	        return 0.0f;
	}

	// Se houve erro mas nada foi corrigido => falha total
	if (number_of_corrected == 0 && number_of_errors > 0) {
		return 100.0f;
	}

	// Fração de erros que ficaram sem corrigir
	int uncorrected = number_of_errors - number_of_corrected;
	if (uncorrected < 0) uncorrected = 0; // segurança

	return ((float)uncorrected / number_of_errors) * 100.0f;
}

/*
 * ******************************************************************************
 *  Local Functions Definitions
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: contar_bits_setados
 *  @brief       : Counts how many bits are set to 1 in a 16-bit value.
 *  @param       : valor - 16-bit word to examine.
 *  @retval      : uint16_t - Number of set bits.
 * ******************************************************************************
 */
uint16_t contar_bits_setados(uint16_t valor) {
    uint16_t count = 0;
    while (valor) {
        valor &= (valor - 1);
        count++;
    }
    return count;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_count_region
 *  @brief       : Accumulates region-based error counters given a diff mask
 *                 (XOR between read data and expected data). The function
 *                 increments beg/mid/end region fields inside telemetry.
 *  @param       : telemetry - Pointer to ECC_Telemetry_t structure to update.
 *  @param       : diff      - XOR mask indicating bit positions with errors.
 *  @retval      : None
 * ******************************************************************************
 */
void nascerr_count_region(ECC_Telemetry_t *telemetry, uint16_t diff)
{
	for (int bit = 15; bit >= 0; --bit) {
		if ((diff >> bit) & 1) {
			int coluna = (15 - bit) % 4;

			if (coluna == 0 || coluna == 1)
				telemetry->beg_region_errors++;

			if (coluna == 2 || coluna == 3)
				telemetry->end_region_errors++;

			if (coluna == 1 || coluna == 2)
				telemetry->mid_region_errors++;
		}
	}
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_ecc_avaliation
 *  @brief       : Evaluates ECC (TBEC or MRSC) over a range of words, comparing
 *                 decoded payloads with the written pattern and accumulating
 *                 single/double/triple-bit and region-based error metrics.
 *  @param       : ecc        - ECC mode (TBEC or MRSC).
 *  @param       : length     - Number of 16-bit words to process.
 *  @param       : write_type - Data pattern written (W_DataTypeDef).
 *  @param       : offset_addr - Starting offset address in memory (for logging).
 *  @retval      : ECC_Telemetry_t - Telemetry with error counters and status.
 * ******************************************************************************
 */
ECC_Telemetry_t nascerr_ecc_avaliation(NASCERR_MODE ecc, uint32_t length, W_DataTypeDef write_type, uint32_t offset_addr)
{
	ECC_Telemetry_t res = {0};
    res.ecc = ecc;

    uint16_t data = nascerr_write_type(write_type);
    ECC_DECODER decoded;
    uint32_t single_bit = 0, double_bit = 0, triple_bit = 0, uncorrected = 0;

    uint16_t MSB, LSB;
    uint32_t encoded;
    uint16_t bit_error_count;
    uint16_t fpga_decoded;
    uint16_t unmatch = 0;

    for (uint32_t i = 0; i < length; i++) {
    	i2c_slave_noBlockBus();
        nascerr_select_ecc(BYPASS_1);
        MSB = read[i];

        nascerr_select_ecc(BYPASS_2);
        LSB = read[i];

        nascerr_select_ecc(ecc);
        fpga_decoded = read[i];

        encoded = (((uint32_t)MSB << 16) | LSB);
        decoded = nascerr_ecc_decode(encoded, ecc);

        uint16_t diff = decoded.data_raw ^ data;
        nascerr_count_region(&res, diff);

        uint32_t expected_codeword = nascerr_ecc_encode(data, ecc);

        uint32_t diff_codeword = expected_codeword ^ encoded;

        if(diff_codeword){
        	int8_t ecc_result = (data == fpga_decoded) ? 0 : 1;
        	print_bit_errors_ecc(offset_addr + i, expected_codeword, encoded, diff_codeword, "ECC RAW", 32, ecc_result, data, fpga_decoded);
        }

		if(decoded.data_corrected != fpga_decoded)
			unmatch++;

        if (decoded.uncorrectable)
            uncorrected++;

        bit_error_count = contar_bits_setados(decoded.data_raw ^ data);

        if (bit_error_count == 1)
            single_bit++;
        else if (bit_error_count == 2)
            double_bit++;
        else if (bit_error_count >= 3)
            triple_bit++;
    }

    res.single_bit_errors = single_bit;
    res.double_bit_errors = double_bit;
    res.triple_bit_errors = triple_bit;
    res.uncorrectable_errors = uncorrected;

    if(unmatch > 0){
    	res.ecc_status = ECC_ERR_UNKNOWN;
    	//DEBUG_PRINT(LOG_INFO, "ECC_ERR_UNKNOWN");
    }
    else if (uncorrected > 0){
        res.ecc_status = ECC_FAIL;
        //DEBUG_PRINT(LOG_INFO, "ECC_FAIL");

    }
    else if (single_bit || double_bit || triple_bit){
        res.ecc_status = ECC_WARN;
        //DEBUG_PRINT(LOG_INFO, "ECC_WARN");
    }
    else{
        res.ecc_status = ECC_OK;
        //DEBUG_PRINT(LOG_INFO, "ECC_OK");
    }
    return res;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_bypass_avaliation
 *  @brief       : Evaluates error metrics in bypass operation (no ECC). Works
 *                 for single-channel (BYPASS_1/BYPASS_2) or dual-channel modes,
 *                 counting single/double/triple-bit and region-based errors.
 *  @param       : ecc        - Mode indicator (BYPASS_1, BYPASS_2, or dual).
 *  @param       : length     - Number of 16-bit words to process.
 *  @param       : write_type - Data pattern written (W_DataTypeDef).
 *  @param       : offset_addr - Starting offset address in memory (for logging).
 *  @retval      : ECC_Telemetry_t - Telemetry with error counters and status.
 * ******************************************************************************
 */
ECC_Telemetry_t nascerr_bypass_avaliation(NASCERR_MODE ecc, uint32_t length, W_DataTypeDef write_type, uint32_t offset_addr)
{

	ECC_Telemetry_t res = {0};
	res.ecc = ecc;

	uint16_t data = nascerr_write_type(write_type);

	uint32_t single_bit = 0, double_bit = 0, triple_bit = 0;
	uint16_t bit_error_count;

	if((ecc == BYPASS_1) || (ecc == BYPASS_2))
	{
		const char *mem_name;
		if(ecc == BYPASS_1)
			mem_name = "SRAM1";
		else
			mem_name = "SRAM2";

		for (uint32_t i = 0; i < length; i++)
		{
			nascerr_select_ecc(ecc);
			nascerr_count_region(&res, read[i] ^ data);

			print_bit_errors(offset_addr + i, data, read[i], read[i] ^ data, mem_name, 16, -1);
			bit_error_count = contar_bits_setados(read[i] ^ data);

			if (bit_error_count == 1) // para o bypass fica a quantidade de bits errados
				single_bit++;
			else if (bit_error_count == 2)
				double_bit++;
			else if (bit_error_count >= 3)
				triple_bit++;
		}
	}
	else {
		for (uint32_t i = 0; i < length; i++) {
			nascerr_select_ecc(BYPASS_1);

			uint16_t diff_up = read[i] ^ data;
			nascerr_count_region(&res, diff_up);
			print_bit_errors(offset_addr + i, data, read[i], diff_up, "SRAM1", 16, -1);
			bit_error_count = contar_bits_setados(diff_up);

			if (bit_error_count == 1)
				single_bit++;
			else if (bit_error_count == 2)
				double_bit++;
			else if (bit_error_count >= 3)
				triple_bit++;

			nascerr_select_ecc(BYPASS_2);

			bit_error_count = 0;
			uint16_t diff_dn = read[i] ^ data;
			nascerr_count_region(&res, diff_dn);
			print_bit_errors(offset_addr + i, data, read[i], diff_dn, "SRAM2", 16, -1);
			bit_error_count += contar_bits_setados(diff_dn);

			if (bit_error_count == 1)
				single_bit++;
			else if (bit_error_count == 2)
				double_bit++;
			else if (bit_error_count >= 3)
				triple_bit++;
		}
	}

	res.single_bit_errors = single_bit;
	res.double_bit_errors = double_bit;
	res.triple_bit_errors = triple_bit;
	res.uncorrectable_errors = single_bit + double_bit + triple_bit;

	if (single_bit || double_bit || triple_bit)
		res.ecc_status = ECC_FAIL;
	else
		res.ecc_status = ECC_OK;

	return res;
}
/*
 * ******************************************************************************
 *  Function Name: nascerr_check_command
 *  @brief     : Validate command fields and check for parameter limits/consistency
 *  @param     : command - NASCERR_CTRL structure containing write parameters
 *  @retval    : Validation status (uint8_t: 1 = valid, 0 = invalid)
 * ******************************************************************************
 */
uint8_t nascerr_check_command(NASCERR_SCHEDULER command)
{
	if (command.block < 0 || command.block > NUM_BLOCKS || command.block > (NUM_BLOCKS + command.qtd_block))
	{
		DEBUG_PRINT(LOG_WARN, "EXPERIMENTO ID: %d INTERROMPIDO!\n QUANTIDADE DE BLOCO INVALIDO \r\n", command.id);
		return false;
	}


	return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_block_to_page
 *  @brief     : Convert the logical block index to the corresponding page
 *  amount/offset.
 *  @param     : block - Logical block index (0 … N)
 *  @retval    : page amount/offset from the start of memory (uint32_t)
 * ******************************************************************************
 */
uint32_t nascerr_block_to_page(uint8_t block)
{
	return block * NUM_PAGES_PER_BLOCK;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_block_to_page
 *  @brief     : Convert the logical page index to the corresponding word
 *  amount/offset.
 *  @param     : page - Logical page index (0 … N)
 *  @retval    : word amount/offset from the start of memory (uint32_t)
 * ******************************************************************************
 */
uint32_t nascerr_page_to_words(uint16_t page)
{
	return page * NUM_WORDS_PER_PAGE;
}

