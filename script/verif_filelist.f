// =============================================================================
// verif_filelist.f - top VCS compile file list for the dyn_ecc UVM environment
//   Usage (from repo root): make compile   (see Makefile; the DPI-C golden
//   model, the firmware's NASCERR_ECCs.c, is added on the vcs command line)
// =============================================================================

// -----------------------------------------------------------------------------
// DUT source list.
// -----------------------------------------------------------------------------
-f script/design_filelist.f

// -----------------------------------------------------------------------------
// Board models: IS61WV2M16AL 2M x 16 async SRAM (own `timescale 1ns/100ps).
// -----------------------------------------------------------------------------
tb/models/IS61WV2M16AL.v

// -----------------------------------------------------------------------------
// Interfaces (compiled before the packages/top that reference them).
// -----------------------------------------------------------------------------
+incdir+vips/sram
vips/sram/sram_if.sv
tb/sideband_if.sv
tb/sram_backdoor_if.sv

// -----------------------------------------------------------------------------
// Verification IP packages.
// -----------------------------------------------------------------------------
vips/sram/sram_pkg.sv

// -----------------------------------------------------------------------------
// Environment / sequences / tests (one package, bundled).
// -----------------------------------------------------------------------------
+incdir+seqs
+incdir+env
+incdir+tests
tests/dyn_ecc_test_pkg.sv

// -----------------------------------------------------------------------------
// Testbench top.
// -----------------------------------------------------------------------------
tb/dyn_ecc_tb.sv
