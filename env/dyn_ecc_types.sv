// Shared types and the golden ECC model for the dyn_ecc (8->16) environment.

// FPGA ecc_sel encoding of the 8->16 design (nascerr_8to16_legacy/fpga/2024-08_fpga_1memory_8to16,
// hdl/hamming_top_design.sv). The 8-bit firmware drives it as
// ECC_MOD1/2/3 = Hamming/TBEC/EDAC.
typedef enum bit [1:0] {
  ECC_BYPASS  = 2'b00,
  ECC_HAMMING = 2'b01,
  ECC_TBEC    = 2'b10,
  ECC_EDAC    = 2'b11
} ecc_mode_e;

// Virtual interface types (must match the parameters used in tb/dyn_ecc_tb.sv).
typedef virtual sideband_if #(.WWIDTH(2), .RWIDTH(1)) mux_vif_t; // wsig = ecc_sel, rsig = erro
typedef virtual sram_backdoor_if                      sram_bd_vif_t;

// Golden encoders: the 8-bit firmware's own C functions (NASCERR_ECCs.c),
// compiled unmodified and linked through DPI-C (see Makefile / dpi/).
// C signature: uint16_t f(uint16_t).
import "DPI-C" pure function shortint unsigned nascerr_eccs_hamming_encoder(input shortint unsigned data_in);
import "DPI-C" pure function shortint unsigned nascerr_eccs_tbec_encoder   (input shortint unsigned data_in);
import "DPI-C" pure function shortint unsigned nascerr_eccs_edac_encoder   (input shortint unsigned data_in);

// Codeword the DUT should store in the SRAM for `data` written in `mode`.
function automatic bit [15:0] dyn_ecc_encode(ecc_mode_e mode, bit [15:0] data);
  case (mode)
    ECC_BYPASS : return data;
    ECC_HAMMING: return nascerr_eccs_hamming_encoder(data);
    ECC_TBEC   : return nascerr_eccs_tbec_encoder(data);
    ECC_EDAC   : return nascerr_eccs_edac_encoder(data);
  endcase
endfunction
