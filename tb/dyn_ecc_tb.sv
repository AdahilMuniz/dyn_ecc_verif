`include "uvm_macros.svh"

module dyn_ecc_tb;

  import uvm_pkg::*;

  // Clock
  logic clk = 1'b0;
  always #5 clk = ~clk;

  // Synchronous, active-low reset
  logic rst_n;

  // Inout buses (must be wire for tristate drive from the DUT)
  wire [15:0] mcu_fpga_io;
  wire [15:0] fpga_mem_io_up;
  wire [15:0] fpga_mem_io_down;

  // Scalar / vector controls
  logic       write_en;
  logic       chip_sel;
  logic [1:0] chip_sel_out;
  logic [2:0] ecc_sel;
  logic [0:2] flag_out;
  logic       output_en;

  // TB-side driver for the bidirectional MCU bus.
  // During a write the DUT tri-states mcu_fpga_io, so the TB drives the input
  // word here; otherwise it releases the bus (high-Z) to observe the DUT.
  logic [15:0] mcu_drv;
  logic        mcu_drv_en;
  assign mcu_fpga_io = mcu_drv_en ? mcu_drv : 16'bz;

  // TB-side driver for the two memory buses.
  // On a write the DUT drives these (TB stays off, mem_drv_en=0).
  // On a read the DUT releases them, so the TB drives the encoded data back in
  // here, emulating the memory contents the decoder reads.
  logic [15:0] mem_up_drv;
  logic [15:0] mem_down_drv;
  logic        mem_drv_en;
  assign fpga_mem_io_up   = mem_drv_en ? mem_up_drv   : 16'bz;
  assign fpga_mem_io_down = mem_drv_en ? mem_down_drv : 16'bz;

  // DUT instantiation
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

  // ---------------------------------------------------------------------------
  // write_word: push a 16-bit word into the FPGA and capture the codified word.
  //   data : input word presented by the "MCU" on mcu_fpga_io
  //   mode : ecc_sel value (3'b010 = TBEC_RSC, 3'b011 = MRSC codification;
  //                         3'b000/3'b001 = bypass, no encoding)
  //   chip : chip_sel value (selects which memory(ies) are driven)
  // The 32-bit codeword appears split across the two memory buses:
  //   fpga_mem_io_up = codeword[0:15], fpga_mem_io_down = codeword[16:31].
  // ---------------------------------------------------------------------------
  task automatic write_word(input  logic [15:0] data,
                            input  logic [2:0]  mode,
                            input  logic        chip,
                            output logic [15:0] code_up,
                            output logic [15:0] code_down);
    // Set up a write cycle (write_en active-low).
    ecc_sel    = mode;
    chip_sel   = chip;
    output_en  = 1'b1;   // not reading
    write_en   = 1'b0;   // write active

    // TB must not drive the memory buses during a write (DUT drives them).
    mem_drv_en = 1'b0;

    // Present the word on the MCU bus (DUT has released it for a write).
    mcu_drv    = data;
    mcu_drv_en = 1'b1;

    // The encode path is combinational; sync to a clock edge and let it settle.
    @(posedge clk);
    #1;

    // Capture the codeword the DUT drives onto the memory buses.
    code_up   = fpga_mem_io_up;
    code_down = fpga_mem_io_down;

    $display("[%0t] WRITE data=0x%04h ecc_sel=%03b chip_sel=%b -> chip_sel_out=%02b | codeword up=0x%04h down=0x%04h",
             $time, data, mode, chip, chip_sel_out, code_up, code_down);

    // Release the bus and deassert the write.
    mcu_drv_en = 1'b0;
    write_en   = 1'b1;
  endtask

  // ---------------------------------------------------------------------------
  // read_word: present a previously captured codeword on the memory buses and
  // read back the decoded word the FPGA drives onto mcu_fpga_io.
  //   code_up/code_down : the encoded data to feed back in (as "memory" content)
  //   mode              : ecc_sel value (use the same mode used to encode)
  //   chip              : chip_sel value
  //   data_out          : decoded word read from mcu_fpga_io
  //   flag_val          : ECC flag latched by the DUT during the read
  // ---------------------------------------------------------------------------
  task automatic read_word(input  logic [15:0] code_up,
                           input  logic [15:0] code_down,
                           input  logic [2:0]  mode,
                           input  logic        chip,
                           output logic [15:0] data_out,
                           output logic [0:2]  flag_val);
    // Set up a read cycle.
    ecc_sel    = mode;
    chip_sel   = chip;
    write_en   = 1'b1;   // not a write -> DUT drives mcu_fpga_io with decoder out
    output_en  = 1'b0;   // enable read path so flag_out latches the decoder flag

    // Release the MCU bus so the FPGA can drive the decoded word onto it.
    mcu_drv_en = 1'b0;

    // Drive the encoded data onto the memory buses (DUT has released them).
    mem_up_drv   = code_up;
    mem_down_drv = code_down;
    mem_drv_en   = 1'b1;

    // Decode is combinational; a clock edge latches flag_out.
    @(posedge clk);
    #1;

    data_out = mcu_fpga_io;   // decoder_output driven by the FPGA
    flag_val = flag_out;

    $display("[%0t] READ  code up=0x%04h down=0x%04h ecc_sel=%03b chip_sel=%b -> data=0x%04h flag=%03b",
             $time, code_up, code_down, mode, chip, data_out, flag_val);

    // Release the memory buses.
    mem_drv_en = 1'b0;
    output_en  = 1'b1;
  endtask

  // FSDB waveform dump (enabled by the Makefile's DEBUG=1 via +define+DUMP_FSDB).
`ifdef DUMP_FSDB
  initial begin
    $fsdbDumpfile("dyn_ecc_tb.fsdb");
    $fsdbDumpvars(0, dyn_ecc_tb);
  end
`endif

  // Quick smoke stimulus: write a word (capture the codeword), then read it
  // back by feeding the captured codeword into the memory buses.
  initial begin
    logic [15:0] code_up, code_down;
    logic [15:0] rdata;
    logic [0:2]  rflag;

    rst_n      = 1'b0;   // assert synchronous reset (active low)
    mcu_drv_en = 1'b0;
    mem_drv_en = 1'b0;
    write_en   = 1'b1;
    output_en  = 1'b1;
    chip_sel   = 1'b0;
    ecc_sel    = 3'b000;
    mcu_drv    = '0;
    mem_up_drv   = '0;
    mem_down_drv = '0;

    // Hold reset for a couple of clocks, then release.
    repeat (2) @(posedge clk);
    rst_n = 1'b1;
    @(posedge clk);

    // --- TBEC_RSC: write then read back ---
    write_word(16'hA5A5, 3'b010, 1'b0, code_up, code_down);
    read_word (code_up, code_down, 3'b010, 1'b0, rdata, rflag);
    if (rdata === 16'hA5A5) $display("  TBEC_RSC round-trip OK");
    else                    $display("  TBEC_RSC round-trip MISMATCH (got 0x%04h, exp 0xA5A5)", rdata);

    // --- MRSC: write then read back ---
    write_word(16'h1234, 3'b011, 1'b0, code_up, code_down);
    read_word (code_up, code_down, 3'b011, 1'b0, rdata, rflag);
    if (rdata === 16'h1234) $display("  MRSC round-trip OK");
    else                    $display("  MRSC round-trip MISMATCH (got 0x%04h, exp 0x1234)", rdata);

    #20;
    $finish;
  end

endmodule
