// =============================================================================
// design_filelist.f - DUT source list: the dyn_ecc 8->16 ASIC RTL (dyn_ecc_8to16/rtl)
//
// One module per file, no `include; package first, leaves before their users.
// Paths are relative to the repo root (where vcs / verilator are invoked).
// Used by `make compile` (via verif_filelist.f) and `make lint`.
// =============================================================================

dyn_ecc_8to16/rtl/dyn_ecc_pkg.sv
dyn_ecc_8to16/rtl/dyn_ecc_hamming_enc.sv
dyn_ecc_8to16/rtl/dyn_ecc_hamming_dec.sv
dyn_ecc_8to16/rtl/dyn_ecc_tbec_enc.sv
dyn_ecc_8to16/rtl/dyn_ecc_tbec_dec.sv
dyn_ecc_8to16/rtl/dyn_ecc_edac_enc.sv
dyn_ecc_8to16/rtl/dyn_ecc_edac_dec.sv
dyn_ecc_8to16/rtl/dyn_ecc_ctrl.sv
dyn_ecc_8to16/rtl/dyn_ecc_top.sv
