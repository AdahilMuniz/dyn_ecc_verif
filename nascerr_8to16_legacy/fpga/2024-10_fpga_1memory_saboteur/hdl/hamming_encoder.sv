module hamming_encoder (
  input logic [15:0] data_in,   // Dados de entrada de 8 bits
  output logic [15:0] encoded_data,  // Dados codificados de 16 bits
    output logic [3:0] parity_bits  // Bits de paridade
);

    //logic [3:0] parity;

    always_comb begin
        // Calcula os bits de paridade
        parity_bits[0] = data_in[0] ^ data_in[1] ^ data_in[3] ^ data_in[4] ^ data_in[6];
        parity_bits[1] = data_in[0] ^ data_in[2] ^ data_in[3] ^ data_in[5] ^ data_in[6];
        parity_bits[2] = data_in[1] ^ data_in[2] ^ data_in[3] ^ data_in[7];
        parity_bits[3] = data_in[4] ^ data_in[5] ^ data_in[6] ^ data_in[7];

        // Cria os dados codificados com os bits de paridade
        encoded_data[0] = parity_bits[0];
        encoded_data[1] = parity_bits[1];
        encoded_data[2] = data_in[0];
        encoded_data[3] = parity_bits[2];
        encoded_data[4] = data_in[1];
        encoded_data[5] = data_in[2];
        encoded_data[6] = data_in[3];
        encoded_data[7] = parity_bits[3];
        encoded_data[8] = data_in[4];
        encoded_data[9] = data_in[5];
        encoded_data[10] = data_in[6];
        encoded_data[11] = data_in[7];
        encoded_data[15:12] = 0;
      


    end
	

endmodule