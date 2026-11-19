module EDACdecoder(input logic[15:0] data_in, 
               output logic [15:0] data_out
              );
  
  

  logic [7:0] W;
  logic [15:0] V;
  logic [15:0] V1;
  logic [15:0] SV1;


  always_comb
begin
  
integer b;
integer sumP1;
integer sumP2;

  W[7:0]=data_in[15:8];
  V1[7:0]= data_in[7:0];
  
  V[0] = W[2]^W[4]^W[6];
  V[1] = W[7]^W[0];
  V[2] = W[2]^W[4]^W[5];
  V[3] = W[7]^W[1]^W[2]^W[5]^W[6];
  V[4] = W[2]^W[4]^W[7]^W[0];
  V[5] = W[7]^W[6]^W[2]^W[5]^W[4];
  V[6] = W[6]^W[3]^W[4]^W[5];//
  V[7] = W[7]^W[1]^W[4]^W[5]^W[6]^W[3];//
  
  V[8] = V[0]^V[4]^V[5]^V[2];
  V[9] = V[0]^V[1]^V[2]^V[4]^V[5]^V[6]^V[7];
  V[10] = V[0]^V[2]^V[3]^V[6]^V[7];
  V[11] = V[1]^V[3]^V[4]^V[7];// 
  V[12] = V[0]^V[1]^V[2]^V[3]^V[4]^V[6]^V[7];
  V[13] = V[1]^V[2]^V[4];
  V[14] = V[0]^V[1]^V[4];
  V[15] = V[0]^V[1]^V[4]^V[5]^V[2];
  
  V1[8] = V1[0]^V1[4]^V1[5]^V1[2];
  V1[9] = V1[0]^V1[1]^V1[2]^V1[4]^V1[5]^V1[6]^V1[7];
  V1[10] = V1[0]^V1[2]^V1[3]^V1[6]^V1[7];
  V1[11] = V1[1]^V1[3]^V1[4]^V1[7];// 
  V1[12] = V1[0]^V1[1]^V1[2]^V1[3]^V1[4]^V1[6]^V1[7];
  V1[13] = V1[1]^V1[2]^V1[4];
  V1[14] = V1[0]^V1[1]^V1[4];
  V1[15] = V1[0]^V1[1]^V1[4]^V1[5]^V1[2];
  
  
  SV1[0] = data_in[0]^V[0];
  SV1[1] = data_in[1]^V[1];
  SV1[2] = data_in[2]^V[2];
  SV1[3] = data_in[3]^V[3];
  SV1[4] = data_in[4]^V[4];
  SV1[5] = data_in[5]^V[5];
  SV1[6] = data_in[6]^V[6];
  SV1[7] = data_in[7]^V[7];
  
  SV1[8] = data_in[8]^V1[8];
  SV1[9] = data_in[9]^V1[9];
  SV1[10] = data_in[10]^V1[10];
  SV1[11] = data_in[11]^V1[11];
  SV1[12] = data_in[12]^V1[12];
  SV1[13] = data_in[13]^V1[13];
  SV1[14] = data_in[14]^V1[14];
  SV1[15] = data_in[15]^V1[15];
  
  sumP1 = 0;
  sumP2 = 0;
  
  sumP1 = SV1[0]+SV1[1]+SV1[2]+SV1[3]+SV1[4]+SV1[5]+SV1[6]+SV1[7];
  sumP2 = SV1[10]+SV1[11]+SV1[12]+SV1[13]+SV1[14]+SV1[15]+SV1[9]+SV1[8];
  
  
  
      //flag = 3'b000;
  if(SV1[15:0] != 4'h0000)// condiçao para erros em quadrantes
    begin
        
      if(sumP1 > sumP2)// Erro no segundo quadrante
        begin
          //flag = flag | 3'b001;
          for(b=0;b<8;b=b+1)
            begin
              if(SV1[b+8]!=1'b0)
                V[b+8]=V1[b+8];
            end
        end
        
      else if(sumP1 < sumP2)// Erro no primeiro quadrante
        begin
           //flag = flag | 3'b100;
          for(b=0;b<8;b=b+1)  // pas vraiment utile 
            begin
              if(SV1[b]!=1'b0)
                V[b]=V[b]^SV1[b];
            end
        end
      
    end
end
  assign data_out= {8'b00000000,V[15:8]};

 
endmodule
