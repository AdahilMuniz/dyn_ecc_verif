// SRAM-style interface: one write port (wdata), one read port (rdata), a simple
// control pair (we/cs) and an address. Used for the MCU bus and for each of the
// two memory buses in the dyn_ecc testbench.
interface sram_if #(
    parameter DATA_WIDTH = 16,
    parameter ADDR_WIDTH = 16
) (
    input clk,
    input rstn
);

    logic [DATA_WIDTH-1:0] wdata; // data written toward the memory
    logic [DATA_WIDTH-1:0] rdata; // data read back from the memory
    logic [ADDR_WIDTH-1:0] addr;  // address
    logic                  we;    // write enable
    logic                  cs;    // chip select
endinterface
