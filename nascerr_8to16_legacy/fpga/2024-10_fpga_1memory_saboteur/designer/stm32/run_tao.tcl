set_device -family {SmartFusion2} -die {M2S005} -speed {-1}
read_verilog -mode system_verilog {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\component\work\OSC_C0\OSC_C0_0\OSC_C0_OSC_C0_0_OSC.v}
read_verilog -mode system_verilog {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\component\work\OSC_C0\OSC_C0.v}
read_verilog -mode system_verilog {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl\Decoder16bits.sv}
read_verilog -mode system_verilog {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl\EDACdecoder.sv}
read_verilog -mode system_verilog {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl\EDACencoder.sv}
read_verilog -mode system_verilog {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl\Encoder16bits_random.sv}
read_verilog -mode system_verilog {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl\Encoder16bits.sv}
 add_include_path  {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl}
 add_include_path  {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl}
 add_include_path  {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl}
 add_include_path  {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl}
 add_include_path  {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl}
read_verilog -mode system_verilog {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl\hamming_top_design.sv}
 add_include_path  {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl}
read_verilog -mode system_verilog {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl\fpga_top_design.sv}
read_verilog -mode system_verilog {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\hdl\lfsr_random_generator.sv}
read_verilog -mode system_verilog {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\component\work\stm32\stm32.v}
set_top_level {stm32}
map_netlist
check_constraints {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\constraint\synthesis_sdc_errors.log}
write_fdc {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\designer\stm32\synthesis.fdc}
