/**
 * ******************************************************************************
 *  @file    : NASCERR_Config.h
 *  @brief   : Configuration header file for NASCERR project.
 *             Contains debug options, constants, and exported symbols
 *             from the linker script related to firmware versioning
 *             and application size.
 *  @author  : Joao Manoel
 * ******************************************************************************
 */

#ifndef INC_NASCERR_CONFIG_H_
#define INC_NASCERR_CONFIG_H_
/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

/* Debug and simulation options */
#define MOCK_COMMAND        	    1   /* > Mock command interface enable flag        */
#define MOCK_EXPERIMENT     	    0   /* > Mock experiment interface enable flag     */
#define ENABLE_LOGS         	    1   /* > Enable/disable log system                 */
#define SOFTWARE_DEBUG      	    1   /* > Enable software debug mode                */
#define ENABLE_SERIAL_SYS		    0   /* > Enable a serial log print                 */ //TODO: DESABILITEI PARA TESTE
#define TEST_MODE          		    1   /* > 1 - Earth Mode | 0 - Space Mode           */
#define ENABLE_SABOTAGE_FLASH 	    0 	/* > Injects, based on probabilities, fails    */
#define ERROR_ENABLE        		0   /* > habilita escrita com com "erros" configurar em NASCERR_Memory_32bits */
#define ENABLE_ERROR_SRAM_DEBUG 	1   /* > habilita print de erros na SRAM									*/
#define CURRENT_LOG_LEVEL			LOG_INFO /* > Define o nivel maximo do log			*/

/* Metadata signature */
#define METADATA_MAGIC      0xAABBCCDD  /* > Magic number for metadata validation */

/*
 * ******************************************************************************
 *  Exported macros
 * ******************************************************************************
 */

/* Application version exported from linker script */
extern const unsigned int _APP_VERSION;
#define APP_VERSION    ((uint32_t)&_APP_VERSION)

/* Application boundaries exported from linker script */
extern uint8_t _app_end[], _app_start[];
#define APP_SIZE       ((uint32_t)(_app_end - _app_start))

#endif /* INC_NASCERR_CONFIG_H_ */
