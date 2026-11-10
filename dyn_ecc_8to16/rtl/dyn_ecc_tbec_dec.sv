// TBEC-RSC 8->16 decoder (codeword layout: see dyn_ecc_tbec_enc).
// Syndromes:
//   sx[3:0] column-pair (AC1, BD2, AC3, BD4)
//   sp[1:0] quadrant
//   sd[1:0] diagonal
// The quadrant parities pick which rows the sx syndromes are applied to.
// Bit-exact with Decoder16bits.sv of the 2024-08 FPGA with its
// signal[0] = data_in[2] typo fixed (the 2024-10 build carries the same fix;
// checked by `make equiv`).
module dyn_ecc_tbec_dec (
  input  logic [15:0] cw,
  output logic [ 7:0] data
);
  logic [7:0] s;
  logic [3:0] sx;
  logic [1:0] sp, sd;
  logic [2:0] sum_sx;

  always_comb begin
    s      = cw[7:0];

    sx[0]  = s[0] ^ s[2] ^ cw[12];
    sx[1]  = s[1] ^ s[3] ^ cw[13];
    sx[2]  = s[4] ^ s[6] ^ cw[14];
    sx[3]  = s[5] ^ s[7] ^ cw[15];
    sp[0]  = s[0] ^ s[4] ^ s[1] ^ s[5] ^ cw[10];
    sp[1]  = s[2] ^ s[3] ^ s[6] ^ s[7] ^ cw[11];
    sd[0]  = s[0] ^ s[2] ^ s[7] ^ s[5] ^ cw[8];
    sd[1]  = s[1] ^ s[3] ^ s[4] ^ s[6] ^ cw[9];

    sum_sx = 3'(sx[0]) + 3'(sx[1]) + 3'(sx[2]) + 3'(sx[3]);

    if (sp != 2'b00 || sd != 2'b00 || sum_sx > 3'd1) begin
      if (sp == 2'b01) begin         // first quadrant: rows 0-1
        s[0] ^= sx[0];  s[4] ^= sx[2];
        s[1] ^= sx[1];  s[5] ^= sx[3];
      end else if (sp == 2'b10) begin  // second quadrant: rows 2-3
        s[2] ^= sx[0];  s[6] ^= sx[2];
        s[3] ^= sx[1];  s[7] ^= sx[3];
      end else if (sp == 2'b11 || sd != 2'b00) begin  // central quadrant: rows 1-2
        s[1] ^= sx[0];  s[5] ^= sx[2];
        s[2] ^= sx[1];  s[6] ^= sx[3];
      end
    end

    data = s;
  end
endmodule
