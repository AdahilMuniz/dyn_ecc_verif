// Exhaustive equivalence check of the dyn_ecc 8->16 codecs (dyn_ecc_8to16/rtl)
// against the recovered FPGA RTL and the 8-bit firmware's C encoders.
//
// References (read-only, from nascerr_8to16_legacy/):
//   fpga/2024-08_fpga_1memory_8to16/hdl : hamming_encoder/decoder, Encoder16bits,
//                                         EDACencoder/EDACdecoder
//   fpga/2024-10_fpga_1memory_saboteur/hdl/Decoder16bits.sv : TBEC decoder with
//                                         the signal[0] typo fixed
//   firmware/.../NASCERR_ECCs.c (DPI-C)  : nascerr_eccs_{hamming,tbec,edac}_encoder
//
// Encoders: all 256 data values. Decoders: all 65536 16-bit inputs (every
// codeword and every error pattern). Run with `make equiv`; it prints
// PASS/FAIL. The legacy Hamming decoder's $display calls are filtered out by
// the Makefile.
module codec_equiv_tb;
  import "DPI-C" pure function shortint unsigned nascerr_eccs_hamming_encoder(input shortint unsigned d);
  import "DPI-C" pure function shortint unsigned nascerr_eccs_tbec_encoder   (input shortint unsigned d);
  import "DPI-C" pure function shortint unsigned nascerr_eccs_edac_encoder   (input shortint unsigned d);

  logic [ 7:0] d;
  logic [15:0] cw;

  // new
  logic [15:0] n_ham_cw, n_tbec_cw, n_edac_cw;
  logic [ 7:0] n_ham_d, n_tbec_d, n_edac_d;
  logic [ 3:0] n_ham_syn;
  dyn_ecc_hamming_enc n0 (.data(d), .cw(n_ham_cw));
  dyn_ecc_tbec_enc    n1 (.data(d), .cw(n_tbec_cw));
  dyn_ecc_edac_enc    n2 (.data(d), .cw(n_edac_cw));
  dyn_ecc_hamming_dec n3 (.cw(cw), .data(n_ham_d), .syndrome(n_ham_syn));
  dyn_ecc_tbec_dec    n4 (.cw(cw), .data(n_tbec_d));
  dyn_ecc_edac_dec    n5 (.cw(cw), .data(n_edac_d));

  // legacy
  logic [15:0] l_ham_cw, l_tbec_cw, l_edac_cw, l_ham_d, l_tbec_d, l_edac_d;
  logic [ 3:0] l_ham_par, l_ham_syn;
  hamming_encoder l0 (.data_in({8'h00, d}), .encoded_data(l_ham_cw), .parity_bits(l_ham_par));
  Encoder16bits   l1 (.data_in({8'h00, d}), .data_out(l_tbec_cw));
  EDACencoder     l2 (.data_in({8'h00, d}), .data_out(l_edac_cw));
  hamming_decoder l3 (.encoded_data(cw), .decoded_data(l_ham_d), .verify_bit(l_ham_syn));
  Decoder16bits   l4 (.data_in(cw), .data_out(l_tbec_d));
  EDACdecoder     l5 (.data_in(cw), .data_out(l_edac_d));

  int fail;

  task automatic chk(string what, logic [15:0] got, logic [15:0] exp, int v);
    if (got !== exp) begin
      fail++;
      if (fail <= 10) $display("EQUIV MISMATCH %s @0x%04h: new 0x%04h, reference 0x%04h", what, v, got, exp);
    end
  endtask

  initial begin
    for (int v = 0; v < 256; v++) begin
      d = 8'(v); #1;
      chk("hamming_enc vs RTL", n_ham_cw,  l_ham_cw,  v);
      chk("tbec_enc vs RTL",    n_tbec_cw, l_tbec_cw, v);
      chk("edac_enc vs RTL",    n_edac_cw, l_edac_cw, v);
      chk("hamming_enc vs C",   n_ham_cw,  nascerr_eccs_hamming_encoder(16'(v)), v);
      chk("tbec_enc vs C",      n_tbec_cw, nascerr_eccs_tbec_encoder(16'(v)),    v);
      chk("edac_enc vs C",      n_edac_cw, nascerr_eccs_edac_encoder(16'(v)),    v);
      chk("edac systematic (cw[15:8]==data)", {8'h00, n_edac_cw[15:8]}, {8'h00, d}, v);
    end
    for (int v = 0; v < 65536; v++) begin
      cw = 16'(v); #1;
      chk("hamming_dec data",     {8'h00, n_ham_d},  l_ham_d,  v);
      chk("hamming_dec syndrome", {12'h0, n_ham_syn}, {12'h0, l_ham_syn}, v);
      chk("tbec_dec data",        {8'h00, n_tbec_d}, l_tbec_d, v);
      chk("edac_dec data",        {8'h00, n_edac_d}, l_edac_d, v);
    end
    $display("EQUIV %s: %0d mismatches (3 encoders x 256 vs RTL and C; 3 decoders x 65536)",
             (fail == 0) ? "PASS" : "FAIL", fail);
    $finish;
  end
endmodule
