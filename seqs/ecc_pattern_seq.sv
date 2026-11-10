// ecc_pattern_seq: the 8-bit firmware's block-by-block experiment
// (NASCERR_Experiment.c, nascerr_experiment_block_by_block_data), with no errors
// injected. For each selected mode:
//   1. write N words with the firmware patterns 0x00/0xFF/0x55/0xAA/random
//      (8-bit, like the firmware; 16-bit random in bypass);
//   2. read them back in the same mode (decoded data);
//   3. read them back in BYPASS (the raw stored codeword).
// +ECC_MODE=... picks the modes (default: all four); +N_WORDS=... sets N (default 256).
class ecc_pattern_seq extends dyn_ecc_base_seq;

  `uvm_object_utils(ecc_pattern_seq)

  function new(string name = "ecc_pattern_seq");
    super.new(name);
  endfunction

  virtual task body();
    ecc_mode_e   modes[$];
    int unsigned n_words = 256;
    bit [15:0]   d;

    void'($value$plusargs("N_WORDS=%d", n_words));
    get_modes(modes, 1);

    foreach (modes[k]) begin
      `uvm_info(get_type_name(), $sformatf("mode %s: %0d words", modes[k].name(), n_words), UVM_LOW)
      set_mode(modes[k]);
      for (int unsigned i = 0; i < n_words; i++) begin
        case (i % 5)
          0: d = 16'h0000;
          1: d = 16'h00FF;
          2: d = 16'h0055;
          3: d = 16'h00AA;
          4: d = (modes[k] == ECC_BYPASS) ? 16'($urandom) : 16'($urandom_range(255));
        endcase
        wr(i, d);
      end
      for (int unsigned i = 0; i < n_words; i++) rd(i, d);
      if (modes[k] != ECC_BYPASS) begin
        set_mode(ECC_BYPASS);
        for (int unsigned i = 0; i < n_words; i++) rd(i, d);
      end
    end
  endtask

endclass
