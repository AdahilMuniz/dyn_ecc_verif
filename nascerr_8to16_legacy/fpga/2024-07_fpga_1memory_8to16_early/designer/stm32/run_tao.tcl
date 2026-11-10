set_device -family {SmartFusion2} -die {M2S005} -speed {-1}
read_verilog -mode system_verilog {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl\Decoder16bits.sv}
read_verilog -mode system_verilog {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl\EDACdecoder.sv}
read_verilog -mode system_verilog {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl\EDACencoder.sv}
read_verilog -mode system_verilog {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl\Encoder16bits.sv}
read_verilog -mode system_verilog {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl\hamming_decoder.sv}
read_verilog -mode system_verilog {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl\hamming_encoder.sv}
 add_include_path  {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl}
 add_include_path  {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl}
 add_include_path  {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl}
 add_include_path  {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl}
 add_include_path  {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl}
 add_include_path  {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl}
read_verilog -mode system_verilog {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl\hamming_top_design.sv}
 add_include_path  {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl}
read_verilog -mode system_verilog {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\hdl\fpga_top_design.sv}
read_verilog -mode system_verilog {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\component\work\stm32\stm32.v}
set_top_level {stm32}
map_netlist
check_constraints {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\constraint\synthesis_sdc_errors.log}
write_fdc {C:\Users\Lesc\Desktop\ECC\Libero\stm32_test_memory - Copy\designer\stm32\synthesis.fdc}
