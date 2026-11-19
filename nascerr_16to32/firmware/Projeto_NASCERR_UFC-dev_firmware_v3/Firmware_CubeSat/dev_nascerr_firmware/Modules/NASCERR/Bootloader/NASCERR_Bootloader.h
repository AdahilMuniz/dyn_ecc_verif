/**
 * ******************************************************************************
 * @file    : NASCERR_Bootloader.h
 * @brief   : Header for NASCERR_Bootloader.c file. Contains definitions
 * and function prototypes for the bootloader logic.
 * @author  : Joao Manoel
 * ******************************************************************************
 */
#ifndef _NASCERR_BOOTLOADER_H_
#define _NASCERR_BOOTLOADER_H_

/*
 * ******************************************************************************
 * Includes   <System Includes> , "Project Includes"
 * ******************************************************************************
 */
#include "NASCERR_BootInterface.h"
#include "NASCERR_Config.h"
#include "NASCERR_InternalFlash.h"
#include "NASCERR_Registers.h"
#include "NASCERR_Log.h"
#include "NASCERR_SysTime.h"
#include "NASCERR_Control.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "main.h"

/*
 * ******************************************************************************
 * Exported constants
 * ******************************************************************************
 */

/* Firmware and metadata memory addresses */
#define FIRST_APPLICATION_ADDRESS 			FLASH_APP1_START_ADDRESS			/* > Start address for the primary application */
#define SECOND_APPLICATION_ADDRESS 			FLASH_APP2_START_ADDRESS			/* > Start address for the secondary/update application */
#define METADATA_ADDRESS					FLASH_METADATA_START_ADDRESS		/* > Start address for the firmware metadata */

#define BOOT_PAYLOAD_TRANSFER_HEADER_SIZE 	7u 									/* >  2 len + 1 cmd + 2 id + 3 CRC */
#define BOOT_PAYLOAD_TRASNFER_FINISH_SIZE   41u 								/* >  2 len + 1 cmd + 4 version + 32 sha-256 + 2 CRC */

#define BOOT_TIMEOUT_MS   					TIMER_5s   							/* > tempo máximo de espera em ms */
/*
 * ******************************************************************************
 * Exported types
 * ******************************************************************************
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;				/*!< Magic number to validate the metadata region */

    /* Information that must always be valid */
    uint32_t current_firmware_addr;		/*!< Address of the currently running firmware */
    uint32_t current_firmware_size;		/*!< Size of the current firmware */
    uint32_t current_firmware_version;	/*!< Version of the current firmware */
    uint8_t  current_firmware_sha[32];	/*!< SHA-256 hash of the current firmware */

    /* Information about the new firmware to be installed */
    uint32_t new_firmware_addr;			/*!< Memory address for the new firmware */
    uint32_t new_firmware_size;			/*!< Size of the new firmware */
    uint32_t new_firmware_version;		/*!< Version of the new firmware */
    uint8_t  new_firmware_sha[32];		/*!< SHA-256 hash of the new firmware */

    /* System status flags */
    uint8_t  boot_flag;					/*!< Flags indicating the status of the boot process (BOOT_FLAG) */
    uint8_t  execution_mode_flag;		/*!< Application region currently running */
    uint8_t  error_flag;				/*!< 0:OK; 1:CRC; 2:BOOT; 3:TIMEOUT; 4:HARDFAULT... */
    uint8_t  rollback_flag;				/*!< Indicates if a rollback is needed */

    uint32_t end_address_log;			/*!< Size of the log data in bytes */
    uint16_t crc_log;					/*!< CRC of the log data */
    uint16_t crc_metadata;				/*!< CRC of the metadata itself */

} firmware_metadata_t;

typedef enum {
    OP_MODE = 0,    								/*!< Operational mode flag */
    EX_MODE,        								/*!< Execution mode flag */
    ERROR_MODE,     								/*!< Error status flag */
    ROLLBACK_MODE,  								/*!< Rollback indicator flag */
} MetadataFlags;

typedef struct {
    uint32_t 	currentAddress;   						/**< Current writing address. */
    uint32_t 	appSize;          						/**< Total size of the application. */
    uint32_t 	bytesWritten;     						/**< Number of bytes already written. */
    uint8_t  	status;           						/**< Status of the operation. */
    ERROS_LOGS 	errorCode;        						/**< Error code, if applicable. */
    uint16_t 	lastPacketID;     						/**< ID of the last received packet. */
} BootloaderState;

typedef enum {
    PROTOCOL_STATE_IDLE = 0,      					/**< Waiting for commands. */
    PROTOCOL_STATE_RECEIVING,    					/**< Receiving data. */
    PROTOCOL_STATE_PROCESSING,    					/**< Processing received data. */
    PROTOCOL_STATE_SENDING_ACK,   					/**< Sending ACK. */
    PROTOCOL_STATE_SENDING_NACK,  					/**< Sending NACK. */
    PROTOCOL_STATE_ERROR,         					/**< Error state. */
} BootProtocolState_t;

typedef enum {
    BOOT_FLAG_UPDATE_PENDING        = (1 << 0), 	/* new image completely written           */
    BOOT_FLAG_IN_BOOTLOADER         = (1 << 1), 	/* currently running in bootloader        */
    BOOT_FLAG_IMAGE_VALID           = (1 << 2), 	/* header / metadata of the new image OK  */
    BOOT_FLAG_DOWNLOAD_SUCCESS      = (1 << 3), 	/* last DATA_TRANSFER packet ACKed        */
    BOOT_FLAG_RETRANSMIT_REQ        = (1 << 4), 	/* at least one packet must be resent     */
    BOOT_FLAG_ROLLBACK_AVAILABLE    = (1 << 5), 	/* old image still intact for rollback    */
    BOOT_FLAG_ERROR_DETECTED        = (1 << 6), 	/* generic error happened                 */
    BOOT_FLAG_IN_DOWNLOAD           = (1 << 7), 		/* MCU still running donwload		  */
	BOOT_ALL_FLAGS  				= 0XFF, 		/* all flags set to 1                     */
} BOOT_FLAG;

typedef enum {
    PROTOCOL_CMD_DOWNLOAD_INIT 		= 0x01,    		/**< Initialize data download. */
    PROTOCOL_CMD_DOWNLOAD_FINISH 	= 0x02,  		/**< Finalize data download. */
    PROTOCOL_CMD_VERSION 			= 0x03,     	/**< Request protocol version. */
    PROTOCOL_CMD_ACKN 				= 0x06,     	/**< Acknowledgment command. */
    PROTOCOL_CMD_NACK 				= 0x15,     	/**< Negative acknowledgment command. */
    PROTOCOL_CMD_DATA_TRANSFER 		= 0xAA,    		/**< Transfer data to the target. */
} BootCommand_t;

typedef struct {
	BootProtocolState_t currentState;   			/**< Current state of the protocol FSM. */
	BootCommand_t       command;        			/**< Last received command. */
    uint8_t             data[REG_DATA_BOOT_SIZE]; 	/**< Buffer for storing received data. */
    uint16_t            dataSize;       			/**< Size of the received data in bytes. */
} BootContext_t;

//
//----------------BOOTLOADER FSM FUNCTIONS----------------//

void nascerr_bootloader_fsmInit(void);

void nascerr_bootloader_fsmStep(void);

uint8_t nascerr_bootloader_processDataTransfer(const BOOT_DATA_PACKET *pkt);

uint8_t nascerr_bootloader_processDownloadFinish(const BOOT_DATA_PACKET *pkt);

uint8_t nascerr_bootloader_init(BOOT_DATA_PACKET *ctrl);

void nascerr_bootloader_jumpToBootloader(void);

uint8_t nascerr_bootloader_processCommand(BootCommand_t command, uint8_t *data, uint16_t dataSize);

uint8_t nascerr_bootloader_calculateSha256(const uint8_t* data, uint32_t size, uint8_t* output_sha);

//----------------FLASH AND PROCESS FUNCTIONS----------------//

uint8_t nascerr_bootloader_eraseFlash(uint32_t sizeApp, uint32_t addrApp);

uint8_t nascerr_bootloader_writeToFlash(uint32_t address, uint8_t *data, uint32_t pkgSize);

//----------------METADATA FUNCTIONS----------------//

void nascerr_bootloader_initMetadata(void);

uint8_t nascerr_bootloader_writeNewMetadata(firmware_metadata_t metadata);

void nascerr_bootloader_initFlags(void);

void nascerr_bootloader_resetRuntimeFlag(BOOT_FLAG f);

void nascerr_bootloader_setRuntimeFlag(BOOT_FLAG f);

uint16_t nascerr_bootloader_getPkgID(void);

uint8_t nascerr_bootloader_updateBootFlags(void);

void nascerr_bootloader_setPkgID(uint16_t new_id);

uint8_t nascerr_bootloader_setNewVersion(uint32_t new_version, uint32_t new_version_size);

uint8_t nascerr_bootloader_writeNewFirmwareSha(const uint8_t* sha);

uint8_t *nascerr_bootloader_getBootStatus(void);

uint8_t *nascerr_bootloader_changeBootStatus(NASCERR_BOOT_STATUS *boot_status);

uint8_t nascerr_bootloader_setLogEndAddress(uint32_t new_end_address);

#endif // _NASCERR_BOOTLOADER_H_
