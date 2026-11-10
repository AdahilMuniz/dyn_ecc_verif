module tbec_rsc_encoder (
  input  logic [ 7:0] data_in,
  output logic [15:0] data_out
);
  logic [1:0] Di;
  logic [1:0] P;
  logic [1:0] Cb [1:0];

  always_comb begin
    Di = {data_in[1] ^ data_in[3] ^ data_in[4] ^ data_in[6], data_in[0] ^ data_in[2] ^ data_in[5] ^ data_in[7]};

    P  = {data_in[2] ^ data_in[3] ^ data_in[6] ^ data_in[7], data_in[0] ^ data_in[1] ^ data_in[4] ^ data_in[5]};

    Cb = {{data_in[5] ^ data_in[7], data_in[1] ^ data_in[3]}, {data_in[4] ^ data_in[6], data_in[0] ^ data_in[2]}};
  end

  assign data_out = {Cb[1], Cb[0], P, Di, data_in};
endmodule
