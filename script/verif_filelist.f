// =============================================================================
// filelist.f - top VCS compile file list for the dyn_ecc UVM environment
//   Usage (from repo root):
//     vcs -full64 -sverilog -ntb_opts uvm-1.2 -f filelist.f -o simv
// =============================================================================

// -----------------------------------------------------------------------------
// DUT source lists (both DUTs; the TB generate-selects one by DATA_WIDTH).
// -----------------------------------------------------------------------------
-f script/design_filelist.f
-f script/nascerr_filelist.f

// -----------------------------------------------------------------------------
// Interfaces (compiled before the packages/top that reference them).
// -----------------------------------------------------------------------------
+incdir+vips/sram
+incdir+vips/sideband
vips/sram/sram_if.sv
tb/sideband_if.sv

// -----------------------------------------------------------------------------
// Verification IP packages.
// -----------------------------------------------------------------------------
vips/sram/sram_pkg.sv
vips/sideband/sideband_pkg.sv

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
