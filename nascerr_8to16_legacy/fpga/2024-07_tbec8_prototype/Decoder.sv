//`timescale 1ns / 1ps
module TBEC_RSC_decoder(input logic[0:15] data_in, output logic [0:7] data_out);
  logic [0:1] signal [0:3];
  logic [0:7]red;
  logic [0:1] linsin [0:1];
  logic [0:1] SP;
  logic [0:1] SDi;
logic [0:2] flag;

  always_comb
begin
  
  integer b;
integer sumSL=0;
integer sumSP=0;
integer sumSDi=0;
integer quad1=0;
integer quad2=0;
  
  signal[0][0:1]={data_in[0],data_in[4]};
  signal[1][0:1]={data_in[1],data_in[5]};
  signal[2][0:1]={data_in[2],data_in[6]};
  signal[3][0:1]={data_in[3],data_in[7]};
  
  red[0:7]=data_in[8:15];
    linsin[0][0]=signal[0][0]^signal[2][0]^red[4];
    linsin[1][0]=signal[0][1]^signal[2][1]^red[6];
    linsin[0][1]=signal[1][0]^signal[3][0]^red[5];
    linsin[1][1]=signal[1][1]^signal[3][1]^red[7];
    
        
    SP[0]=signal[0][0]^signal[0][1]^signal[1][0]^signal[1][1]^red[2];
    SP[1]=signal[2][0]^signal[2][1]^signal[3][0]^signal[3][1]^red[3];
  
    sumSP=1*SP[0] + 1*SP[1];
  
    SDi[0]=signal[0][0]^signal[1][1]^signal[2][0]^signal[3][1]^red[0];
    SDi[1]=signal[0][1]^signal[1][0]^signal[2][1]^signal[3][0]^red[1];
    
    sumSDi=1*SDi[0] + 1*SDi[1];
    
    sumSL=1*linsin[1][0] + 1*linsin[1][1]+ 1*linsin[0][0]+ 1*linsin[0][1];
  
  
      //flag = 3'b000;
  if((((SP[0:1] !=2'b00) & (SDi[0:1] !=2'b00)) |  sumSL>1) & !((sumSDi==0) & (sumSP==1) & (sumSL>=2)))// condiçao para erros em quadrantes
    begin
        quad1=1*SP[0];
        quad2=1*SP[1];
        if(quad1 > quad2)// Erro no primeiro quadrante
        begin
          //flag = flag | 3'b100;
          for(b=0;b<2;b=b+1)
            begin
                if(linsin[b][0:1]!=2'b00)
                signal[b][0:1]=signal[b][0:1]^linsin[b][0:1];
            end
        end
        
        else if(quad1 < quad2)// Erro no segundo quadrante
        begin
           //flag = flag | 3'b001;
          for(b=2;b<4;b=b+1)
            begin
                if(linsin[b][0:1]!=2'b00)
                  signal[b][0:1]=signal[b][0:1]^linsin[b][0:1];
            end
        end
        
        else if((quad1 == quad2)&({SP[0],SP[1],SDi[0],SDi[1]} !=4'b0000)) // Erro no quadrante central
        begin
               //flag = flag | 3'b010;
          for(b=1;b<3;b=b+1)
                begin
                    if(linsin[b][0:1]!=2'b00)
                      signal[b][0:1]=signal[b][0:1]^{linsin[b][1],linsin[b][0]};
                end

        end
    end
end
  assign data_out={signal[0][0],signal[1][0],signal[2][0],signal[3][0],signal[0][1],signal[1][1],signal[2][1],signal[3][1]};

endmodule
