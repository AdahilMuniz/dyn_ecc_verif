`include "macros.svh"

module tbec_rsc_decoder (
  input  logic [15:0] data_in,
  output logic [ 7:0] data_out
);
  logic [7:0] signal;
  logic [7:0] red;

  logic [1:0] linsin [1:0];
  logic [1:0] SP;
  logic [1:0] SDi;

  logic [2:0] sumSL;
  logic [1:0] sumSP, sumSDi;

  always_comb begin
    {red, signal} = data_in;

    linsin = {
      {signal[5] ^ signal[7] ^ red[7], signal[4] ^ signal[6] ^ red[6]}, {signal[1] ^ signal[3] ^ red[5], signal[0] ^ signal[2] ^ red[4]}
    };

    SP = {signal[2] ^ signal[3] ^ signal[6] ^ signal[7] ^ red[3], signal[0] ^ signal[1] ^ signal[4] ^ signal[5] ^ red[2]};

    SDi = {signal[1] ^ signal[3] ^ signal[4] ^ signal[6] ^ red[1], signal[0] ^ signal[2] ^ signal[5] ^ signal[7] ^ red[0]};

    sumSP = `EXT2(SP[0]) + `EXT2(SP[1]);
    sumSDi = `EXT2(SDi[0]) + `EXT2(SDi[1]);
    sumSL = `EXT3(linsin[0][0]) + `EXT3(linsin[0][1]) + `EXT3(linsin[1][0]) + `EXT3(linsin[1][1]);

    // Como a ordem dos vetores mudaram, aqui é o lugar mais provável de ocorrer bugs.
    // E entender o motivo de existirem índices maiores que o tamanho do linsin. (Foi feito um ajuste como descrito nas issues)
    if (((SP != '0 && SDi != '0) || sumSL > 1) && !(sumSDi == 0 && sumSP == 1 && sumSL >= 2)) begin
      if (SP[0] > SP[1]) begin
        for (int i = 0; i < 2; i++) begin
          if (linsin[i] != '0) begin
            signal[i] ^= linsin[i][0];
            signal[i+4] ^= linsin[i][1];
          end
        end
      end else if (SP[0] < SP[1]) begin
        for (int i = 2; i < 4; i++) begin
          if (linsin[i-2] != '0) begin
            signal[i] ^= linsin[i-2][0];
            signal[i+4] ^= linsin[i-2][1];
          end
        end
      end else if (SP[0] == SP[1] && {SDi, SP} != '0) begin
        for (int i = 1; i < 3; i++) begin
          if (linsin[i-1] != '0) begin
            signal[i] ^= linsin[i-1][1];
            signal[i+4] ^= linsin[i-1][0];
          end
        end
      end
    end
  end

  assign data_out = signal;
endmodule
