`include "ecc_design.sv"

module fpga_top_design( inout wire [15:0] mcu_fpga_io, 
                        inout wire [15:0] fpga_mem_io_up,
                        inout wire [15:0] fpga_mem_io_down,
                        input logic 	    write_en,
                        input logic [1:0]    chip_sel,
                        input logic [1:0]   ecc_sel);

 
  logic [15:0] encoder_input;
  logic [15:0] decoder_output;
  logic [15:0] encoder_output_up;
  logic [15:0] decoder_input_up;
  logic [15:0] encoder_output_down;
  logic [15:0] decoder_input_down;
  
  
  //buffer tristate MCU-FPGA
  assign mcu_fpga_io = (write_en == 0 && (chip_sel [1] == 0 || chip_sel[0] == 0))? 16'bz: decoder_output;
  assign encoder_input = mcu_fpga_io;
    
  //buffer tristate FPGA-MEMÓRIA
  assign fpga_mem_io_up = (write_en == 0 && (chip_sel [1] == 0 || chip_sel[0] == 0))? encoder_output_up : 16'bz;
  assign fpga_mem_io_down = (write_en == 0 && (chip_sel [1] == 0 || chip_sel[0] == 0))? encoder_output_down : 16'bz;
  assign decoder_input_up = (write_en == 0 && (chip_sel [1] == 0 || chip_sel[0] == 0))? 16'bz : fpga_mem_io_up;
  assign decoder_input_down = (write_en == 0 && (chip_sel [1] == 0 || chip_sel[0] == 0))? 16'bz : fpga_mem_io_down;

  

  
 ecc_design modulo(.data_in_left(encoder_input), // entrada FPGA pelo processador
                    .data_out_right_up(encoder_output_up), // saída memória 1
                    .data_out_right_down(encoder_output_down), // saída memória 2
                    .data_in_right_up(decoder_input_up), // entrada memória 1
                    .data_in_right_down(decoder_input_down), // entrada memória 2
                    .data_out_left(decoder_output), // saida FPGA para o processador
                    .sel(ecc_sel));  
   
                                                                             
    
   
endmodule