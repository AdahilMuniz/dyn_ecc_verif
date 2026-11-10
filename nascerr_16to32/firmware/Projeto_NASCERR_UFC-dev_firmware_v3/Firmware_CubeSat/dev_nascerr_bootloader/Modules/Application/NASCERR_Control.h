/**
 * ******************************************************************************
 *  @file    : NASCERR_Control.h
 *  @brief   : Header for NASCERR_Control.c file.
 *  @author  : Marcus Anderson
 * ******************************************************************************
 */

#ifndef INC_NASCERR_CONTROL_H_
#define INC_NASCERR_CONTROL_H_

/*
 * ******************************************************************************
 *  Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */
#include "NASCERR_System.h"
//#include "NASCERR_Registers.h"
#include "main.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

/* Helpers: le inteiros big-endian de um vetor de bytes */
#define GET_U16_BE(p) ( ((uint16_t)(p)[0] << 8) | (uint16_t)(p)[1] )
#define GET_U32_BE(p) ( ((uint32_t)(p)[0] << 24) | ((uint32_t)(p)[1] << 16) | \
                        ((uint32_t)(p)[2] <<  8) |  (uint32_t)(p)[3] )

#define EXP_UNREAD_PAGE_SIZE   30   /* > Maximo de itens por mensagem/resumo   */
/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

typedef uint32_t nascerr_block;
typedef uint16_t nascerr_rpt;
typedef uint32_t nascerr_addr;
typedef uint32_t nascerr_result;

typedef uint16_t nascerr_nblock;
typedef uint32_t nascerr_data;
typedef uint16_t id_block;
typedef uint32_t id_db;
typedef uint32_t time_t;

/*
 * ******************************************************************************
 *  Enum Name : NASCERR_COMMANDS
 *  @brief    : Comandos de alto nível recebidos pelo protocolo I2C
 * ******************************************************************************
 */
typedef enum
{
    NO_COMMANDS        = 0,  /* > Nenhum comando recebido                   */
    SEND_STATUS,             /* > Solicita envio de status geral            */
	CMD_RECEIVED,            /* > Comando de experimento recebido           */
    SEND_MODE,               /* > Solicita modo de operacao atual           */
    COMMAND_TO_SLEEP,        /* > Coloca sistema em modo sleep              */
    COMMAND_TO_WAKEUP,       /* > Acorda sistema do modo sleep              */
    GPS_DATA_RECEIVED,       /* > Confirma dado de GPS recebido             */
    ACK_RECEIVED,            /* > ACK recebido da Master                    */
	SET_PAGE_SUMARY,         /* > Define pagina de resumo de experimentos   */
	HW_STATUS_COMMAND,		 /* > Hardware Command							*/
	BOOT_COMMAND,            /* > Comando de bootloader recebido            */
	CLEAR_DB_COMMAND,        /* > Comando para limpar o banco de dados      */
} NASCERR_COMMANDS;

/*
 * ******************************************************************************
 *  Enum Name : NASCERR_HARDWARE_STATUS
 *  @brief    : Tipos de dados de telemetria enviados pela placa
 * ******************************************************************************
 */
typedef enum
{
    TEMP_DATA      = 0xB1,   /* > Temperatura instantanea                   */
    CURRENT_DATA,            /* > Corrente instantanea                      */
    POWER_DATA,              /* > Potencia/tensao instantanea               */
    POWER_CONSUMED           /* > Energia consumida acumulada               */
} NASCERR_HARDWARE_STATUS;

/*
 * ******************************************************************************
 *  Enum Name : CmdTypeDef
 *  @brief    : Tipos de comando para leitura e escrita
 * ******************************************************************************
 */
typedef enum
{
    READ_CMD  = 0,            /* > Comando de leitura                        */
    WRITE_CMD                 /* > Comando de escrita                        */
} CmdTypeDef;

/*
 * ******************************************************************************
 *  Enum Name : W_DataTypeDef
 *  @brief    : Padrões de dados usados nas operacoes de escrita
 * ******************************************************************************
 */
typedef enum
{
    WRITE_ZEROS = 0,         /* > Gravar somente zeros                      */
    WRITE_ONES,              /* > Gravar somente uns                        */
    WRITE_x55h,              /* > Gravar padrão 0x55                        */
    WRITE_xAAh               /* > Gravar padrão 0xAA                        */
} W_DataTypeDef;

/*
 * ******************************************************************************
 *  Enum Name : Seq_ReadTypeDef
 *  @brief    : Modos de leitura sequencial de dados
 * ******************************************************************************
 */
typedef enum
{
    WORD_BY_WORD = 0,        /* > Ler palavra a palavra                     */
    BOCK_BY_BOCK             /* > Ler bloco a bloco                         */
} Seq_ReadTypeDef;

/*
 * ******************************************************************************
 *  Enum Name : NASCERR_MODE
 *  @brief    : Modos de operacao e teste do NASCERR
 * ******************************************************************************
 */
typedef enum
{
    UNDEFINED_MOD = 0,       /* > Modo nao definido                         */
    BYPASS_1,                /* > Bypass modo 1                             */
    BYPASS_2,                /* > Bypass modo 2                             */
    TBEC,                    /* > Teste TBEC padrão                         */
    MRSC,                    /* > Modo MRSC                                 */
    DUAL_BYPASS              /* > Bypass duplo                              */
} NASCERR_MODE;

/**
 * @brief Enum para selecionar a interface de comunicação.
 */
typedef enum {
    COMM_UART,
    COMM_I2C
} BootCommInterface_t;


/**
 * @brief Estrutura para armazenar os dados recebidos para bootloader.
 */
typedef struct __attribute__((packed)){
	BootCommInterface_t interface;  /**< Tipo da interface (UART ou I2C) */
	uint8_t cmd;              /**< Ponteiro para os dados recebidos */
    uint8_t *data;              /**< Ponteiro para os dados recebidos */
    uint16_t dataSize;          /**< Tamanho dos dados recebidos */
} BOOT_DATA_PACKET;


////////////////////// TODO: APAGAR ////////////////////////////////////////////////
/* Defines the result description */
typedef struct __attribute__((packed))
{
	nascerr_addr    addr;    /* > Address 32-bits */
	nascerr_result  result;  /* > Result: 0 - No errors ; 1 - Bit error */
} ResDescTypeDef;

/* NASCERR_RESPONSE */
#pragma pack(1)
typedef struct
{
	uint16_t        id;              	/* > Identifier for the response message */
	nascerr_block   data_lenght;     	/* > Length of the data in blocks */
	nascerr_block   number_errors;   	/* > Number of detected errors */
	nascerr_block   corrected_data;  	/* > Amount of data that was corrected */
	float			percent_error;		/* > Command type */
	CmdTypeDef		cmd_type;			/* > Command type */
	uint16_t		number_packages;	/* > Total number of packages */
	uint16_t		package_number;		/* > Current package number */
	uint16_t		error_by_package;	/* > Errors per package */
	ResDescTypeDef  *result;         	/*> Pointer to an array of result descriptions */

} NASCERR_RESPONSE;
////////////////////////////////////////////////////////////////////////////////////

#pragma pack(push, 1)
/*
 * ******************************************************************************
 *  Struct Name: LogOperation
 *  @brief     : Estrutura de logs de operacoes do NASCERR
 * ******************************************************************************
 */
typedef struct
{
	uint32_t timestamp;     /* > Tempo (em milissegundos desde o boot) em que a operacao ocorreu. 	*/
	uint8_t operation_type; /* > Tipo da operacao executada.										*/
	uint8_t status;			/* > Status da operacao.												*/
	uint8_t error_code;     /* > Codigo especifico do resultado da operacao.						*/

} LogOperation;

/*
 * ******************************************************************************
 *  Struct Name: MDCA_Data_t
 *  @brief     : Estrutura que representa o tempo GNSS e a localizacao fornecida pelo MDCA
 * ******************************************************************************
 */
typedef struct
{
    uint16_t gnss_week;                    /* > Numero da semana GNSS (2 bytes)                */
    uint32_t gnss_tow;                     /* > Tempo da semana GNSS em segundos (4 bytes)     */

    float latitude;                        /* > Latitude em graus (4 bytes)                    */
    float longitude;                       /* > Longitude em graus (4 bytes)                   */
    float altitude_ellipsoid;              /* > Altitude no elipsoide (4 bytes)                */
    float altitude_msl;                    /* > Altitude em relacao ao nivel medio do mar (4B) */

    float ecef_x;                          /* > Coordenada X no sistema ECEF (4 bytes)         */
    float ecef_y;                          /* > Coordenada Y no sistema ECEF (4 bytes)         */
    float ecef_z;                          /* > Coordenada Z no sistema ECEF (4 bytes)         */

} MDCA_Data_t;

/*
 * ******************************************************************************
 *  Struct Name: ECC_Telemetry_t
 *  @brief     : Dados de telemetria do sistema ECC
 * ******************************************************************************
 */
typedef struct
{
	nascerr_data  single_bit_errors;      /* > Total de erros de 1 bit corrigidos                     */
	nascerr_data  double_bit_errors;      /* > Total de erros de 2 bits corrigidos                    */
	nascerr_data  triple_bit_errors;      /* > Total de erros de 3 bits corrigidos                    */
	nascerr_data  beg_region_errors;      /* > Total de erros localizados no inicio da regiao         */
	nascerr_data  mid_region_errors;      /* > Total de erros localizados na regiao intermediaria     */
	nascerr_data  end_region_errors;      /* > Total de erros localizados no final da regiao          */
	nascerr_data  uncorrectable_errors;   /* > Total de erros nao corrigiveis                         */

    NASCERR_MODE  ecc;                    /* > Tipo de ECC utilizado                                  */
    uint8_t       ecc_status;             /* > Status do ECC (0: OK, 1: WARN, 2: FAIL)                */

} ECC_Telemetry_t;

/*
 * ******************************************************************************
 *  Struct Name: NASCERR_EXPERIMENT
 *  @brief     : Informacoes completas do experimento NASCERR
 * ******************************************************************************
 */
typedef struct
{
    id_db           id;                    /* > Message identifier                           */
    id_block        block;                 /* > Block ID                                     */
    nascerr_nblock  data_lenght;           /* > Length of the data in blocks                 */
    time_t          time_delta;    		   /* > DeltaT entre escrita e leitura (ms)          */
    nascerr_data  	number_errors;         /* > Number of detected errors                    */
    nascerr_data  	corrected_data;        /* > Amount of data that was corrected            */
    float           percent_error;         /* > Percentual de erro detectado                 */

    ECC_Telemetry_t ecc_telemetry;         /* > Dados de telemetria do ECC                   */
    MDCA_Data_t     mdca_data;             /* > Dados de tempo e posicao do MDCA             */

} NASCERR_EXPERIMENT;

/*
 * ******************************************************************************
 *  Struct Name: NASCERR_QUERY
 *  @brief     : Informacoes de leitura do experimento NASCERR
 * ******************************************************************************
 */
typedef struct
{
    id_db    id;         /* > Identificador do experimento                     */
    uint16_t page;       /* > Offset/página onde iniciar a leitura (0..N)      */
    uint16_t qtd_block;  /* > Quantidade de structs a ler (0 = ate encher TXB) */

} NASCERR_QUERY;

/*
 * ******************************************************************************
 *  Struct Name: NASCERR_SCHEDULER
 *  @brief     : Informacoes do agendador do experimento NASCERR
 * ******************************************************************************
 */
typedef struct
{
    id_db           id;                 /* > Identificador unico da mensagem do experimento  */
	id_block        block;              /* > Identificador do bloco de dados                 */
	nascerr_nblock  qtd_block;          /* > Quantidade de blocos a serem escritos           */
	NASCERR_MODE    ecc;                /* > Tipo de codificacao ECC a ser utilizada         */
	W_DataTypeDef   write_type;    	    /* > Tipo de escrita a ser realizada                 */
	time_t          scheduler_time;     /* > Horario programado para execucao do experimento */
	nascerr_rpt     repeat;        	    /* > Numero de repeticoes programadas                */
	time_t          delay;              /* > Intervalo entre repeticoes, em ms               */
	time_t          current_tick;       /* > Tick atual do sistema operacional   			 */

} NASCERR_SCHEDULER;

/*
 * ******************************************************************************
 *  Struct Name: NASCERR_CMD
 *  @brief     : Informacoes de comando do experimento NASCERR
 * ******************************************************************************
 */
typedef struct
{
	uint16_t        	bytes_msg;     	/* > Total number of bytes in the message            */
	CmdTypeDef      	cmd_type;      	/* > Command type (read or write operation)          */
	NASCERR_SCHEDULER 	scheduler;		/* > Scheduler data for the experiment               */
	uint16_t			crc16;         	/* > CRC16 checksum                                  */

} NASCERR_CMD;

/*
 * ******************************************************************************
 *  Struct Name: NASCERR_ExperimentStatus_t
 *  @brief     : Estrutura que armazena o status geral dos experimentos NASCERR
 * ******************************************************************************
 */
typedef struct
{
    uint16_t qtd_scheduler_total;       /* > Total de agendamentos registrados               */
    uint16_t qtd_scheduler_pending;     /* > Quantidade de agendamentos ainda nao executados */
    uint16_t qtd_scheduler_executed;    /* > Quantidade de agendamentos ja executados        */

    uint16_t qtd_experiments_total;     /* > Total de experimentos registrados               */
    uint16_t qtd_experiments_running;   /* > Quantidade de experimentos em execucao          */
    uint16_t qtd_experiments_finished;  /* > Quantidade de experimentos finalizados          */
    uint16_t qtd_experiments_unread;    /* > Quantidade de experimentos nao lidos            */

    uint8_t scheduler_full_flag;        /* > Flag indicando se o escalonador esta cheio      */
    uint8_t flash_full_flag;            /* > Flag indicando se a memoria flash esta cheia    */

} NASCERR_ExperimentStatus_t;

#pragma pack(pop)

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

uint8_t nascerr_control_cmdCasting(NASCERR_CMD *ctrl, const uint8_t *cmd_value);
uint8_t nascerr_control_makeExperiment(NASCERR_EXPERIMENT *exp_out, uint32_t id, id_block num_block, time_t timestamp);

uint8_t nascerr_control_castingBootCMD(BOOT_DATA_PACKET *ctrl, uint8_t *cmd_value);

#endif /* INC_NASCERR_CONTROL_H_ */
