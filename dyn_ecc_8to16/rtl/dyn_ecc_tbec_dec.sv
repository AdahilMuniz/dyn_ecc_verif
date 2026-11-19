// TBEC-RSC 8->16 decoder (codeword layout: see dyn_ecc_tbec_enc).
// Data as a 4x2 matrix s[row + 4*col]. Syndromes:
//   sx[3:0] column-pair parity: sx[0]/sx[1] = rows 0/2 and 1/3 of column 0,
//           sx[2]/sx[3] = the same pairs in column 1
//   sp[1:0] quadrant parity (rows 0-1 / rows 2-3)
//   sd[1:0] diagonal parity
// The quadrant parities pick the rows the sx syndromes are applied to; a bit
// in row r, column c is always corrected by the pair syndrome of (r % 2, c).
//
// Decoding logic from IagodFarias/NASCERR, branch fix/tbec-decoder (7fa5364,
// 2026-11-18), mapped to the firmware's codeword layout (that repo's encoder
// has cw[13] and cw[14] swapped). Bit-exact with it over all 65,536 inputs
// once cw[13]/cw[14] are swapped (checked 2026-11-19). Compared with the 2024-08 FPGA
// decoder (Decoder16bits.sv, typo fixed), it keeps every 0/1-bit correction
// and corrects more multi-bit errors: 2-bit any 25.0% -> 38.3%, 3-bit
// adjacent 35.7% -> 50.0%. The differences:
//   - the correction gate requires sp AND sd (or >1 pair syndrome), and skips
//     the sd==0 / one sp / >=2 sx case
//   - the central quadrant (rows 1-2) applies the pair syndrome of each row's
//     own pair; the 2024-08 decoder crossed them
// Checked by equiv/run_equiv.sh.
module dyn_ecc_tbec_dec (
  input  logic [15:0] cw,
  output logic [ 7:0] data
);
  logic [7:0] s;
  logic [3:0] sx;
  logic [1:0] sp, sd;
  logic [2:0] sum_sx;
  logic [1:0] sum_sp, sum_sd;

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
    sum_sp = 2'(sp[0]) + 2'(sp[1]);
    sum_sd = 2'(sd[0]) + 2'(sd[1]);

    if (((sp != 2'b00 && sd != 2'b00) || sum_sx > 3'd1)
        && !(sum_sd == 2'd0 && sum_sp == 2'd1 && sum_sx >= 3'd2)) begin
      if (sp == 2'b01) begin                          // rows 0-1
        s[0] ^= sx[0];  s[4] ^= sx[2];
        s[1] ^= sx[1];  s[5] ^= sx[3];
      end else if (sp == 2'b10) begin                 // rows 2-3
        s[2] ^= sx[0];  s[6] ^= sx[2];
        s[3] ^= sx[1];  s[7] ^= sx[3];
      end else if (sp == 2'b11 || sd != 2'b00) begin  // central quadrant: rows 1-2
        s[1] ^= sx[1];  s[5] ^= sx[3];
        s[2] ^= sx[0];  s[6] ^= sx[2];
      end
    end

    data = s;
  end
endmodule
