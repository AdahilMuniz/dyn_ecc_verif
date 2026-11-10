//Encoder for 16 bits inout

`timescale 1ns / 1ps

module TBEC_RSC_encoder( input logic [0:15] data_in,
                         output logic [0:15] data_out);
     
  logic [0:1] signal [0:3];
  logic Di[0:1];
  logic P[0:1];
  logic [0:1] Cb[0:1];
  
always_comb
begin
  integer b;
    signal[0][0]=data_in[0];
    signal[1][0]=data_in[1];
    signal[2][0]=data_in[2];
    signal[3][0]=data_in[3];
    signal[0][1]=data_in[4];
    signal[1][1]=data_in[5];
    signal[2][1]=data_in[6];
    signal[3][1]=data_in[7];
  
  
      Cb[0][0]=signal[0][0]^signal[2][0];
  	  Cb[0][1]=signal[0][1]^signal[2][1];
      Cb[1][0]=signal[1][0]^signal[3][0];
      Cb[1][1]=signal[1][1]^signal[3][1];
    
    
    P[0]=signal[0][0]^signal[0][1]^signal[1][0]^signal[1][1];
    P[1]=signal[2][0]^signal[2][1]^signal[3][0]^signal[3][1];
  
    Di[0]=signal[0][0]^signal[1][1]^signal[2][0]^signal[3][1];
    Di[1]=signal[0][1]^signal[1][0]^signal[2][1]^signal[3][0];
  
end
  assign data_out={signal[0][0],signal[1][0],signal[2][0],signal[3][0],
                   signal[0][1],signal[1][1],signal[2][1],signal[3][1],
                   Di[0],Di[1],P[0],P[1],Cb[0][0],Cb[0][1],Cb[1][0],Cb[1][1]};
 
endmodule


