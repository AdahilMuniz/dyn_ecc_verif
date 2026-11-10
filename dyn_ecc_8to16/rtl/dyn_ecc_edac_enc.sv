// EDAC 8->16 encoder: v[7:0] = T(data) (a linear XOR transform), then
// v[15:8] = P(v[7:0]). P(T(w)) = w, so the codeword is systematic in its upper
// byte: cw = {data, T(data)}.
// Bit-exact with EDACencoder.sv of the 2024-08 FPGA and with the 8-bit
// firmware's nascerr_eccs_edac_encoder (checked by `make equiv`).
module dyn_ecc_edac_enc (
  input  logic [ 7:0] data,
  output logic [15:0] cw
);
  logic [7:0] w;
  logic [15:0] v;

  always_comb begin
    w      = data;
    v[0]   = w[2] ^ w[4] ^ w[6];
    v[1]   = w[7] ^ w[0];
    v[2]   = w[2] ^ w[4] ^ w[5];
    v[3]   = w[7] ^ w[1] ^ w[2] ^ w[5] ^ w[6];
    v[4]   = w[2] ^ w[4] ^ w[7] ^ w[0];
    v[5]   = w[7] ^ w[6] ^ w[2] ^ w[5] ^ w[4];
    v[6]   = w[6] ^ w[3] ^ w[4] ^ w[5];
    v[7]   = w[7] ^ w[1] ^ w[4] ^ w[5] ^ w[6] ^ w[3];
    v[15:8] = dyn_ecc_edac_p(v[7:0]);
  end

  assign cw = v;

  // P: the 8 parity equations over v[7:0] (shared with dyn_ecc_edac_dec).
  function automatic logic [7:0] dyn_ecc_edac_p(input logic [7:0] x);
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
