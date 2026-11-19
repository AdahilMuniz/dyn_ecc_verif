`include "uvm_macros.svh"

// Defaults; overridable from the command line via +define+ (see Makefile).
`ifndef TB_DATA_WIDTH
  `define TB_DATA_WIDTH 8
`endif
`ifndef TB_ADDR_WIDTH
  `define TB_ADDR_WIDTH 16
`endif

module dyn_ecc_tb #(
    parameter DATA_WIDTH = `TB_DATA_WIDTH,
    parameter ADDR_WIDTH = `TB_ADDR_WIDTH,
    parameter SIDEBAND_WWIDTH = 2,
    parameter SIDEBAND_RWIDTH = 3
);

  import uvm_pkg::*;
  import sram_pkg::*;
  import dyn_ecc_test_pkg::*;

  // Clock and active-low reset: declared (as variables) before the interfaces
  // that reference them, otherwise clk/rst_n get inferred as implicit nets and
  // cannot be driven procedurally.
  logic clk = 1'b0;
  logic rst_n;
  always #5 clk = ~clk;

  // ---- Interface instances --------------------------------------------------
  sram_if #(.DATA_WIDTH(DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH)) mcu_if0 (.clk(clk), .rstn(rst_n));
  sram_if #(.DATA_WIDTH(DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH)) sram_if0 (.clk(clk), .rstn(rst_n));
  sram_if #(.DATA_WIDTH(DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH)) sram_if1 (.clk(clk), .rstn(rst_n));
  sideband_if #(.WWIDTH(SIDEBAND_WWIDTH), .RWIDTH(SIDEBAND_RWIDTH)) mux_if0 (.clk(clk), .rstn(rst_n));

  // ---- DUT buses / controls -------------------------------------------------
  wire [DATA_WIDTH-1:0] cpu_data;
  wire [DATA_WIDTH-1:0] mem_up_data;
  wire [DATA_WIDTH-1:0] mem_down_data;

  logic       write_en;
  logic       read_en;
  logic       chip_sel;
  logic [1:0] chip_sel_out;
  logic [SIDEBAND_WWIDTH-1:0] ecc_sel;
  logic [0:SIDEBAND_WWIDTH-1] flag_out;

  // ---- DUT instance + interface<->pin bridging ------------------------------
  // Bridges live inside each branch because the two DUTs use opposite control
  // conventions (fpga: active-low write_en + output_en; NASCERR top:
  // active-high read_en/write_en pair).
  generate;
    if (DATA_WIDTH == 16) begin : g_fpga
      fpga_top_design dut (
        .mcu_fpga_io      (cpu_data),
        .fpga_mem_io_up   (mem_up_data),
        .fpga_mem_io_down (mem_down_data),
        .write_en         (write_en),
        .chip_sel         (chip_sel),
        .chip_sel_out     (chip_sel_out),
        .ecc_sel          ({1'b0, ecc_sel}),
        .flag_out         (flag_out),
        .clk              (clk),
        .rst_n            (rst_n),
        .output_en        (read_en)
      );

      // write_en ACTIVE LOW: the DUT releases cpu_data during a write.
      assign write_en = mcu_if0.we;
      assign chip_sel = mcu_if0.cs;
      assign ecc_sel  = mux_if0.wsig;
      assign read_en  = 1'b1; // output_en idle (not reading)

      assign cpu_data      = (~write_en) ? mcu_if0.wdata : 'bz;
      assign mcu_if0.rdata = ( write_en) ? cpu_data      : 'bz;

      assign sram_if0.wdata = (~write_en) ? mem_up_data    : 'bz;
      assign mem_up_data    = ( write_en) ? sram_if0.rdata : 'bz;
      assign sram_if0.we    = write_en;
      assign sram_if0.cs    = chip_sel_out[0];

      assign sram_if1.wdata = (~write_en) ? mem_down_data  : 'bz;
      assign mem_down_data  = ( write_en) ? sram_if1.rdata : 'bz;
      assign sram_if1.we    = write_en;
      assign sram_if1.cs    = chip_sel_out[1];

      assign mux_if0.rsig = flag_out;
    end
    else begin : g_nascerr
      top dut (
        .sel(ecc_sel),
        .read_en(read_en),
        .write_en(write_en),
        .cpu_data(cpu_data),
        .ram_up_data(mem_up_data),
        .ram_down_data(mem_down_data)
      );

      // FIX 1: read_en is the complement of write (not tied high), so the DUT's
      // write path (write_en=1 && read_en=0) can actually fire.
      assign write_en = mcu_if0.we;
      assign read_en  = ~mcu_if0.we;
      assign ecc_sel  = mux_if0.wsig;

      // FIX 2: active-high polarity. The DUT drives cpu_data on a READ and the
      // memory buses on a WRITE, so the TB drives the opposite side each phase.
      assign cpu_data      = write_en ? mcu_if0.wdata : 'bz; // MCU drives on write
      assign mcu_if0.rdata = read_en  ? cpu_data      : 'bz; // capture on read

      assign sram_if0.wdata = write_en ? mem_up_data    : 'bz; // capture codeword
      assign mem_up_data    = read_en  ? sram_if0.rdata : 'bz; // supply on read
      assign sram_if0.we    = write_en;

      assign sram_if1.wdata = write_en ? mem_down_data  : 'bz;
      assign mem_down_data  = read_en  ? sram_if1.rdata : 'bz;
      assign sram_if1.we    = write_en;

      // FIX 3: NASCERR top has no chip_sel_out or flag outputs; drive the
      // dependent interface signals to defined values instead of leaving them X.
      //assign sram_if0.cs  = 1'b0;
      //assign sram_if1.cs  = 1'b0;
      assign sram_if0.cs  = mcu_if0.cs;
      assign sram_if1.cs  = mcu_if0.cs;
      assign mux_if0.rsig = '0; //@TODO: Must be connected to any output flag if any in the future.
    end
  endgenerate

  // ---- Behavioral SRAM memory models (backdoor-accessible) ------------------
  // One model per physical memory (up/down). The storage lives in a class
  // object whose handle is published on the config_db below, so a UVM test can
  // edit mem[] directly (backdoor) and the change is seen here by reference.
  sram_mem_model mem_up_model   = new("mem_up_model");
  sram_mem_model mem_down_model = new("mem_down_model");

  // The NASCERR DUT is combinational and carries no address of its own, so the
  // memories are addressed with the MCU transaction address.
  assign sram_if0.addr = mcu_if0.addr;
  assign sram_if1.addr = mcu_if0.addr;

  // Capture the codeword(s) the DUT presents during a write. Keyed on the MCU's
  // we (the canonical "this cycle is a write"), sampled one edge after the
  // driver asserts it.
  always @(posedge clk) begin
    if (rst_n === 1'b1 && mcu_if0.we === 1'b1) begin
      mem_up_model.poke  (mcu_if0.addr, sram_if0.wdata);
      mem_down_model.poke(mcu_if0.addr, sram_if1.wdata);
    end
  end

  // Drive stored data back toward the DUT's memory-side buses. Recomputed on
  // the clock negedge (and on address change) rather than with a continuous
  // assign: the backing store is a class array, so element writes - whether
  // from the write path above or from a UVM backdoor poke - are not in a
  // continuous assign's sensitivity. Sampling on negedge keeps rdata stable and
  // fresh before the driver samples it on the posedge (no race).
  always @(negedge clk or mcu_if0.addr) begin
    sram_if0.rdata = mem_up_model.peek  (mcu_if0.addr);
    sram_if1.rdata = mem_down_model.peek(mcu_if0.addr);
  end

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

    // Publish the memory model handles for backdoor peek/poke from the UVM env.
    uvm_config_db#(sram_mem_model)::set(null, "*", "mem_up",   mem_up_model);
    uvm_config_db#(sram_mem_model)::set(null, "*", "mem_down", mem_down_model);

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
