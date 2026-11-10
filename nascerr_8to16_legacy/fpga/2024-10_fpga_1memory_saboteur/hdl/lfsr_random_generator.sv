// Code your design here
module lfsr_random_generator(
    input logic clk,
    input logic reset,
    output logic [2:0] random_value
);

    logic [1:0] lfsr;

    // Bloco sequencial para o LFSR
    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            lfsr <= 2'b01;
            //random_value <= 3'b000;
        end else begin
            lfsr <= {lfsr[0], lfsr[1] ^ lfsr[0]};
        end
    end

    // Bloco combinacional para gerar o random_value baseado no LFSR
    always_comb begin
        if (lfsr == 2'b01) begin
            random_value = 3'b001;
        end
        else if (lfsr == 2'b10) begin
            random_value = 3'b011;
        end
        else if (lfsr == 2'b11) begin
            random_value = 3'b111;
        end
        else begin
            random_value = 3'b000;
        end
    end

endmodule
