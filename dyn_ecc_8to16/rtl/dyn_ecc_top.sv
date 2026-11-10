// dyn_ecc 8->16 top: the only place with tri-state buses (which become I/O
// pads in the ASIC), plus the `erro` status register.
//
// Pins (all control inputs active low, as on the STM32 FMC):
//   mcu_data[15:0]  MCU data bus (FMC D[15:0])
//   mem_data[15:0]  SRAM data bus
//   cs_n, we_n, oe_n  FMC NE, NWE, NOE (also wired straight to the SRAM)
//   ecc_sel[1:0]    mode select (dyn_ecc_pkg::ecc_sel_t)
//   clk             only clocks `erro`
//   erro            Hamming-syndrome error flag (2024-08 behavior, kept until
//                   the unified error report, D4/D11): cleared while we_n is
//                   low, set on a clock edge when the Hamming decoder of the
//                   memory bus sees a nonzero syndrome, in any mode. No reset (D5).
module dyn_ecc_top
  import dyn_ecc_pkg::*;
(
  // The two buses are never driven at once (write vs. read), but a linter sees
  // mcu_data -> encoder -> mem_data -> decoder -> mcu_data as a loop.
  /* verilator lint_off UNOPTFLAT */
  inout  wire  [15:0] mcu_data,
  inout  wire  [15:0] mem_data,
  /* verilator lint_on UNOPTFLAT */
  input  logic        cs_n,
  input  logic        we_n,
  input  logic        oe_n,
  input  logic [ 1:0] ecc_sel,
  input  logic        clk,
  output logic        erro
);
  logic [15:0] mcu_data_o, mem_data_o;
  logic        mcu_data_oe, mem_data_oe;
  logic [ 3:0] hamming_syndrome;

  dyn_ecc_ctrl u_ctrl (
    .sel             (ecc_sel_t'(ecc_sel)),
    .cs_n            (cs_n),
    .we_n            (we_n),
    .oe_n            (oe_n),
    .mcu_data_i      (mcu_data),
    .mcu_data_o      (mcu_data_o),
    .mcu_data_oe     (mcu_data_oe),
    .mem_data_i      (mem_data),
    .mem_data_o      (mem_data_o),
    .mem_data_oe     (mem_data_oe),
    .hamming_syndrome(hamming_syndrome)
  );

  assign mcu_data = mcu_data_oe ? mcu_data_o : 'z;
  assign mem_data = mem_data_oe ? mem_data_o : 'z;

  always_ff @(posedge clk) begin
    if (!we_n) erro <= 1'b0;
    else if (hamming_syndrome != 4'd0) erro <= 1'b1;
  end
endmodule
