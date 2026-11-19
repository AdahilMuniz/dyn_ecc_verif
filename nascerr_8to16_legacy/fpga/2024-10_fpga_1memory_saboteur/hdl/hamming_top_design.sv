//`include "hamming_encoder.sv"
//`include "hamming_decoder.sv"
`include "Encoder16bits.sv"
`include "Encoder16bits_random.sv"
`include "Decoder16bits.sv"
`include "EDACencoder.sv"
`include "EDACdecoder.sv"

module hamming_top_design(input logic [15:0] data_in_left,
                          output logic [15:0] data_out_left,
                          input logic [15:0] data_in_right,
                          output logic [15:0] data_out_right,
                          input logic [1:0] sel,
                          input logic [2:0]   random_value);
  
 
  logic [15:0] decoder_output1;
  logic [15:0] encoder_output1;
  logic [15:0] decoder_output2;
  logic [15:0] encoder_output2;
  logic [15:0] decoder_output3;
  logic [15:0] encoder_output3;

  
  logic [3:0] parity;
  logic [3:0] final_verify_bit;
  
  //hamming_encoder codificador(.data_in(data_in_left),.encoded_data(encoder_output1),.parity_bits(parity));
  //hamming_decoder decodificador (.encoded_data(data_in_right), .decoded_data(decoder_output1),.verify_bit(final_verify_bit));
  
  Encoder16bits codificador2(.data_in(data_in_left),.data_out(encoder_output1));
  Decoder16bits decodificador2 (.data_in(data_in_right), .data_out(decoder_output1));
  
  Encoder16bits_random codificador2_random(.data_in(data_in_left),.data_out(encoder_output2),.random_value(random_value));
    
  
  EDACencoder codificador3(.data_in(data_in_left),.data_out(encoder_output3));
  EDACdecoder decodificador3 (.data_in(data_in_right), .data_out(decoder_output3));
  
  
  always_comb begin
    case (sel)
        2'b00 : begin //BYPASS
            data_out_right =  data_in_left;
            data_out_left = data_in_right;
                end
        2'b01 : begin //TBEC_RSC
            data_out_right = encoder_output1;
            data_out_left =  decoder_output1;
                end
        2'b10 : begin //TBEC_RSC_random
            data_out_right = encoder_output2;
            data_out_left =  decoder_output1;
                end
        default : begin //ECC3 EADC 
            data_out_right = encoder_output3;
            data_out_left =  decoder_output3;
                end
                
        
    endcase
    
   
  end
  
  
    
  
endmodule