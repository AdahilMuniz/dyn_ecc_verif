// Top package for the dyn_ecc UVM environment: bundles the shared types and
// golden model, the sequences, the scoreboard, the environment and the base
// test. Requires +incdir+seqs +incdir+env +incdir+tests, must be compiled after
// sram_pkg and the interfaces (sideband_if, sram_backdoor_if), and needs the
// firmware's NASCERR_ECCs.c linked for the DPI-C encoders (see Makefile).
`ifndef DYN_ECC_TEST_PKG_SV
`define DYN_ECC_TEST_PKG_SV

package dyn_ecc_test_pkg;

  import uvm_pkg::*;
  `include "uvm_macros.svh"
  import sram_pkg::*;

  // Types + golden model
  `include "dyn_ecc_types.sv"
  // Sequences
  `include "dyn_ecc_base_seq.sv"
  `include "write_read_seq.sv"
  `include "ecc_pattern_seq.sv"
  `include "ecc_single_err_seq.sv"
  `include "ecc_burst_err_seq.sv"
  // Environment
  `include "dyn_ecc_scoreboard.sv"
  `include "dyn_ecc_env.sv"
  `include "dyn_ecc_base_test.sv"

endpackage : dyn_ecc_test_pkg

`endif // DYN_ECC_TEST_PKG_SV
