`include "hamming_top_design.sv"

module fpga_top_design( inout wire [15:0] mcu_fpga_io,
                        inout wire [15:0] fpga_mem_io,
                        input logic 	    write_en,
                        input logic 	    chip_sel,
                        input logic [1:0]   ecc_sel,
                        input logic [2:0]   random_value);

 
  logic [15:0] encoder_input;
  logic [15:0] decoder_output;
  logic [15:0] encoder_output;
  logic [15:0] decoder_input;
  
  //buffer tristate MCU-FPGA
  assign mcu_fpga_io = (write_en == 0 && chip_sel == 0)? 16'bz: decoder_output;
  assign encoder_input = mcu_fpga_io;
    
  //buffer tristate FPGA-MEMÓRIA
  assign fpga_mem_io = (write_en == 0 && chip_sel == 0)? encoder_output : 16'bz;
  assign decoder_input = (write_en == 0 && chip_sel == 0)? 16'bz : fpga_mem_io;

  
  
  
    hamming_top_design modulo_hamming(  .data_in_left(encoder_input),
                                        .data_out_left(decoder_output),
                                        .data_in_right(decoder_input),
                                        .data_out_right(encoder_output),
                                        .sel(ecc_sel),
                                        .random_value(random_value));
                                        
                                        
    
   
endmodule