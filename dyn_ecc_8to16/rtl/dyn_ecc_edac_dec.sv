// EDAC 8->16 decoder (codeword cw = {w, c}: w = data byte, c = check byte; see
// dyn_ecc_edac_enc).
//   t    = T(w)  expected check byte from the data byte
//   d    = P(c)  data byte rebuilt from the check byte
//   sv_c = c ^ t, sv_w = w ^ P(c)  syndromes
// When the check-byte syndrome weighs more than the data-byte syndrome, each
// data bit whose syndrome is set is taken from P(c); otherwise the output is
// P(T(w)), which is w.
// Bit-exact with EDACdecoder.sv of the 2024-08 FPGA (checked by `make equiv`).
// The original's correction of V[7:0] in the "sumP1 < sumP2" branch never
// reached the output, so it's omitted.
module dyn_ecc_edac_dec (
  input  logic [15:0] cw,
  output logic [ 7:0] data
);
  logic [7:0] w, c, t, pt, pc, sv_c, sv_w;
  logic [3:0] wt_c, wt_w;

  always_comb begin
    w    = cw[15:8];
    c    = cw[7:0];
    t    = edac_t(w);
    pt   = edac_p(t);
    pc   = edac_p(c);
    sv_c = c ^ t;
    sv_w = w ^ pc;
    wt_c = 4'($countones(sv_c));
    wt_w = 4'($countones(sv_w));

    data = pt;
    if ({sv_w, sv_c} != 16'd0 && wt_c > wt_w) begin
      for (int b = 0; b < 8; b++) if (sv_w[b]) data[b] = pc[b];
    end
  end

  function automatic logic [7:0] edac_t(input logic [7:0] x);
    logic [7:0] y;
    y[0] = x[2] ^ x[4] ^ x[6];
    y[1] = x[7] ^ x[0];
    y[2] = x[2] ^ x[4] ^ x[5];
    y[3] = x[7] ^ x[1] ^ x[2] ^ x[5] ^ x[6];
    y[4] = x[2] ^ x[4] ^ x[7] ^ x[0];
    y[5] = x[7] ^ x[6] ^ x[2] ^ x[5] ^ x[4];
    y[6] = x[6] ^ x[3] ^ x[4] ^ x[5];
    y[7] = x[7] ^ x[1] ^ x[4] ^ x[5] ^ x[6] ^ x[3];
    return y;
  endfunction

  function automatic logic [7:0] edac_p(input logic [7:0] x);
    logic [7:0] y;
    y[0] = x[0] ^ x[4] ^ x[5] ^ x[2];
    y[1] = x[0] ^ x[1] ^ x[2] ^ x[4] ^ x[5] ^ x[6] ^ x[7];
    y[2] = x[0] ^ x[2] ^ x[3] ^ x[6] ^ x[7];
    y[3] = x[1] ^ x[3] ^ x[4] ^ x[7];
    y[4] = x[0] ^ x[1] ^ x[2] ^ x[3] ^ x[4] ^ x[6] ^ x[7];
    y[5] = x[1] ^ x[2] ^ x[4];
    y[6] = x[0] ^ x[1] ^ x[4];
    y[7] = x[0] ^ x[1] ^ x[4] ^ x[5] ^ x[2];
    return y;
  endfunction
endmodule
