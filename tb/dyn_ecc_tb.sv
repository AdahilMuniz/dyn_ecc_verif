`include "uvm_macros.svh"

module dyn_ecc_tb;

  import uvm_pkg::*;
  import dyn_ecc_test_pkg::*;

  // Clock and active-low reset: declared (as variables) before the interfaces
  // that reference them, otherwise clk/rst_n get inferred as implicit nets and
  // cannot be driven procedurally.
  logic clk = 1'b0;
  logic rst_n;
  always #5 clk = ~clk;

  // ---- Interface instances --------------------------------------------------
  sram_if #(.DATA_WIDTH(16), .ADDR_WIDTH(16)) mcu_if0 (.clk(clk), .rstn(rst_n));
  sram_if #(.DATA_WIDTH(16), .ADDR_WIDTH(16)) sram_if0 (.clk(clk), .rstn(rst_n));
  sram_if #(.DATA_WIDTH(16), .ADDR_WIDTH(16)) sram_if1 (.clk(clk), .rstn(rst_n));
  sideband_if #(.WWIDTH(3), .RWIDTH(3)) mux_if0 (.clk(clk), .rstn(rst_n));

  // ---- DUT buses / controls -------------------------------------------------
  wire [15:0] mcu_fpga_io;
  wire [15:0] fpga_mem_io_up;
  wire [15:0] fpga_mem_io_down;

  logic       write_en;
  logic       chip_sel;
  logic [1:0] chip_sel_out;
  logic [2:0] ecc_sel;
  logic [0:2] flag_out;
  logic       output_en;

  // ---- DUT instance ---------------------------------------------------------
  fpga_top_design dut (
    .mcu_fpga_io      (mcu_fpga_io),
    .fpga_mem_io_up   (fpga_mem_io_up),
    .fpga_mem_io_down (fpga_mem_io_down),
    .write_en         (write_en),
    .chip_sel         (chip_sel),
    .chip_sel_out     (chip_sel_out),
    .ecc_sel          (ecc_sel),
    .flag_out         (flag_out),
    .clk              (clk),
    .rst_n            (rst_n),
    .output_en        (output_en)
  );

  // ---- Interface <-> DUT pin bridging --------------------------------------
  // NOTE: write_en is ACTIVE LOW in the DUT (write when write_en==0). All the
  // polarities below follow that. A write is therefore issued by driving
  // mcu_if0.we = 0, a read by mcu_if0.we = 1.
  assign write_en = mcu_if0.we;
  assign chip_sel = mcu_if0.cs;
  assign ecc_sel  = mux_if0.wsig;

  // MCU side: on a write (write_en==0) the MCU agent drives the input word onto
  // the bus; on a read (write_en==1) it captures the decoded word the DUT drives.
  assign mcu_fpga_io   = (~write_en) ? mcu_if0.wdata : 'bz;
  assign mcu_if0.rdata = ( write_en) ? mcu_fpga_io   : 'bz;

  // SRAM side (up = sram_if0, down = sram_if1): on a write (write_en==0) the
  // agent captures the codeword the DUT drives toward memory; on a read
  // (write_en==1) it drives the stored codeword back onto the bus.
  assign sram_if0.wdata   = (~write_en) ? fpga_mem_io_up : 'bz;
  assign fpga_mem_io_up   = ( write_en) ? sram_if0.rdata : 'bz;
  assign sram_if0.we      = write_en;
  assign sram_if0.cs      = chip_sel_out[0];

  assign sram_if1.wdata   = (~write_en) ? fpga_mem_io_down : 'bz;
  assign fpga_mem_io_down = ( write_en) ? sram_if1.rdata   : 'bz;
  assign sram_if1.we      = write_en;
  assign sram_if1.cs      = chip_sel_out[1];

  // Sideband: ecc mode select in (wsig -> ecc_sel), ECC flag out (flag_out -> rsig).
  assign mux_if0.rsig = flag_out;

  // output_en is not yet owned by any agent; tie it idle (not reading) for now.
  assign output_en = 1'b1;

  // ---- Reset generation -----------------------------------------------------
  initial begin
    rst_n = 1'b0;
    repeat (2) @(posedge clk);
    rst_n = 1'b1;
  end

  // ---- Publish interfaces to the UVM config_db and launch the test ----------
  initial begin
    uvm_config_db#(virtual sram_if)::set    (null, "*", "mcu_if",   mcu_if0);
    uvm_config_db#(virtual sram_if)::set    (null, "*", "sram0_if", sram_if0);
    uvm_config_db#(virtual sram_if)::set    (null, "*", "sram1_if", sram_if1);
    uvm_config_db#(virtual sideband_if)::set(null, "*", "mux_if",   mux_if0);
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
