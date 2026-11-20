// Top package for the dyn_ecc UVM environment: bundles the base sequence, the
// environment and the base test. Requires +incdir+seqs +incdir+env +incdir+tests
// and must be compiled after sram_pkg.
`ifndef DYN_ECC_TEST_PKG_SV
`define DYN_ECC_TEST_PKG_SV

package dyn_ecc_test_pkg;

  import uvm_pkg::*;
  `include "uvm_macros.svh"
  import sram_pkg::*;
  import sideband_pkg::*;
  import typedefs::*;

  parameter DEC_WORD_WIDTH = 8;
  parameter ENC_WORD_WIDTH = 2*DEC_WORD_WIDTH;

  // Virtual sequencer (needed by the sequences' p_sequencer declaration).
  `include "dyn_ecc_virtual_sequencer.sv"
  // Sequences
  `include "dyn_ecc_base_seq.sv"
  `include "write_read_seq.sv"
  `include "write_error_read_seq.sv"
  `include "tbec_rsc_all_patterns_seq.sv"
  // Environment
  `include "dyn_ecc_scbd.sv"
  `include "dyn_ecc_env.sv"
  `include "dyn_ecc_base_test.sv"

endpackage : dyn_ecc_test_pkg

`endif // DYN_ECC_TEST_PKG_SV
