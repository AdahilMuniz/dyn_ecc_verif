`include "hamming_encoder.sv"
`include "hamming_decoder.sv"
`include "Encoder16bits.sv"
`include "Decoder16bits.sv"
`include "EDACencoder.sv"
`include "EDACdecoder.sv"

module hamming_top_design(input logic [15:0] data_in_left,
                          output logic [15:0] data_out_left,
                          input logic [15:0] data_in_right,
                          output logic [15:0] data_out_right,
                          input logic [1:0] sel,
                          output logic      erro );
  
 
  logic [15:0] decoder_output1;
  logic [15:0] encoder_output1;
  logic [15:0] decoder_output2;
  logic [15:0] encoder_output2;
  logic [15:0] decoder_output3;
  logic [15:0] encoder_output3;

  
  logic [3:0] parity;
  logic [3:0] final_verify_bit;
  logic reg_erro;
  
  hamming_encoder codificador(.data_in(data_in_left),.encoded_data(encoder_output1),.parity_bits(parity));
  hamming_decoder decodificador (.encoded_data(data_in_right), .decoded_data(decoder_output1),.verify_bit(final_verify_bit));
  
  Encoder16bits codificador2(.data_in(data_in_left),.data_out(encoder_output2));
  Decoder16bits decodificador2 (.data_in(data_in_right), .data_out(decoder_output2));
  
  EDACencoder codificador3(.data_in(data_in_left),.data_out(encoder_output3));
  EDACdecoder decodificador3 (.data_in(data_in_right), .data_out(decoder_output3));
  
  
  always_comb begin
    case (sel)
        2'b00 : begin //BYPASS
            data_out_right =  data_in_left;
            data_out_left = data_in_right;
                end
        2'b01 : begin //ECC1 HAMMING
            data_out_right = encoder_output1;
            data_out_left =  decoder_output1;
                end
        2'b10 : begin //ECC2 TBEC_RSC 8 bits
            data_out_right = encoder_output2;
            data_out_left =  decoder_output2;
                end
        default : begin //ECC3 EADC (falta colocar)
            data_out_right = encoder_output3;
            data_out_left =  decoder_output3;
                end
        
    endcase
    
    if(final_verify_bit == 0) begin
        reg_erro = 0;
    end
    else begin
        reg_erro = 1;
    end
  end
  
  assign erro = reg_erro;

 
  
    
  
endmodule