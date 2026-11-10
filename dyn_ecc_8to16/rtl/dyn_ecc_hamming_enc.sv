// Hamming(12,8) encoder. Parity at codeword positions 0, 1, 3, 7; data at
// 2, 4-6, 8-11; cw[15:12] = 0.
// Bit-exact with hamming_encoder.sv of the 2024-08 FPGA and with the 8-bit
// firmware's nascerr_eccs_hamming_encoder (checked by `make equiv`).
module dyn_ecc_hamming_enc (
  input  logic [ 7:0] data,
  output logic [15:0] cw
);
  logic [3:0] p;

  always_comb begin
    p[0] = data[0] ^ data[1] ^ data[3] ^ data[4] ^ data[6];
    p[1] = data[0] ^ data[2] ^ data[3] ^ data[5] ^ data[6];
    p[2] = data[1] ^ data[2] ^ data[3] ^ data[7];
    p[3] = data[4] ^ data[5] ^ data[6] ^ data[7];
  end

  assign cw = {4'b0000, data[7:4], p[3], data[3:1], p[2], data[0], p[1], p[0]};
endmodule
