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
#include "main.h"

/*
 * ******************************************************************************
 *  Exported constants
 * ******************************************************************************
 */

/*
 * ******************************************************************************
 *  Exported types
 * ******************************************************************************
 */

typedef uint32_t nascerr_block;
typedef uint16_t nascerr_rpt;
typedef uint32_t nascerr_addr;
typedef uint16_t nascerr_result;

/* Defines the command types for read and write operations */
typedef enum
{
	READ_CMD = 0,     /* > Read command */
	WRITE_CMD,    	  /* > Write command */
} CmdTypeDef;

/* Defines the types of data to write during operations */
typedef enum
{
	WRITE_ZEROS = 0,   /* > Write all zeros */
	WRITE_ONES,    	   /* > Write all ones */
	WRITE_x55h,    	   /* > Write 0x55 pattern */
	WRITE_xAAh,    	   /* > Write 0xAA pattern */
	WRITE_RANDOM,      /* > Write random data */
} W_DataTypeDef;

/* Defines the types of sequential reads for NASCERR */
typedef enum
{
	WORD_BY_WORD = 0,  /* > Read word by word */
	BOCK_BY_BOCK,      /* > Read block by block */
} Seq_ReadTypeDef;

/* Defines the result description */
typedef struct
{
	nascerr_addr    addr;    /* > Address 32-bits */
	nascerr_result  result;  /* > Result: 0 - No errors ; 1 - Bit error */
} ResDescTypeDef;

/* NASCERR_CTRL */
typedef struct
{
	uint16_t        bytes_msg;     /* > Total number of bytes in the message */
	CmdTypeDef      cmd_type;      /* > Command type (read or write operation) */
	uint8_t         id;            /* > Message identifier */
	nascerr_block   data_lenght;   /* > Length of the data in blocks */
	nascerr_rpt     repeat;        /* > Number of repetitions for the operation */
	W_DataTypeDef   write_type;    /* > Type of data to write */
	Seq_ReadTypeDef data;          /* > Sequential read type (word/block) */
	NASCERR_MODE    mode;          /* > Operating mode */
	uint16_t        delay;         /* > Delay time in milliseconds */
	uint16_t		crc16;         /* > CRC16 checksum */
} NASCERR_CTRL;

/* NASCERR_RESPONSE */
typedef struct
{
	uint8_t         id;              /* > Identifier for the response message */
	nascerr_block   data_lenght;     /* > Length of the data in blocks */
	nascerr_block   number_errors;   /* > Number of detected errors */
	nascerr_block   corrected_data;  /* > Amount of data that was corrected */
	ResDescTypeDef  *result;         /* > Pointer to an array of result descriptions */
} NASCERR_RESPONSE;

/*
 * ******************************************************************************
 *  Exported global functions (to be accessed by other files)
 * ******************************************************************************
 */

#endif /* INC_NASCERR_CONTROL_H_ */
