// TBEC-RSC 8->16 encoder.
// Data as a 4x2 matrix s[row][col] = data[row + 4*col]; codeword:
//   cw[7:0]   = data
//   cw[9:8]   = Di[1:0] diagonal parities
//   cw[11:10] = P[1:0]  quadrant (rows 0-1 / rows 2-3) parities
//   cw[15:12] = Cb      column-pair parities {s11^s31, s01^s21, s10^s30, s00^s20}
// Bit-exact with Encoder16bits.sv of the 2024-08 FPGA and with the 8-bit
// firmware's nascerr_eccs_tbec_encoder (checked by `make equiv`).
module dyn_ecc_tbec_enc (
  input  logic [ 7:0] data,
  output logic [15:0] cw
);
  logic [1:0] di, p;
  logic [3:0] cb;

  always_comb begin
    di[0] = data[0] ^ data[5] ^ data[2] ^ data[7];
    di[1] = data[4] ^ data[1] ^ data[6] ^ data[3];
    p[0]  = data[0] ^ data[4] ^ data[1] ^ data[5];
    p[1]  = data[2] ^ data[6] ^ data[3] ^ data[7];
    cb[0] = data[0] ^ data[2];
    cb[1] = data[1] ^ data[3];
    cb[2] = data[4] ^ data[6];
    cb[3] = data[5] ^ data[7];
  end

  assign cw = {cb, p, di, data};
endmodule
