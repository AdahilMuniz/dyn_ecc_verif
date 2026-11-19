//top_level without tests for 16 bits inout

`include "combinational16bits.sv"
//`include "Bloc_error.sv"

module top_design(inout wire [15:0] mcu_fpga_io,
                  inout wire [15:0] fpga_mem_io,
                  input logic      sel,
                  input logic 	   write_enable
                   );
  
  logic [15:0] encoder_input;
  logic [15:0] decoder_output;
  logic [15:0] encoder_output;
  logic [15:0] decoder_input;
  
  logic [15:0] memory_data;
  
  
  //buffer tristate MCU-FPGA
  assign mcu_fpga_io = ~ write_enable ? decoder_output : 16'bz;
  assign encoder_input = mcu_fpga_io;

  
  //buffer tristate FPGA-MEMÓRIA
  assign memory_data = write_enable ? encoder_output : 16'bz;
  assign decoder_input = memory_data;

  
  //without addition of the error
  assign fpga_mem_io = memory_data;
  
 
  
  combinacional bloco(.data_out_encoder(encoder_output),
                      .data_out_decoder(decoder_output),
                      .data_in_decoder(decoder_input),
                      .data_in_encoder(encoder_input), 
                      .sel(sel)
                    );
  
  
endmodule