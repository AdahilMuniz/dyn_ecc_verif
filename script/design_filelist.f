// =============================================================================
// design_filelist.f - DUT source list (FPGA_design submodule)
//
// The HDL files no longer use `include or `timescale; every module is listed
// here explicitly. Paths are relative to the repo root (the directory from
// which vcs is invoked). Leaf modules are listed before the modules that
// instantiate them.
// =============================================================================

FPGA_design/FPGA2memory/hdl/TBEC_RSC_encoder.sv
FPGA_design/FPGA2memory/hdl/TBEC_RSC_decoder.sv
FPGA_design/FPGA2memory/hdl/MRSC_encoder.sv
FPGA_design/FPGA2memory/hdl/MRSC_decoder.sv
FPGA_design/FPGA2memory/hdl/ecc_design.sv
FPGA_design/FPGA2memory/hdl/design.sv
