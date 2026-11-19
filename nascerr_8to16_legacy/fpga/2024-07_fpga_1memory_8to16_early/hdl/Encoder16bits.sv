//Encoder for 16 bits inout

 module Encoder16bits( input logic [15:0] data_in,
                        output logic [15:0] data_out);

     
  logic [1:0] signal [3:0];
  logic Di[1:0];
  logic P[1:0];
  logic [1:0] Cb[1:0];
  
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
  
  
  Cb[0][0]=signal[0][0]^signal[2][0]; //AC1
  Cb[0][1]=signal[1][0]^signal[3][0]; //BD2
  Cb[1][0]=signal[0][1]^signal[2][1]; //AC3
  Cb[1][1]=signal[1][1]^signal[3][1]; //BD4
    
    
    P[0]=signal[0][0]^signal[0][1]^signal[1][0]^signal[1][1];
    P[1]=signal[2][0]^signal[2][1]^signal[3][0]^signal[3][1];
  
    Di[0]=signal[0][0]^signal[1][1]^signal[2][0]^signal[3][1];
    Di[1]=signal[0][1]^signal[1][0]^signal[2][1]^signal[3][0];
  
end
  assign data_out={Cb[1][1],
                   Cb[1][0],
                   Cb[0][1],
                   Cb[0][0],
                   P[1],
                   P[0],
                   Di[1],
                   Di[0],
                   signal[3][1],signal[2][1],signal[1][1],signal[0][1],signal[3][0],signal[2][0],signal[1][0],signal[0][0] };
 
endmodule
