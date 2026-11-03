// Top package for the dyn_ecc UVM environment: bundles the base sequence, the
// environment and the base test. Requires +incdir+seqs +incdir+env +incdir+tests
// and must be compiled after sram_pkg.
`ifndef DYN_ECC_TEST_PKG_SV
`define DYN_ECC_TEST_PKG_SV

package dyn_ecc_test_pkg;

  import uvm_pkg::*;
  `include "uvm_macros.svh"
  import sram_pkg::*;

  // Sequences
  `include "dyn_ecc_base_seq.sv"
  `include "write_read_seq.sv"
  // Environment
  `include "dyn_ecc_env.sv"
  `include "dyn_ecc_base_test.sv"

endpackage : dyn_ecc_test_pkg

`endif // DYN_ECC_TEST_PKG_SV
