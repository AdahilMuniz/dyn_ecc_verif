module EDACencoder( input logic [15:0] data_in,
               output logic [15:0] data_out);
     
logic [7:0] W;
  logic [15:0] V;
  
  assign W = data_in[7:0];

always_comb
begin
  
  V[0] = W[2]^W[4]^W[6];
  V[1] = W[7]^W[0];
  V[2] = W[2]^W[4]^W[5];
  V[3] = W[7]^W[1]^W[2]^W[5]^W[6];
  V[4] = W[2]^W[4]^W[7]^W[0];
  V[5] = W[7]^W[6]^W[2]^W[5]^W[4];
  V[6] = W[6]^W[3]^W[4]^W[5];
  V[7] = W[7]^W[1]^W[4]^W[5]^W[6]^W[3];
  
  V[8] = V[0]^V[4]^V[5]^V[2];
  V[9] = V[0]^V[1]^V[2]^V[4]^V[5]^V[6]^V[7];
  V[10] = V[0]^V[2]^V[3]^V[6]^V[7];
  V[11] = V[1]^V[3]^V[4]^V[7];
  V[12] = V[0]^V[1]^V[2]^V[3]^V[4]^V[6]^V[7];
  V[13] = V[1]^V[2]^V[4];
  V[14] = V[0]^V[1]^V[4];
  V[15] = V[0]^V[1]^V[4]^V[5]^V[2];

end
  
  
  assign data_out={V[15], V[14], V[13], V[12], V[11], V[10], V[9], V[8], V[7], V[6], V[5], V[4], V[3], V[2], V[1], V[0]};
 
endmodule

