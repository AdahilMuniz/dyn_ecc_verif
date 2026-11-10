// dyn_ecc 8->16: shared types.
package dyn_ecc_pkg;

  // ECC mode select (decision D2): the 2024-08 FPGA map, driven by the MCU's
  // ECC-select GPIOs (ecc_sel[0] = PC0 on the 8-bit board).
  typedef enum logic [1:0] {
    ECC_BYPASS  = 2'b00,  // 16-bit pass-through, no coding
    ECC_HAMMING = 2'b01,  // Hamming(12,8), cw[15:12] = 0
    ECC_TBEC    = 2'b10,  // TBEC-RSC 8->16
    ECC_EDAC    = 2'b11   // EDAC 8->16
  } ecc_sel_t;

endpackage
