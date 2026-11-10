`include "combinacional.sv"

module top_design(inout wire [15:0]mcu_fpga_io,
                  inout wire [15:0]fpga_mem1_io,
                  inout wire [15:0]fpga_mem2_io,
                  input logic      sel,
                  input logic 	   write_enable
                   );
  
  logic [15:0] encoder_input;
  logic [15:0] decoder_output;
  logic [31:0] encoder_output;
  logic [31:0] decoder_input;
  
  logic [31:0] memory_data;
  
  //buffer tristate MCU-FPGA
  assign mcu_fpga_io = ~ write_enable ? decoder_output : 16'bz;
  assign encoder_input = mcu_fpga_io;
    
  //buffer tristate FPGA-MEMORIA
  assign memory_data = write_enable ? encoder_output : 32'bz;
  assign decoder_input = memory_data;
  
  //separando as palavras de 32 bits em 16 bitas para cada memoria
  assign fpga_mem1_io = memory_data[15:0];
  assign fpga_mem2_io = memory_data[31:16];
  
  
  combinacional bloco(.data_out_encoder(encoder_output),
                      .data_out_decoder(decoder_output),
                      .data_in_decoder(decoder_input),
                      .data_in_encoder(encoder_input), 
                      .sel(sel)
                    );
  
  
endmodule