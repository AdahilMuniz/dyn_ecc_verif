//top_level with tests for 16 bits inout

`include "combinational.sv"
//`include "Bloc_error.sv"

module top_design(inout wire [15:0] mcu_fpga_io,
                  inout wire [15:0] fpga_mem_io,
                  
                  output wire [15:0] dataerror,
                  output wire [15:0] datasanserror,
                  output wire [15:0] outdecode,
                  
                  input logic      sel,
                  input logic 	   write_enable
                   );
  
  logic [7:0] encoder_input;
  logic [7:0] decoder_output;
  logic [15:0] encoder_output;
  logic [15:0] decoder_input;
  
  logic [15:0] data_error;
  logic [15:0] memory_data;
  
  
  //buffer tristate MCU-FPGA
  assign mcu_fpga_io = ~ write_enable ? {8'b00000000,decoder_output[7:0]} : 16'bz;
  assign encoder_input = mcu_fpga_io[7:0];
  assign outdecode = {8'b00000000,decoder_output[7:0]};


  
  //buffer tristate FPGA-MEMÓRIA
  assign memory_data = write_enable ? encoder_output : 16'bz;
  //assign decoder_input = memory_data;
  assign decoder_input = data_error;

  
  //addition of the error
  //assign fpga_mem_io = memory_data;
  assign datasanserror = memory_data;
  assign dataerror = memory_data^(1<<10);
  assign data_error = memory_data ^(1<<10);
  assign fpga_mem_io = data_error;
  
 
  
  combinacional bloco(.data_out_encoder(encoder_output),
                      .data_out_decoder(decoder_output),
                      .data_in_decoder(decoder_input),
                      .data_in_encoder(encoder_input), 
                      .sel(sel)
                    );
  
  
endmodule