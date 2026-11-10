set_device -family {SmartFusion2} -die {M2S025T} -speed {STD}
read_verilog -mode system_verilog {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\hdl\MRSC_decoder.sv}
read_verilog -mode system_verilog {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\hdl\MRSC_encoder.sv}
read_verilog -mode system_verilog {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\hdl\TBEC_RSC_decoder.sv}
read_verilog -mode system_verilog {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\hdl\TBEC_RSC_encoder.sv}
 add_include_path  {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\hdl}
 add_include_path  {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\hdl}
 add_include_path  {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\hdl}
 add_include_path  {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\hdl}
read_verilog -mode system_verilog {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\hdl\ecc_design.sv}
 add_include_path  {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\hdl}
read_verilog -mode system_verilog {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\hdl\design.sv}
read_verilog -mode system_verilog {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\component\work\prj_2_memory_sb\prj_2_memory_sb.v}
set_top_level {prj_2_memory_sb}
map_netlist
check_constraints {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\constraint\synthesis_sdc_errors.log}
write_fdc {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\designer\prj_2_memory_sb\synthesis.fdc}
