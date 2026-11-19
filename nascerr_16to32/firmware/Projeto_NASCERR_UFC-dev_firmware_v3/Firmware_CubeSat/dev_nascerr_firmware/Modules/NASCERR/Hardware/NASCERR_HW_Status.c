/*
 * ******************************************************************************
 *  @file    : NASCERR_HW_Status.c
 *  @brief   : NASCERR Hardware verification and status module.
 *  @author  : Cicero Oliveira
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */
#include "NASCERR_HW_Status.h"
#include "NASCERR_ECCs_32bits.h"
#include "NASCERR_Log.h"

/*
 * ******************************************************************************
 *  Variables
 * ******************************************************************************
 */
extern TIM_HandleTypeDef htim15;
uint8_t shift_adaptativo = 0;

uint8_t hw_flag = 0;
uint8_t qtd_setted_bit = 0;
uint8_t window_pos = 0;

const uint32_t adc_scale_uV = 805;
static uint32_t current_u[5] = {0, 0, 0, 0, 0};
int temperature = 0;

int lut_temp_int[256] = {
	 24412, 20034, 17792, 16320, 15239, 14391, 13697, 13112,
	 12608, 12166, 11773, 11419, 11098, 10804, 10534, 10283,
	 10050, 9831, 9626, 9433, 9250, 9077, 8912, 8755,
	 8605, 8462, 8324, 8192, 8065, 7942, 7824, 7710,
	 7599, 7492, 7388, 7288, 7190, 7095, 7002, 6912,
	 6824, 6738, 6655, 6573, 6493, 6415, 6338, 6263,
	 6190, 6118, 6047, 5978, 5910, 5844, 5778, 5713,
	 5650, 5588, 5526, 5466, 5406, 5348, 5290, 5233,
	 5177, 5121, 5066, 5012, 4959, 4906, 4854, 4803,
	 4752, 4702, 4652, 4603, 4554, 4506, 4459, 4412,
	 4365, 4319, 4273, 4227, 4182, 4138, 4093, 4050,
	 4006, 3963, 3920, 3878, 3835, 3794, 3752, 3711,
	 3670, 3629, 3588, 3548, 3508, 3469, 3429, 3390,
	 3351, 3312, 3273, 3235, 3197, 3159, 3121, 3083,
	 3045, 3008, 2971, 2934, 2897, 2860, 2823, 2787,
	 2751, 2714, 2678, 2642, 2606, 2570, 2535, 2499,
	 2463, 2428, 2392, 2357, 2322, 2287, 2251, 2216,
	 2181, 2146, 2111, 2076, 2041, 2006, 1972, 1937,
	 1902, 1867, 1832, 1797, 1763, 1728, 1693, 1658,
	 1623, 1588, 1553, 1518, 1483, 1448, 1413, 1378,
	 1343, 1308, 1272, 1237, 1201, 1166, 1130, 1094,
	 1058, 1022, 986, 950, 913, 877, 840, 803,
	 766, 729, 692, 655, 617, 579, 541, 503,
	 464, 426, 387, 347, 308, 268, 228, 188,
	 147, 107, 65, 24, -18, -61, -103, -146,
	 -190, -234, -278, -323, -368, -414, -461, -508,
	 -555, -604, -652, -702, -752, -803, -855, -908,
	 -961, -1016, -1071, -1127, -1185, -1243, -1303, -1364,
	 -1427, -1491, -1556, -1623, -1692, -1763, -1836, -1911,
	 -1989, -2069, -2152, -2238, -2327, -2420, -2518, -2620,
	 -2727, -2841, -2961, -3089, -3226, -3373, -3534, -3711,
	 -3907, -4129, -4386, -4693, -5076, -5596, -27315, -27315
	};

uint32_t adc_buf[BUFFER_SIZE];
//to check errors
uint8_t window[5] = {0};
//to calculate median
uint32_t sums[6];
uint32_t buffers[6][buf_size];
uint8_t index_circular = 0;

NASCERR_HW_RESPONSE hw_status = {0};

const uint8_t hw_failures[] = {
	    HW_FAIL_1V2,
	    HW_FAIL_2V5,
	    HW_FAIL_2V5vpp,
	    HW_FAIL_3V3,
	    HW_FAIL_3V3_FPGA,
	    HW_FAIL_TEMPERATURE
	};

/*
 * ******************************************************************************
 *  Local Functions
 * ******************************************************************************
 */

void nascerr_hw_adc_start(ADC_HandleTypeDef hadc);
void nascerr_hw_adc_stop(ADC_HandleTypeDef hadc);
void nascerr_hw_tim_start(TIM_HandleTypeDef htim);
void nascerr_hw_tim_stop(TIM_HandleTypeDef htim);

NASCERR_HARDWARE_STATUS nascerr_hw_get_stat_req(void);
void nascerr_hw_calculate_power(NASCERR_HW_RESPONSE* status);//, float adc_1v2, float adc_2v5, float adc_2v5_vpp, float adc_3v3, float adc_3v3_fpga);
uint32_t nascerr_hw_calculate_limit(uint8_t idx, int temp);
int32_t interpole_temp(uint16_t adc_buf_value);

/*
 * ******************************************************************************
 *  Function Name: NASCERR_HWStart
 *  @brief     : Initializes the ADC and Timer
 *  @param     : hadc - Pointer to ADC Handler structure
 *               htim - Pointer to TIM Handler structure
 *  @retval    : none
 * ******************************************************************************
 */
void NASCERR_HWStart(ADC_HandleTypeDef hadc, TIM_HandleTypeDef htim)
{
	nascerr_hw_tim_start(htim);
	nascerr_hw_adc_start(hadc);
	nascerr_hw_tim_start(htim15);
	HAL_GPIO_WritePin(GPIOB, DEBUG_LD1_Pin, GPIO_PIN_SET);
}

/*
 * ******************************************************************************
 *  Function Name: NASCERR_HWStop
 *  @brief     : Stops the ADC and Timer
 *  @param     : hadc - Pointer to ADC Handler structure
 *               htim - Pointer to TIM Handler structure
 *  @retval    : none
 * ******************************************************************************
 */
void NASCERR_HWStop(ADC_HandleTypeDef hadc, TIM_HandleTypeDef htim)
{
	nascerr_hw_tim_stop(htim);
	nascerr_hw_adc_stop(hadc);
	HAL_GPIO_WritePin(GPIOB, DEBUG_LD1_Pin, GPIO_PIN_RESET);
}

/*
 * ******************************************************************************
 *  Function Name: NASCERR_HWScan
 *  @brief     : Stores the ADC readings and makes the conversion into their
 *               respective fields in the structure. Uses micro volts in order
 *               to work only with integers.
 *  @param     : none
 *  @retval    : none
 * ******************************************************************************
 */
void NASCERR_HWScan(void)
{
	for (int i = 0; i < 6; i++) {
	    sums[i] += adc_buf[i] - buffers[i][index_circular];
	    buffers[i][index_circular] = adc_buf[i];
	}
	index_circular = (index_circular + 1) & (buf_size-1);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_adc_convert_to_currents
 *  @brief     : transforms the mean adc reading in the mean current
 *  @param     : none
 *  @retval    : uint32_t* current_u - array containing the mean currents
 * ******************************************************************************
 */
uint32_t* nascerr_hw_adc_convert_to_currents(void)
{
	uint16_t scale = 205;
	for(uint8_t i = 0; i < 5; i++)
	{
		current_u[i] = (sums[i] * adc_scale_uV) >> 11;
		current_u[i] = (current_u[i] * scale) >> shift_size;
	}
	return current_u;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_adc_convert_to_temperature
 *  @brief     : transforms the mean adc reading in the mean temperature
 *  @param     : none
 *  @retval    : int temperature - mean temperature
 * ******************************************************************************
 */
int nascerr_hw_adc_convert_to_temperature(void)
{
	temperature = interpole_temp(sums[5] >> shift_size);
	NASCERR_HWRead();
	return temperature;
}

/*
 * ******************************************************************************
 *  Function Name: NASCERR_HWRead
 *  @brief     : Gets the values of power, temperature and current stored
 *  @param     : none
 *  @retval    : NASCERR_HW_RESPONSE - structure containing the hardware info in
 *               mA for currents, mW for power and °C for temperature
 * ******************************************************************************
 */
NASCERR_HW_RESPONSE NASCERR_HWRead(void)
{
	for(uint8_t i = 0; i < 5; i++)
		hw_status.currents[i] = ((float)current_u[i]) * 0.001;

	hw_status.temp = ((float)temperature) * 0.01;
	nascerr_hw_calculate_power(&hw_status);
	return hw_status;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_processHWStatusReq (FUNCTION USED FOR UPPER LAYERS)
 *  @brief     : Process the hardware status request and prepares transmission
 *               buffer for the hardware info.
 *  @param     : none
 *  @retval    : uint8_t - true if valid request, false - if invalid request
 * ******************************************************************************
 */
uint8_t nascerr_hw_processHWStatusReq(void)
{
    uint8_t buffer_size;
    uint16_t crc_hw_stat;
    NASCERR_HW_RESPONSE medidas = NASCERR_HWRead();

    switch (nascerr_hw_get_stat_req())
    {
        case TEMP_DATA:
        {
            buffer_size = 6;
            g_register_data[HW_TXB].size = buffer_size;

            uint8_t temp_bytes[4];
            memcpy(temp_bytes, &medidas.temp, sizeof(float));
            memcpy(g_register_data[HW_TXB].value, temp_bytes, sizeof(float));

            crc_hw_stat = calculate_crc16(g_register_data[HW_TXB].value, buffer_size - 2);
            g_register_data[HW_TXB].value[4] = (uint8_t)((crc_hw_stat >> 8) & 0xFF);
            g_register_data[HW_TXB].value[5] = (uint8_t)(crc_hw_stat & 0xFF);
            nascerr_registers_changeHwStatus(nascerr_registers_getStatusSystem().OP.HW, REQ_HW_TX_TEMP);
            //DEBUG_PRINT(LOG_INFO, "T: %f \r\n", medidas.temp);
            return true;
            break;
        }

        case CURRENT_DATA:
        {
            buffer_size = (5 * sizeof(int32_t)) + 2;
            g_register_data[HW_TXB].size = buffer_size;

            for (int i = 0; i < 5; i++) {
                uint8_t current_bytes[4];
                memcpy(current_bytes, &medidas.currents[i], sizeof(float));
                memcpy(&g_register_data[HW_TXB].value[i * 4], current_bytes, sizeof(float));
            }

            crc_hw_stat = calculate_crc16(g_register_data[HW_TXB].value, buffer_size - 2);
            g_register_data[HW_TXB].value[buffer_size - 2] = (uint8_t)((crc_hw_stat >> 8) & 0xFF);
            g_register_data[HW_TXB].value[buffer_size - 1] = (uint8_t)(crc_hw_stat & 0xFF);
            nascerr_registers_changeHwStatus(nascerr_registers_getStatusSystem().OP.HW, REQ_HW_TX_CURRENTS);
            //DEBUG_PRINT(LOG_INFO, "%f  %f  %f  %f  %f\r\n", medidas.currents[0], medidas.currents[1], medidas.currents[2], medidas.currents[3], medidas.currents[4]);
            return true;
            break;
        }

        case POWER_DATA:
        {
            buffer_size = (5 * sizeof(int32_t)) + 2;
            g_register_data[HW_TXB].size = buffer_size;

            for (int i = 0; i < 5; i++) {
                uint8_t power_bytes[4];
                memcpy(power_bytes, &medidas.powers[i], sizeof(float));
                memcpy(&g_register_data[HW_TXB].value[i * 4], power_bytes, sizeof(float));
            }

            crc_hw_stat = calculate_crc16(g_register_data[HW_TXB].value, buffer_size - 2);
            g_register_data[HW_TXB].value[buffer_size - 2] = (uint8_t)((crc_hw_stat >> 8) & 0xFF);
            g_register_data[HW_TXB].value[buffer_size - 1] = (uint8_t)(crc_hw_stat & 0xFF);
            nascerr_registers_changeHwStatus(nascerr_registers_getStatusSystem().OP.HW, REQ_HW_TX_POWERS);
            //DEBUG_PRINT(LOG_INFO, "%f  %f  %f  %f  %f\r\n", medidas.powers[0], medidas.powers[1], medidas.powers[2], medidas.powers[3], medidas.powers[4]);
            return true;
            break;
        }

        case POWER_CONSUMED:
        {
            buffer_size = 6;
            g_register_data[HW_TXB].size = buffer_size;

            uint8_t total_power_bytes[4];
            memcpy(total_power_bytes, &medidas.total_power, sizeof(float));
            memcpy(g_register_data[HW_TXB].value, total_power_bytes, sizeof(float));

            crc_hw_stat = calculate_crc16(g_register_data[HW_TXB].value, buffer_size - 2);
            g_register_data[HW_TXB].value[4] = (uint8_t)((crc_hw_stat >> 8) & 0xFF);
            g_register_data[HW_TXB].value[5] = (uint8_t)(crc_hw_stat & 0xFF);
            nascerr_registers_changeHwStatus(nascerr_registers_getStatusSystem().OP.HW, REQ_HW_TX_TOT_PWR);
            //DEBUG_PRINT(LOG_INFO, "%f \r\n", medidas.total_power);
            return true;
            break;
        }

        default:
        	return false;
            break;
    }
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_check_limits
 *  @brief     : Compares current measurements with defined limits and triggers
 *  warnings
 *  @param     : measures - Structure containing measured hardware values
 *  @retval    :uint8_t hw_flag - flag indicating which threshold were superssed
 * ******************************************************************************
 */
uint8_t nascerr_hw_check_limits(void)
{
    window_pos = (window_pos + 1) % WINDOW_SIZE;

    uint32_t limites[5];

    if (temperature < 2500)
    {
    	limites[0] = MARGIN_25_1V2;
    	limites[1] = MARGIN_25_2V5_EFFECTIVE;
    }

    else if (temperature > 8500)
    {
    	limites[0] = MARGIN_85_1V2;
    	limites[1] = MARGIN_85_2V5_EFFECTIVE;
    	hw_flag |= (1u << 5);
    }
    else
    {
    	limites[0] = BETA_1V2 * temperature + MARGIN_INTERCEPT_1V2;
    	limites[1] = BETA_2V5_EFFECTIVE * temperature + MARGIN_INTERCEPT_2V5_EFFECTIVE;
    }

    limites[2] = INTERCEPT_2V5_VPP;
    limites[3] = INTERCEPT_3V3;
    limites[4] = INTERCEPT_3V3_FPGA;

    for (uint8_t i = 0; i < 5; i++)
    {
        if (current_u[i] > limites[i]) {
            window[i] |= (1U << window_pos);
        } else {
            window[i] &= ~(1U << window_pos);
        }

        if (window[i] == 0xFF && !(hw_flag & (1U << i))) {
            hw_flag |= (1U << i);
            nascerr_registers_changeHwFailStatus(hw_flag);
        }

        if (window[i] == 0x00 && (hw_flag & (1U << i))) {
            hw_flag &= ~(1U << i);
            nascerr_registers_changeHwFailStatus(hw_flag);
        }
    }
    return hw_flag;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_check_wakeup (FUNCTION USED FOR UPPER LAYERS)
 *  @brief     : Verify if hardware should wakeup
 *  @param     : none
 *  @retval    : uint8_t - true (able to wakeup) or false (not able to wakeup)
 * ******************************************************************************
 */
uint8_t nascerr_hw_check_wakeup(void)
{
	if(nascerr_registers_getStatusSystem().OP.HW == OP_HW_SLEEP) {
		uint16_t crc_wakeup = (((uint16_t)g_register_data[WAKEUP].value[2]) << 8) | g_register_data[WAKEUP].value[1];
		if(g_register_data[WAKEUP].value[0] == 0xaa && verify_crc(g_register_data[WAKEUP].value, g_register_data[WAKEUP].size - 2, crc_wakeup)) {
			return true;
		}
	}
	return false;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_check_wakeup (FUNCTION USED FOR UPPER LAYERS)
 *  @brief     : Verify if hardware should wakeup
 *  @param     : none
 *  @retval    : uint8_t - true (able to wakeup) or false (not able to wakeup)
 * ******************************************************************************
 */
uint8_t nascerr_hw_check_sleep(void)
{
	uint16_t crc_sleep = (((uint16_t)g_register_data[SLEEP].value[2]) << 8) | g_register_data[SLEEP].value[1];
	if(g_register_data[SLEEP].value[0] == 0xaa && verify_crc(g_register_data[SLEEP].value, g_register_data[SLEEP].size - 2, crc_sleep)) {
		return true;
	}
	return false;
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_shutdown
 *  @brief     : Disables the FPGA and RAM
 *  @param     : none
 *  @retval    : none
 * ******************************************************************************
 */
void nascerr_hw_shutdown(void)
{
	HAL_GPIO_WritePin(DIS_GPIO_Port,DIS_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DEBUG_LD2_GPIO_Port, DEBUG_LD2_Pin, GPIO_PIN_RESET);
	nascerr_registers_changeHwStatus(OP_HW_SLEEP, REQ_HW_NO_REQUEST);
}

/*
 * *******************************************************************************
 *  Function Name: nascerr_waking_up
 *  @brief       : Enables power to the FPGA and RAM
 *  @param       : none
 *  @retval      : none
 * *******************************************************************************
 */
void nascerr_hw_waking_up(void)
{
	HAL_GPIO_WritePin(DIS_GPIO_Port, DIS_Pin, GPIO_PIN_SET);
	nascerr_registers_changeHwStatus(OP_HW_OK, REQ_HW_NO_REQUEST);
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_ready (FUNCTION USED FOR UPPER LAYERS)
 *  @brief     : Verify if hardware should be able to do conversions
 *  @param     : none
 *  @retval    : uint8_t - true (able) or false (not able)
 * ******************************************************************************
 */
uint8_t nascerr_hw_get_fail_flag(void)
{
	return hw_flag;
}

/*
 * ******************************************************************************
 *  Local Functions Definitions
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_adc_start
 *  @brief     : Calibrates and initializes the ADC
 *  @param     : hadc - Pointer to the ADC Handler structure
 *  @retval    : none
 * ******************************************************************************
 */
void nascerr_hw_adc_start(ADC_HandleTypeDef hadc)
{
	if (HAL_ADCEx_Calibration_Start(&hadc, ADC_SINGLE_ENDED) != HAL_OK)
	{
		DEBUG_PRINT(LOG_ERROR, "[ADC] Falha na auto-calibracao do ADC\r\n");
		Error_Handler();
	}

	if (HAL_ADC_Start_DMA(&hadc, adc_buf, BUFFER_SIZE) != HAL_OK)
	{
		DEBUG_PRINT(LOG_ERROR, "[ADC] Falha ao iniciar conversao DMA do ADC\r\n");
		Error_Handler();
	}
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_adc_stop
 *  @brief     : Stops the ADC DMA conversion
 *  @param     : hadc - Pointer to the ADC Handler structure
 *  @retval    : none
 * ******************************************************************************
 */
void nascerr_hw_adc_stop(ADC_HandleTypeDef hadc)
{
	if (HAL_ADC_Stop_DMA(&hadc) != HAL_OK)
	{
		DEBUG_PRINT(LOG_ERROR, "[ADC] Falha ao parar conversao DMA do ADC\r\n");
		Error_Handler();
	}
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_tim_start
 *  @brief     : Starts the hardware timer with interrupt
 *  @param     : htim - Pointer to the Timer Handler structure
 *  @retval    : none
 * ******************************************************************************
 */
void nascerr_hw_tim_start(TIM_HandleTypeDef htim)
{
	if (HAL_TIM_Base_Start(&htim) != HAL_OK)
	{
		DEBUG_PRINT(LOG_ERROR, "[TIM] Falha ao iniciar timer\r\n");
		Error_Handler();
	}
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_tim_stop
 *  @brief     : Stops the hardware timer
 *  @param     : htim - Pointer to the Timer Handler structure
 *  @retval    : none
 * ******************************************************************************
 */
void nascerr_hw_tim_stop(TIM_HandleTypeDef htim)
{
	if (HAL_TIM_Base_Stop(&htim) != HAL_OK)
	{
		DEBUG_PRINT(LOG_ERROR, "[TIM] Falha ao parar timer\r\n");
		Error_Handler();
	}
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_adc_covertion_pause
 *  @brief     : Pauses the hardware timer that triggers the adc(disables counting)
 *  @param     : none
 *  @retval    : none
 * ******************************************************************************
 */
void  nascerr_hw_adc_covertion_pause(void)
{
	__HAL_RCC_TIM2_CLK_DISABLE();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_adc_covertion_unpause
 *  @brief     : Resumes the hardware timer after being paused, which restart the
 *  adc triggering
 *  @param     : none
 *  @retval    : none
 * ******************************************************************************
 */
void nascerr_hw_adc_covertion_unpause(void)
{
	__HAL_RCC_TIM2_CLK_ENABLE();
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_get_stat_req
 *  @brief     : Gets the hardware requisition made.
 *  @param     : none
 *  @retval    : NASCERR_HARDWARE_STATUS - returns the hardware info requisition
 * ******************************************************************************
 */
NASCERR_HARDWARE_STATUS nascerr_hw_get_stat_req(void)
{
	return (NASCERR_HARDWARE_STATUS) g_register_data[HW_STAT].value[0];
}

/*
 * ******************************************************************************
 *  Function Name: nascerr_hw_calculate_power
 *  @brief     : Calculates power consumption and adds compensation
 *  for the fast signals and wrappers it on the the response strucuture
 *  @param     : status - Pointer to response structure to be updated
 *  @retval    : none
 * ******************************************************************************
 */
void nascerr_hw_calculate_power(NASCERR_HW_RESPONSE* status)
{
	float delta_corrente_2v5 = 0.0f;
	float current_estimate = 0.0f;

    NASCERR_MODE ecc = nascerr_get_current_ecc();
    if(ecc == TBEC || ecc == MRSC) {
    	current_estimate = BETA_2V5_BOTH_ON * status->temp + INTERCEPT_2V5_BOTH_ON;
    	delta_corrente_2v5 = current_estimate/1000.0 - status->currents[1];
    }
    else if(ecc == BYPASS_1 || ecc == BYPASS_2) {
    	current_estimate = BETA_2V5_BYPASS * status->temp + INTERCEPT_2V5_BYPASS;
    	delta_corrente_2v5 = current_estimate/1000.0 - status->currents[1];
    }

    status->powers[0] = status->currents[0] * 1.2f;
    status->powers[1] = (status->currents[1] + delta_corrente_2v5) * 2.5f;
    status->powers[2] = status->currents[2] * 2.5f;
    status->powers[3] = status->currents[3] * 3.3f;
    status->powers[4] = status->currents[4] * 3.3f;

    status->total_power = 0.0f;
    for (int i = 0; i < 5; i++) {
        status->total_power += status->powers[i];
    }
}

/*
 * ******************************************************************************
 *  Function Name: interpole_temp
 *  @brief     : Estimates the temperatura through linear interpolation based on
 *  the lut and adc value read
 *  @param     : adc_buf_value - Adc value read and stored in buffer (not
 *  converted to volts)
 *  @retval    : int_32_t - Temperature value scaled by 100
 * ******************************************************************************
 */
int32_t interpole_temp(uint16_t adc_buf_value)
{
    uint16_t index;
    int32_t t0, t1;
    int32_t interpolated_scaled;

    index = adc_buf_value >> SHIFT_FOR_INDEX;

    if (index >= (LUT_SIZE - 1)) {
        return lut_temp_int[LUT_SIZE - 1];
    }

    t0 = lut_temp_int[index];
    t1 = lut_temp_int[index + 1];

    uint32_t delta_val = adc_buf_value & ((1 << SHIFT_FOR_INDEX) - 1);
    int32_t delta_t = t1 - t0;
    interpolated_scaled = t0 + ((delta_t * (int32_t)delta_val) >> SHIFT_FOR_INDEX);
    return interpolated_scaled;
}
