module hamming_decoder (
  input logic [15:0] encoded_data,  // Dados codificados de 16 bits
    output logic [15:0] decoded_data,   // Dados decodificados de 8 bits
    output logic [3:0] verify_bit       // Bits de erro detectados
);

	logic [11:0] received_data;

    always_comb begin
        // Calcula os bits de paridade
		received_data = encoded_data;
        verify_bit[0] = received_data[0] ^ received_data[2] ^ received_data[4] ^ received_data[6] ^ received_data[8] ^ received_data[10];
        verify_bit[1] = received_data[1] ^ received_data[2] ^ received_data[5] ^ received_data[6] ^ received_data[9] ^ received_data[10];
        verify_bit[2] = received_data[3] ^ received_data[4] ^ received_data[5] ^ received_data[6] ^ received_data[11];
        verify_bit[3] = received_data[7] ^ received_data[8] ^ received_data[9] ^ received_data[10] ^ received_data[11];

 		   
        // Corrige o bit de erro, se houver um único erro detectado
        if (verify_bit != 4'b0000) begin
          $display("received data antes : %b",received_data);
          received_data[verify_bit-1] = ~received_data[verify_bit-1];
          $display("received data depois : %b",received_data);
        end

        // Decodifica os dados
        decoded_data[0] = received_data[2];
        decoded_data[1] = received_data[4];
        decoded_data[2] = received_data[5];
        decoded_data[3] = received_data[6];
        decoded_data[4] = received_data[8];
        decoded_data[5] = received_data[9];
        decoded_data[6] = received_data[10];
        decoded_data[7] = received_data[11];
        decoded_data [15:8] = 0;

    end

endmodule