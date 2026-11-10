`include "encoder.v"
`include "decoder.v"

module combinacional(inout wire [15:0]data_out, 
                     inout wire [15:0]data_out1,
                     inout wire [15:0]data_out2,
                     input logic data_write_en);
  
  
  logic [31:0]codified_data1;
  logic [31:0]codified_data2;
  logic data_out_en;
  
  logic [15:0]data_out_temp; 
  logic [15:0]in_mem1;
  logic [15:0]in_mem2;
  logic [15:0]data_in_temp; 
  logic [15:0]out_mem1;
  logic [15:0]out_mem2;
  logic encoder_en;
  logic decoder_en;
  
  assign data_out = data_out_en ? data_out_temp : 16'bz;
  assign data_in_temp = data_out;
  
  assign data_out1 = data_write_en ? out_mem1 : 16'bz;
  assign in_mem1 = data_out1;
  
  assign data_out2 = data_write_en ? out_mem2 : 16'bz;
  assign in_mem2 = data_out2;
  
  
  encoder codificador(.data_in(data_in_temp),.data_out(codified_data1),.en(encoder_en));
  decoder decodificador(.data_in(codified_data2),.data_out(data_out_temp),.en(decoder_en));
  
  splitter separador(.data_in(codified_data1),.data_out1(out_mem1),.data_out2(out_mem2));
  joiner juntador(.data_out(codified_data2),.data_in1(in_mem1),.data_in2(in_mem2));

  always@(data_out_en) begin
    if(data_out_en) begin 
      	decoder_en = 1;
        #0.1
    	decoder_en = ~ decoder_en;
    end
    else begin
        encoder_en = 1;
      	#0.1
    	encoder_en = ~ encoder_en;
    end
  end
  	
  
  assign data_out_en = ~data_write_en;
  
endmodule

module splitter(input logic [31:0]data_in,
                output logic [31:0]data_out1,
                output logic [31:0]data_out2);
  
  assign data_out1 = data_in[15:0];
  assign data_out2 = data_in[31:16];
  
endmodule

module joiner(output logic [31:0]data_out,
              input logic [15:0]data_in1,
              input logic [15:0]data_in2);
  
  assign data_out = {data_in2,data_in1};
  
endmodule