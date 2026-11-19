`include "Encoder.sv"
`include "Decoder.sv"

module combinacional(output wire [15:0]data_out_encoder,
                     output wire [7:0]data_out_decoder,
                     input wire [15:0]data_in_decoder,
                     input wire [7:0]data_in_encoder,
                     input logic sel
                    );
  
  
  logic [15:0]codified_data1;
  logic [15:0]codified_data2;


  logic [7:0]data_out_temp1;
  logic [7:0]data_out_temp2;
  
  
  //seletores de caminhos de dados para o ECC seleciondo
  assign data_out_encoder = sel ? codified_data1 : codified_data2;
  assign data_out_decoder = sel ? data_out_temp1 : data_out_temp2;
 
  
// instanciação dos módulos
  TBEC_RSC_encoder encoder_0(.data_in(data_in_encoder),.data_out(codified_data1));
  
  TBEC_RSC_decoder decoder_0(.data_in(data_in_decoder),.data_out(data_out_temp1));


endmodule
