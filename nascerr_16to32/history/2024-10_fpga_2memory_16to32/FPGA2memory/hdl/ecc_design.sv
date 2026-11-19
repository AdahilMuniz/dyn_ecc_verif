`include "TBEC_RSC_encoder.sv"
`include "TBEC_RSC_decoder.sv"
`include "MRSC_encoder.sv"
`include "MRSC_decoder.sv"
//`include "EDACencoder.sv"
//`include "EDACdecoder.sv"

module ecc_design(input logic [0:15] data_in_left, // entrada FPGA pelo processador
                  output logic [0:15] data_out_right_up, // saída memória 1
                  output logic [0:15] data_out_right_down, // saída memória 2
                  input logic [0:15] data_in_right_up, // entrada memória 1
                  input logic [0:15] data_in_right_down, // entrada memória 2
                  output logic [0:15] data_out_left, // saida FPGA para o processador
                  input logic [1:0] sel);             // seletor ecc/bypass
  
 
  logic [0:15] decoder_output1;
  logic [0:31] encoder_output1;
  logic [0:15] decoder_output2;
  logic [0:31] encoder_output2;
  
  //logic [15:0] decoder_output3;
  //logic [15:0] encoder_output3;

  
  
  TBEC_RSC_encoder codificador1(.data_in(data_in_left),.data_out(encoder_output1));
  TBEC_RSC_decoder decodificador1 (.data_in({data_in_right_up,data_in_right_down}), .data_out(decoder_output1));
  
  MRSC_encoder codificador2(.data_in(data_in_left),.data_out(encoder_output2));
  MRSC_decoder decodificador2 (.data_in({data_in_right_up,data_in_right_down}), .data_out(decoder_output2));
  
  
  always_comb begin
    case (sel)
        2'b00 : begin //BYPASS memória 1
            data_out_right_up =  data_in_left;
            data_out_left = data_in_right_up; 	   
                end
        2'b01 : begin //BYPASS memória 2
            data_out_right_down =  data_in_left;
            data_out_left  = data_in_right_down; 
                end
        2'b10 : begin //ECC1 TBEC_RSC
          	
            data_out_right_up = encoder_output1[0:15];
            data_out_right_down = encoder_output1[16:31];
          
            data_out_left =  decoder_output1;
                end
        default : begin //ECC2 MRSC
            data_out_right_up = encoder_output2[0:15];
            data_out_right_down = encoder_output2[16:31];
          
            data_out_left =  decoder_output2;
                end
        
    endcase
    
   
  end
  
  
    
  
endmodule