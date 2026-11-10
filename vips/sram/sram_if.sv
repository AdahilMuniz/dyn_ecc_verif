// SRAM-style interface: one write port (wdata), one read port (rdata), a simple
// control pair (we/cs) and an address. Used for the MCU (FMC) bus and for the
// memory-side bus in the dyn_ecc testbench.
//
// The signals are LOGICAL and active high (cs=1 selects, we=1 writes). The
// testbench top converts them to the active-low pins of the DUT and of the SRAM
// model (NE/NWE/NOE, CE_n/WE_n/OE_n).
//
// The defaults match the board: 16-bit data, 21-bit word address (2M x 16 SRAM).
interface sram_if #(
    parameter DATA_WIDTH = 16,
    parameter ADDR_WIDTH = 21
) (
    input clk,
    input rstn
);

    logic [DATA_WIDTH-1:0] wdata; // data written toward the memory
    logic [DATA_WIDTH-1:0] rdata; // data read back from the memory
    logic [ADDR_WIDTH-1:0] addr;  // address
    logic                  we;    // write enable (1 = write)
    logic                  cs;    // chip select  (1 = access in progress)
endinterface
