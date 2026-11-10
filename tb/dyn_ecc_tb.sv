`include "uvm_macros.svh"

// Testbench top for the dyn_ecc 8->16 ASIC RTL (dyn_ecc_8to16/rtl, dyn_ecc_top).
//
// Wired like the board-level top of the 8->16 FPGA it derives from
// (nascerr_8to16_legacy/fpga/2024-08_fpga_1memory_8to16/component/work/stm32/stm32.v),
// plus NOE into the DUT, which now drives the MCU bus only during reads:
//
//   MCU agent (FMC model) --D[15:0]--> dyn_ecc_top --D[15:0]--> SRAM (IS61WV2M16AL)
//        |  A[20:0], NE, NWE, NOE  ------------------------------->  (straight to SRAM)
//        |  NE -> cs_n, NWE -> we_n, NOE -> oe_n                    LB/UB tied active
//   sideband: wsig -> ecc_sel[1:0], erro -> rsig[0]
//
// The agents use logical active-high cs/we; this module converts them to the
// active-low pins of the DUT and the SRAM.
module dyn_ecc_tb;

  import uvm_pkg::*;
  import dyn_ecc_test_pkg::*;

  // Clock and active-low reset: declared (as variables) before the interfaces
  // that reference them, otherwise clk/rst_n get inferred as implicit nets and
  // cannot be driven procedurally. The DUT itself has no reset; rst_n only gates
  // the agents.
  logic clk = 1'b0;
  logic rst_n;
  always #5 clk = ~clk; // 100 MHz; stands in for the FPGA's on-chip oscillator

  // ---- Interface instances --------------------------------------------------
  sram_if                               mcu_if0  (.clk(clk), .rstn(rst_n)); // MCU (FMC) bus, active agent
  sram_if                               sram_if0 (.clk(clk), .rstn(rst_n)); // FPGA<->SRAM bus, passive agent
  sideband_if #(.WWIDTH(2), .RWIDTH(1)) mux_if0  (.clk(clk), .rstn(rst_n)); // ecc_sel / erro
  sram_backdoor_if                      sram_bd0 ();

  // ---- Board nets -------------------------------------------------------------
  wire  [15:0] mcu_fpga_io;   // MCU <-> FPGA data
  wire  [15:0] fpga_mem_io;   // FPGA <-> SRAM data
  logic        ne_n;          // FMC NE1  -> DUT chip_sel, SRAM CE_n
  logic        nwe_n;         // FMC NWE  -> DUT write_en, SRAM WE_n
  logic        noe_n;         // FMC NOE  -> DUT oe_n, SRAM OE_n
  logic        erro;

  assign ne_n  = ~mcu_if0.cs;
  assign nwe_n = ~(mcu_if0.cs &  mcu_if0.we);
  assign noe_n = ~(mcu_if0.cs & ~mcu_if0.we);

  // ---- DUT ----------------------------------------------------------------------
  dyn_ecc_top dut (
    .mcu_data (mcu_fpga_io),
    .mem_data (fpga_mem_io),
    .cs_n     (ne_n),
    .we_n     (nwe_n),
    .oe_n     (noe_n),
    .ecc_sel  (mux_if0.wsig),
    .clk      (clk),
    .erro     (erro)
  );

  // ---- SRAM (address and controls straight from the MCU, as on the board) ---------
  IS61WV2M16AL u_sram (
    .Address (mcu_if0.addr),
    .dataIO  (fpga_mem_io),
    .OE_bar  (noe_n),
    .CE_bar  (ne_n),
    .WE_bar  (nwe_n),
    .LB_bar  (1'b0),
    .UB_bar  (1'b0)
  );

  // ---- MCU bus: drive data on a write, observe the FPGA's output otherwise ---------
  assign mcu_fpga_io   = (~nwe_n) ? mcu_if0.wdata : 'z;
  assign mcu_if0.rdata = mcu_fpga_io;

  // ---- Memory-side observation (passive) ------------------------------------------
  assign sram_if0.addr  = mcu_if0.addr;
  assign sram_if0.cs    = ~ne_n;
  assign sram_if0.we    = ~nwe_n;
  assign sram_if0.wdata = fpga_mem_io; // codeword driven by the FPGA on a write
  assign sram_if0.rdata = fpga_mem_io; // stored word driven by the SRAM on a read

  // ---- Sideband ---------------------------------------------------------------------
  assign mux_if0.rsig = erro;
  initial mux_if0.wsig = ECC_BYPASS; // sequences change it through the virtual interface

  // ---- Reset generation -----------------------------------------------------
  initial begin
    rst_n = 1'b0;
    repeat (2) @(posedge clk);
    rst_n = 1'b1;
  end

  // ---- Publish interfaces to the UVM config_db and launch the test ----------
  initial begin
    uvm_config_db#(virtual sram_if)::set(null, "*", "mcu_if",   mcu_if0);
    uvm_config_db#(virtual sram_if)::set(null, "*", "sram0_if", sram_if0);
    uvm_config_db#(mux_vif_t)      ::set(null, "*", "mux_if",   mux_if0);
    uvm_config_db#(sram_bd_vif_t)  ::set(null, "*", "sram_bd",  sram_bd0);
    run_test();
  end

  // ---- FSDB waveform dump (enabled by the Makefile's DEBUG=1) ---------------
`ifdef DUMP_FSDB
  initial begin
    $fsdbDumpfile("dyn_ecc_tb.fsdb");
    $fsdbDumpvars(0, dyn_ecc_tb);
  end
`endif

endmodule
