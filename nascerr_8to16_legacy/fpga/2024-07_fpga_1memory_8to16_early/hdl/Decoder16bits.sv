//Decoder for 16 bits inout


module Decoder16bits(input logic[15:0] data_in, output logic [15:0] data_out);
  
  logic [7:0] signal;
  logic [7:0] Cb;
  logic [3:0] SX;
  logic [1:0] SD;
  logic [1:0] SP;
  logic [2:0] flag;
  
  
  always_comb
    begin
      
      integer b;
      integer sumSP;
      integer sumSD;
      integer sumSX;
      integer part1;
      integer part2;
      
      signal[0] = data_in[2];
      signal[1] = data_in[1];
      signal[2] = data_in[2];
      signal[3] = data_in[3];
      signal[4] = data_in[4];
      signal[5] = data_in[5];
      signal[6] = data_in[6];
      signal[7] = data_in[7];
      
      
      Cb[0] = data_in[8];
      Cb[1] = data_in[9];
      Cb[2] = data_in[10];
      Cb[3] = data_in[11];
      Cb[4] = data_in[12];
      Cb[5] = data_in[13];
      Cb[6] = data_in[14];
      Cb[7] = data_in[15];
                     
      
      SX[0]=signal[0]^signal[2]^Cb[4]; //AC1
      SX[1]=signal[1]^signal[3]^Cb[5]; //BD2
      SX[2]=signal[4]^signal[6]^Cb[6]; //AC3
      SX[3]=signal[5]^signal[7]^Cb[7]; //BD4
      
      
      SP[0]=signal[0]^signal[4]^signal[1]^signal[5]^Cb[2];
      SP[1]=signal[2]^signal[3]^signal[6]^signal[7]^Cb[3];
      
      SD[0]=signal[0]^signal[2]^signal[7]^signal[5]^Cb[0];
      SD[1]=signal[1]^signal[3]^signal[4]^signal[6]^Cb[1];
      
  
      sumSP = 1*SP[0] + 1*SP[1];
      sumSD = 1*SD[0] + 1*SD[1];
      sumSX = 1*SX[0] + 1*SX[1] + 1*SX[2] + 1*SX[3];
      
      flag = 3'b000;
      part1 = 1*SP[0];
      part2 = 1*SP[1];
      
      if((SP != 2'b00) || (SD != 2'b00) || (sumSX>1))
      begin
          
          if(part1 > part2)
          begin
            for(b=0;b<2;b=b+1)
            begin
              if({SX[b],SX[b+2]} != 2'b00)
              begin     // Quand b=0 => ligne 0 et AC
                  		// Quand b=1 => ligne 1 et BD
                 signal[b] = signal[b]^SX[b]; 
                 signal[b+4] = signal[b+4]^SX[b+2]; 
                 flag = 3'b001;
              end
            end
          end
        else if(part1 < part2)
          begin
            for(b=2;b<4;b=b+1)
            begin
              if({SX[b-2],SX[b]} != 2'b00)
              begin
                		// Quand b=2 => ligne 2 et AC
                  		// Quand b=3 => ligne 3 et BD
                signal[b] = signal[b]^SX[b-2]; 
                 signal[b+4] = signal[b+4]^SX[b]; 
                 flag = 3'b010;
              end
            end
          end
        
        else if((part1 == part2) && ({SD[0],SD[1],SP[0],SP[1]} != 4'b0000))
          begin
            for(b=1;b<3;b=b+1)
            begin
              if({SX[b-1],SX[b+1]} != 2'b00)
              begin
                		// Quand b=1 => ligne 1 et AC
                  		// Quand b=2 => ligne 2 et BD
                signal[b] = signal[b]^SX[b-1]; 
                signal[b+4] = signal[b+4]^SX[b+1]; 
                 flag = 3'b100;
              end
            end
          end
        end
    end
      
  assign sig = flag;

  assign data_out = {8'b00000000, signal[7:0]};
      
endmodule
