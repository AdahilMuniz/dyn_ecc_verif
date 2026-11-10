// ecc_burst_err_seq: the burst-error experiment of the standalone STM32 test
// firmware (nascerr_8to16_legacy/firmware/2024-08_stm_test_mem_fw, burstErrorRSC/Hamming/EDAC).
//
// For each selected ECC mode and each burst length n:
//   write N random 8-bit words, flip n ADJACENT codeword bits at a random
//   position (0 .. 16-n) in each stored word, read every word back in the same
//   mode, and count the words that come back correct.
// The firmware injects the burst with a bypass write; this uses the SRAM backdoor.
//
// +BURST_POS=0 fixes the burst at codeword bits [n-1:0]. With n = 1..3 these
// are exactly the 1/2/3-bit patterns the Oct 2024 FPGA saboteur
// (nascerr_8to16_legacy/fpga/2024-10_fpga_1memory_saboteur, Encoder16bits_random) XORs into
// cw[2:0]: 3'b001, 3'b011, 3'b111.
//
// n = 1 is checked by the scoreboard (must be corrected); n >= 2 is only
// counted, both here and in the scoreboard summary.
// Plusargs: +ECC_MODE=HAMMING|TBEC|EDAC|ALL  +BURST=<n> (default 1..3)
//           +BURST_POS=<p> (default random)  +N_WORDS=<N> (default 1000, like the firmware)
class ecc_burst_err_seq extends dyn_ecc_base_seq;

  `uvm_object_utils(ecc_burst_err_seq)

  function new(string name = "ecc_burst_err_seq");
    super.new(name);
  endfunction

  virtual task body();
    ecc_mode_e   modes[$];
    int unsigned n_words = 1000;
    int          burst_only = 0;
    int          fixed_pos  = -1;
    bit [7:0]    data[];
    bit [15:0]   d;
    string       s = "";

    void'($value$plusargs("N_WORDS=%d", n_words));
    void'($value$plusargs("BURST=%d", burst_only));
    void'($value$plusargs("BURST_POS=%d", fixed_pos));
    get_modes(modes, 0);
    data = new[n_words];

    foreach (modes[k]) begin
      if (modes[k] == ECC_BYPASS) continue;
      set_mode(modes[k]);
      for (int n = 1; n <= 3; n++) begin
        int unsigned ok = 0;
        if (burst_only != 0 && n != burst_only) continue;
        for (int unsigned i = 0; i < n_words; i++) begin
          int pos = (fixed_pos >= 0) ? fixed_pos : $urandom_range(16 - n);
          data[i] = $urandom_range(255);
          wr(i, data[i]);
          flip(i, ((16'h1 << n) - 1) << pos);
        end
        for (int unsigned i = 0; i < n_words; i++) begin
          rd(i, d);
          if (d == {8'h00, data[i]}) ok++;
        end
        s = {s, $sformatf("\n  %-11s burst=%0d pos=%-6s correct %4d/%0d (%5.1f%%)",
                          modes[k].name(), n, (fixed_pos >= 0) ? $sformatf("%0d", fixed_pos) : "random",
                          ok, n_words, 100.0 * ok / n_words)};
      end
    end
    `uvm_info(get_type_name(), {"Burst-error results (like the firmware's count):", s}, UVM_NONE)
  endtask

endclass
