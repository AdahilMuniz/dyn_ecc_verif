// Hamming(12,8) decoder: single-error correction over cw[11:0]; cw[15:12] is
// ignored. A syndrome s in 1..12 flips bit s-1; 13..15 (only reachable with
// multi-bit errors) flips nothing, as in the original, where the out-of-range
// index write was ignored.
// Bit-exact with hamming_decoder.sv of the 2024-08 FPGA (checked by `make equiv`).
module dyn_ecc_hamming_dec (
  /* verilator lint_off UNUSEDSIGNAL */  // cw[15:12] are not part of Hamming(12,8)
  input  logic [15:0] cw,
  /* verilator lint_on UNUSEDSIGNAL */
  output logic [ 7:0] data,
  output logic [ 3:0] syndrome
);
  /* verilator lint_off UNUSEDSIGNAL */  // parity positions of r aren't output
  logic [11:0] r;
  /* verilator lint_on UNUSEDSIGNAL */
  logic [11:0] flip;

  always_comb begin
    syndrome[0] = cw[0] ^ cw[2] ^ cw[4] ^ cw[6] ^ cw[8] ^ cw[10];
    syndrome[1] = cw[1] ^ cw[2] ^ cw[5] ^ cw[6] ^ cw[9] ^ cw[10];
    syndrome[2] = cw[3] ^ cw[4] ^ cw[5] ^ cw[6] ^ cw[11];
    syndrome[3] = cw[7] ^ cw[8] ^ cw[9] ^ cw[10] ^ cw[11];

    flip = '0;
    if (syndrome != 4'd0 && syndrome <= 4'd12) flip[syndrome-4'd1] = 1'b1;
    r = cw[11:0] ^ flip;

    data = {r[11], r[10], r[9], r[8], r[6], r[5], r[4], r[2]};
  end
endmodule
