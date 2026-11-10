/*
 * Host-side stand-in for the firmware header Core/Inc/NASCERR_ECCs.h.
 *
 * Lets the 8-bit firmware's ECC encoders
 *   nascerr_8to16_legacy/firmware/Projeto_NASCERR_UFC-image_processing/Firmware_CubeSat/NASCERR_slave/Core/Src/NASCERR_ECCs.c
 * be compiled UNMODIFIED into the VCS simulation (via -CFLAGS -I<repo>/dpi) and
 * called from SystemVerilog through DPI-C as the scoreboard's golden model.
 * The real header pulls in the STM32 HAL; NASCERR_ECCs.c only needs the types
 * below (copied from Core/Inc/NASCERR_System.h and NASCERR_Control.h).
 */
#ifndef DPI_NASCERR_ECCS_H_
#define DPI_NASCERR_ECCS_H_

#include <stdint.h>

/* From NASCERR_System.h (8-bit firmware). */
typedef enum
{
	UNDEFINED_MOD = 0,
	BYPASS_TO_SRAM,
	BYPASS_TO_FLASH,
	ECC_MOD1,   /* Hamming -> FPGA ecc_sel = 2'b01 */
	ECC_MOD2,   /* TBEC    -> FPGA ecc_sel = 2'b10 */
	ECC_MOD3,   /* EDAC    -> FPGA ecc_sel = 2'b11 */
} NASCERR_MODE;

/* From NASCERR_Control.h (8-bit firmware). */
typedef uint32_t nascerr_block;

uint16_t nascerr_eccs_edac_encoder(uint16_t data_in);
uint16_t nascerr_eccs_hamming_encoder(uint16_t data_in);
uint16_t nascerr_eccs_tbec_encoder(uint16_t data_in);
void nascerr_eccs_encoded_buffer(uint16_t* write_buffer, uint16_t* encoded_buffer, NASCERR_MODE ecc, nascerr_block length);

#endif /* DPI_NASCERR_ECCS_H_ */
