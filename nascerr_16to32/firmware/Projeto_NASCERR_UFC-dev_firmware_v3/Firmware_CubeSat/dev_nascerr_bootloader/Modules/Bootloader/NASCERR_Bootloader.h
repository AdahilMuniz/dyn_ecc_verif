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
#include "NASCERR_InternalFlash.h"
#include "NASCERR_System.h"
#include "NASCERR_Util.h"
#include "NASCERR_DB.h"
#include "NASCERR_Registers.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/*
 * ******************************************************************************
 * Exported constants
 * ******************************************************************************
 */

/* Firmware and metadata memory addresses */
#define FIRST_APPLICATION_ADDRESS 			FLASH_APP1_START_ADDRESS			/* > Start address for the primary application */
#define SECOND_APPLICATION_ADDRESS 			FLASH_APP2_START_ADDRESS			/* > Start address for the secondary/update application */
#define METADATA_ADDRESS					FLASH_METADATA_START_ADDRESS		/* > Start address for the firmware metadata */

#define METADATA_MAGIC 	0xAABBCCDD
#define APP_MAGIC 		0x12345678
#define VT_OFFSET       0x1C0 - 4  /* > Offset for the vector table in the metadata region */

/*
 * ******************************************************************************
 * Exported types
 * ******************************************************************************
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;						/*!< Magic number to validate the metadata region */

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
    uint8_t  boot_flag;
    uint8_t  execution_mode_flag;		/*!< Application region currently running */
    uint8_t  error_flag;				/*!< 0:OK; 1:CRC; 2:BOOT; 3:TIMEOUT; 4:HARDFAULT... */
    uint8_t  rollback_flag;				/*!< Indicates if a rollback is needed */

    uint32_t end_address_log;			/*!< Size of the log data in bytes */
    uint16_t crc_log;					/*!< CRC of the log data */
    uint16_t crc_metadata;				/*!< CRC of the metadata itself */

} firmware_metadata_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
} application_signature_t;

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
    BL_STATE_CHECK_METADATA = 0,
    BL_STATE_RECOVER_METADATA,
    BL_STATE_CREATE_METADATA,

    BL_STATE_INIT,
    BL_STATE_VERIFY_INTEGRITY,
    BL_STATE_VERIFY_CURRENT,

    BL_STATE_BACKUP_FIRMWARE,
    BL_STATE_ERASE_TARGET_BANK,
    BL_STATE_COPY_FIRMWARE,
    BL_STATE_VERIFY_COPY,
    BL_STATE_FINALIZE_SUCCESS,
    BL_STATE_DISCARD_UPDATE,

    BL_STATE_ROLLBACK,
    BL_STATE_JUMP_TO_APP,
    BL_STATE_FAILURE,
    BL_STATE_CRITICAL_FAILURE,
} BootloaderFSM_State;

typedef struct {
    uint32_t 	currentAddress;   						/**< Current writing address. */
    uint32_t 	appSize;          						/**< Total size of the application. */
    uint32_t 	bytesWritten;     						/**< Number of bytes already written. */
    uint8_t  	status;           						/**< Status of the operation. */
    ERROS_LOGS 	errorCode;        						/**< Error code, if applicable. */
    uint16_t 	lastPacketID;     						/**< ID of the last received packet. */
} BootloaderState;

void nascerr_bootloader_FSM_Init(void);

void nascerr_bootloader_FSM_Run(void);

uint8_t nascerr_bootloader_calculateSha256(const uint8_t* data, uint32_t size, uint8_t* output_sha);

//----------------FLASH AND PROCESS FUNCTIONS----------------//

uint8_t nascerr_bootloader_hasValidBackup(void);

uint8_t nascerr_bootloader_canRollback(void);

uint8_t nascerr_bootloader_eraseFlash(uint32_t sizeApp, uint32_t addrApp);

uint8_t nascerr_bootloader_writeToFlash(uint32_t address, uint8_t *data, uint32_t pkgSize);

//----------------METADATA FUNCTIONS----------------//

uint8_t nascerr_bootloader_writeNewMetadata(firmware_metadata_t metadata);

void nascerr_bootloader_initFlags(void);

uint8_t nascerr_bootloader_updateBootFlags(void);

void nascerr_bootloader_resetRuntimeFlag(BOOT_FLAG f);

void nascerr_bootloader_setRuntimeFlag(BOOT_FLAG f);

uint8_t nascerr_bootloader_buildFlagsFromMetadata(void);

uint8_t *nascerr_bootloader_changeBootStatus(NASCERR_BOOT_STATUS *boot_status);

uint8_t *nascerr_bootloader_getBootStatus(void);

uint8_t nascerr_bootloader_setCurrentVersion(uint32_t new_version, uint32_t new_version_size);

#endif // _NASCERR_BOOTLOADER_H_
