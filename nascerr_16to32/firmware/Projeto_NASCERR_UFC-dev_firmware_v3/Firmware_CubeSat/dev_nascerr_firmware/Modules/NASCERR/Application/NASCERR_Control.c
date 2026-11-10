/**
 * ******************************************************************************
 *  @file    : NASCERR_Control.c
 *  @brief   : Projeto NASCERR – Nanosatelite com Eletrônica Robusta a Radiação
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */

#include "NASCERR_Control.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */


/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: nascerr_control_cmdCasting
 *  @brief     : Converte buffer bruto (cmd_value) em estrutura NASCERR_CMD
 *  @param[out]: ctrl      – ponteiro para estrutura NASCERR_CMD de saida
 *  @param[in] : cmd_value – ponteiro para frame decodificado pelo protocolo
 *  @retval    : uint8_t   (true = sucesso, false = erro de tamanho / tipo)
 * ******************************************************************************
 *
 *  Formato esperado do frame (big-endian):
 *  --------------------------------------------------------------------
 *  Offset  Campo                         Tamanho
 *  --------------------------------------------------------------------
 *   0      bytes_msg                     2 bytes
 *   2      cmd_type                      1 byte
 *   3      scheduler.id                  4 bytes
 *   7      scheduler.block               2 bytes
 *   9      scheduler.qtd_block           2 bytes
 *  11      scheduler.ecc                 1 byte
 *  12      scheduler.write_type          1 byte
 *  13      scheduler.scheduler_time      4 bytes
 *  17      scheduler.repeat              2 bytes
 *  19      scheduler.delay               4 bytes
 *  23      scheduler.current_tick        4 bytes
 *  27      crc16                         2 bytes   (ja validado na camada protocolo)
 *  --------------------------------------------------------------------
 *  Total: 29 bytes  (bytes_msg deve ser 29)
 *  Ajuste os offsets/tamanhos se o protocolo real for diferente.
 */

uint8_t nascerr_control_cmdCasting(NASCERR_CMD *ctrl, const uint8_t *cmd_value)
{
    if (!ctrl || !cmd_value) return false;

    /* bytes_msg esta em big-endian (MSB primeiro) */
    uint16_t bytes_msg = GET_U16_BE(cmd_value);
    if (bytes_msg < 27u) return false;          /* 25 payload + 2 CRC */

    ctrl->bytes_msg = bytes_msg;
    ctrl->cmd_type  = (CmdTypeDef)cmd_value[2];

    const uint8_t *p = &cmd_value[3];

    ctrl->scheduler.id             = GET_U32_BE(p);  p += 4;
    ctrl->scheduler.block          = GET_U16_BE(p);  p += 2;
    ctrl->scheduler.qtd_block      = GET_U16_BE(p);  p += 2;
    ctrl->scheduler.ecc            = (NASCERR_MODE)*p++;
    ctrl->scheduler.write_type     = (W_DataTypeDef)*p++;
    ctrl->scheduler.scheduler_time = GET_U32_BE(p);  p += 4;
    ctrl->scheduler.repeat         = GET_U16_BE(p);  p += 2;
    ctrl->scheduler.delay          = GET_U32_BE(p);  p += 4;
    ctrl->scheduler.current_tick   = GET_U32_BE(p);  p += 4;
    ctrl->crc16                    = GET_U16_BE(p);  /* CRC ja validado */

    return true;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_mock_makeExperiment
 *  @brief       : Preenche um NASCERR_EXPERIMENT sintetico para testes
 *  @param[out]  : exp_out - ponteiro que recebera o experimento preenchido
 *  @param[in]   : id - numero sequencial ou id desejado
 *  @param[in]   : num_block - numero do bloco do experimento (0, 1, 2…)
 *  @param[in]   : timestamp - tempo do experimento (time_t)
 *  @retval      : true (OK) ou false (erro nos argumentos)
 * ******************************************************************************
 */
uint8_t nascerr_control_makeExperiment(NASCERR_EXPERIMENT *exp_out, uint32_t id, id_block num_block, time_t timestamp)
{
    if (!exp_out) return false;

    memset(exp_out, 0, sizeof(*exp_out));

    /* Cabeçalhos principais */
    exp_out->id            = id;
    exp_out->block         = num_block;      /* pode incrementar depois       */
    exp_out->data_lenght   = 4;              /* 4 blocos (exemplo)            */
    exp_out->time_delta  = timestamp;
    exp_out->number_errors = 0;
    exp_out->corrected_data= 0;
    exp_out->percent_error = 0.0f;

    /* Telemetria ECC */
    exp_out->ecc_telemetry.single_bit_errors = 0;
    exp_out->ecc_telemetry.double_bit_errors = 0;
    exp_out->ecc_telemetry.triple_bit_errors = 0;
    exp_out->ecc_telemetry.beg_region_errors = 0;
    exp_out->ecc_telemetry.mid_region_errors = 0;
    exp_out->ecc_telemetry.end_region_errors = 0;
    exp_out->ecc_telemetry.uncorrectable_errors = 0;
    exp_out->ecc_telemetry.ecc        = MRSC; /* por exemplo */
    exp_out->ecc_telemetry.ecc_status = 0;

    /* Dados MDCA (GNSS) */
    exp_out->mdca_data.gnss_week          = 2345;
    exp_out->mdca_data.gnss_tow           = 360000;
    exp_out->mdca_data.latitude           = -3.7319f;
    exp_out->mdca_data.longitude          = -38.5267f;
    exp_out->mdca_data.altitude_ellipsoid = 25.0f;
    exp_out->mdca_data.altitude_msl       = 21.0f;
    exp_out->mdca_data.ecef_x             = 4085.0f;
    exp_out->mdca_data.ecef_y             = -4231.0f;
    exp_out->mdca_data.ecef_z             = 0.0f;

    return true;
}


/**
 * ******************************************************************************
 *  Function Name: nascerr_control_castingDataCMD
 *  @brief     : Converte o comando em um BOOT_DATA_PACKET
 *  @param     : ctrl - ponteiro para estrutura de controle a ser preenchida
 *  @param     : cmd_value - ponteiro para os dados do comando (array uint8_t)
 *  @retval    : uint8_t (true = sucesso, false = erro de tamanho)
 * ******************************************************************************
 */
uint8_t nascerr_control_castingBootCMD(BOOT_DATA_PACKET *ctrl, uint8_t *cmd_value)
{
	ctrl->interface = COMM_I2C;
	const uint8_t *p = cmd_value;
	ctrl->dataSize = GET_U16_BE(p); p+=2; /* tamanho total do dado */

	if (ctrl->dataSize > REG_DATA_BOOT_SIZE)
	{
		return false;
	}

	ctrl->cmd =  *p++; /* comando */
	ctrl->data = &cmd_value[3];

	return true;
}


/**
 * ******************************************************************************
 *  Function Name: nascerr_control_gps_check_crc
 *  @brief     : Verifica a validade do crc do pacote de gps recebido
 *  @param     : none
 *  @retval    : uint8_t (true = valido, false = crc falhou)
 * ******************************************************************************
 */
uint8_t nascerr_control_gps_check_crc()
{

	uint16_t crc_gps = (((uint16_t)g_register_data[GPS].value[g_register_data[GPS].size - 1]) << 8) | g_register_data[GPS].value[g_register_data[GPS].size - 2];
	if(verify_crc(g_register_data[GPS].value, g_register_data[GPS].size - 2, crc_gps))
	{
		return true;
	}
	return false;
}

/**
 * ******************************************************************************
 *  Function Name: nascerr_control_fill_experiment_gps
 *  @brief     : Preenche o experimento com as informacoes referentes
 *  ao gps e tempo atual apos o lancamento.
 *  @param     : exp_out - ponteiro que recebera o experimento a ser preenchido
 *  @param	   : gps_info - ponteiro que contem as ultimas informacoes de gps
 *  @retval    : none
 * ******************************************************************************
 */
void nascerr_control_fill_experiment_gps(NASCERR_EXPERIMENT* exp_out, MDCA_Data_t* gps_info)
{
	exp_out->mdca_data.gnss_week          = gps_info->gnss_week;
	exp_out->mdca_data.gnss_tow           = gps_info->gnss_tow;
	exp_out->mdca_data.latitude           = gps_info->latitude;
	exp_out->mdca_data.longitude          = gps_info->longitude;
	exp_out->mdca_data.altitude_ellipsoid = gps_info->altitude_ellipsoid;
	exp_out->mdca_data.altitude_msl       = gps_info->altitude_msl;
	exp_out->mdca_data.ecef_x             = gps_info->ecef_x;
	exp_out->mdca_data.ecef_y             = gps_info->ecef_y;
	exp_out->mdca_data.ecef_z             = gps_info->ecef_z;
}
