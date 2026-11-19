// dyn_ecc 8->16 datapath and control: mode mux between bypass and the three
// codecs, plus output enables for the two bidirectional buses. Fully
// combinational (D12: no added latency); no tri-states here (see dyn_ecc_top).
//
// Bus protocol: STM32 FMC asynchronous SRAM, active-low NE (cs_n), NWE (we_n),
// NOE (oe_n). Address and byte lanes go from the MCU straight to the SRAM.
//   write (cs_n=0, we_n=0)          : drive mem_data with the codeword
//   read  (cs_n=0, we_n=1, oe_n=0)  : drive mcu_data with the decoded word
// Unlike the 2024-08 FPGA, which drove the MCU bus whenever no write was in
// progress, the MCU bus is driven only during a read (D6, power). That
// requires the NOE input.
//
// In ECC modes the MCU word's low byte is encoded, and reads return
// {8'h00, data}. In bypass all 16 bits pass through.
module dyn_ecc_ctrl
  import dyn_ecc_pkg::*;
(
  input  ecc_sel_t    sel,
  input  logic        cs_n,
  input  logic        we_n,
  input  logic        oe_n,
  input  logic [15:0] mcu_data_i,
  output logic [15:0] mcu_data_o,
  output logic        mcu_data_oe,
  input  logic [15:0] mem_data_i,
  output logic [15:0] mem_data_o,
  output logic        mem_data_oe,
  output logic [ 3:0] hamming_syndrome
);
  logic        write, read;
  logic [15:0] ham_cw, tbec_cw, edac_cw;
  logic [ 7:0] ham_data, tbec_data, edac_data;

  dyn_ecc_hamming_enc u_ham_enc  (.data(mcu_data_i[7:0]), .cw(ham_cw));
  dyn_ecc_tbec_enc    u_tbec_enc (.data(mcu_data_i[7:0]), .cw(tbec_cw));
  dyn_ecc_edac_enc    u_edac_enc (.data(mcu_data_i[7:0]), .cw(edac_cw));

  dyn_ecc_hamming_dec u_ham_dec  (.cw(mem_data_i), .data(ham_data), .syndrome(hamming_syndrome));
  dyn_ecc_tbec_dec    u_tbec_dec (.cw(mem_data_i), .data(tbec_data));
  dyn_ecc_edac_dec    u_edac_dec (.cw(mem_data_i), .data(edac_data));

  assign write       = !cs_n && !we_n;
  assign read        = !cs_n && we_n && !oe_n;
  assign mem_data_oe = write;
  assign mcu_data_oe = read;

  always_comb begin
    unique case (sel)
      ECC_BYPASS: begin
        mem_data_o = mcu_data_i;
        mcu_data_o = mem_data_i;
      end
      ECC_HAMMING: begin
        mem_data_o = ham_cw;
        mcu_data_o = {8'h00, ham_data};
      end
      ECC_TBEC: begin
        mem_data_o = tbec_cw;
        mcu_data_o = {8'h00, tbec_data};
      end
      ECC_EDAC: begin
        mem_data_o = edac_cw;
        mcu_data_o = {8'h00, edac_data};
      end
      default: begin
        mem_data_o = 'x;
        mcu_data_o = 'x;
      end
    endcase
  end
endmodule
