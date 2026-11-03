interface sideband_if #(
    parameter WWIDTH = 16,
    parameter RWIDTH = 16
) (
    input clk,
    input rstn
);

    logic [WWIDTH-1:0] wsig; // Input Signal
    logic [RWIDTH-1:0] rsig; // Output Signal
endinterface
