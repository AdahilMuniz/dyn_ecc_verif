// ecc_single_err_seq: exhaustive single-bit error correction. For each selected
// ECC mode, writes all 256 data values (address = value), then for every
// codeword bit position b = 0..15: flips bit b of every stored word through the
// SRAM backdoor, reads every word back in the same mode (the scoreboard requires
// the original data) and restores the bit. That is 256 x 16 = 4096 corrected
// reads per mode.
// +ECC_MODE=HAMMING|TBEC|EDAC|ALL (default ALL = the three ECC modes).
class ecc_single_err_seq extends dyn_ecc_base_seq;

  `uvm_object_utils(ecc_single_err_seq)

  function new(string name = "ecc_single_err_seq");
    super.new(name);
  endfunction

  virtual task body();
    ecc_mode_e modes[$];
    bit [15:0] d;

    get_modes(modes, 0);

    foreach (modes[k]) begin
      if (modes[k] == ECC_BYPASS) continue;
      `uvm_info(get_type_name(), $sformatf("mode %s: 256 words x 16 single-bit errors", modes[k].name()), UVM_LOW)
      set_mode(modes[k]);
      for (int v = 0; v < 256; v++) wr(v, v);
      for (int b = 0; b < 16; b++) begin
        for (int v = 0; v < 256; v++) begin
          flip(v, 16'h1 << b);
          rd(v, d);
          flip(v, 16'h1 << b);
        end
      end
    end
  endtask

endclass
