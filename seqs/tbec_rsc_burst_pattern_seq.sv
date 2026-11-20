class tbec_rsc_burst_pattern_seq extends dyn_ecc_base_seq;

  `uvm_object_utils(tbec_rsc_burst_pattern_seq)

  parameter MAX_BURST_LENGTH = 8;

  function new(string name = "tbec_rsc_burst_pattern_seq");
    super.new(name);
  endfunction

  virtual task body();
    sram_seq_item                  wr;
    sram_seq_item                  rd;
    sideband_seq_item              sb;
    bit [SRAM_ADDR_WIDTH-1:0]      addr;
    bit [SRAM_DATA_WIDTH-1:0]      wdata;
    bit [ENC_WORD_WIDTH-1:0]       original_enc_word;
    bit [ENC_WORD_WIDTH-1:0]       mask;
    bit [ENC_WORD_WIDTH-1:0]       base;                                // l contiguous ones
    int                            nbits;                               // #bits set in mask
    int                            error_cnt;                           // overall mismatches
    int                            total_tested;                        // total patterns swept
    int                            error_cnt_by_k [MAX_BURST_LENGTH+1];   // mismatches per #flipped bits
    int                            total_by_k     [MAX_BURST_LENGTH+1];   // patterns per #flipped bits
    real                           correct_rate;

    error_cnt_by_k = '{default: 0};
    total_by_k     = '{default: 0};

    // ---- Drive the mode onto the sideband interface ----
    // Reach the sideband sequencer through the virtual sequencer. The sideband
    // driver holds wsig until its next item, so the mode stays applied through
    // the write/read below.
    sb = sideband_seq_item::type_id::create("sb");
    start_item(sb, -1, p_sequencer.mux_seqr);
    sb.wdata = TBEC_RSC;         // deterministic: apply the chosen mode
    finish_item(sb);

    // ---- WRITE ----
    wr = sram_seq_item::type_id::create("wr");
    start_item(wr, -1, p_sequencer.mcu_seqr);
    if (!wr.randomize() with { op == SRAM_WRITE; })
      `uvm_error(get_type_name(), "WRITE item randomize failed")
    addr = wr.addr;
    wdata = wr.data;
    finish_item(wr);
    `uvm_info(get_type_name(),
              $sformatf("WRITE addr=0x%0h data=0x%0h", wr.addr, wr.data), UVM_HIGH)
    
    #1ps;//@TOSOÇ This is a workaround, maybe worth export the interface to sequences to wait clocks
    original_enc_word[ENC_WORD_WIDTH-1:(ENC_WORD_WIDTH/2)] = p_sequencer.mem_up.peek(addr);
    original_enc_word[(ENC_WORD_WIDTH/2)-1:0] = p_sequencer.mem_down.peek(addr);

    // Continuous burst of length l = l contiguous '1's, slid across every
    // start position. For an N-bit codeword there are (N - l + 1) positions.
    for (int l=1; l<=MAX_BURST_LENGTH; ++l) begin
       nbits = l;                    // a continuous burst of length l flips l bits
       base  = (1 << l) - 1;         // l contiguous ones, e.g. l=3 -> 0b111

       for (int s=0; s<=(ENC_WORD_WIDTH-l); ++s) begin // slide: shift base by s
           mask = base << s;
           total_by_k[nbits]++;

           // ---- Insert Error ---
           flip_bits_mask(.mask(mask[ENC_WORD_WIDTH-1:(ENC_WORD_WIDTH/2)]), .up_down(1'b0), .addr(addr));
           flip_bits_mask(.mask(mask[(ENC_WORD_WIDTH/2)-1:0]), .up_down(1'b1), .addr(addr));

           // ---- READ (same address) ----
           rd = sram_seq_item::type_id::create("rd");
           start_item(rd, -1, p_sequencer.mcu_seqr);
           if (!rd.randomize() with { op == SRAM_READ; addr == local::addr; })
           `uvm_error(get_type_name(), "READ item randomize failed")
           finish_item(rd);
           `uvm_info(get_type_name(),
                $sformatf("READ  addr=0x%0h mask=0x%0h -> data=0x%0h", rd.addr, mask, rd.data), UVM_HIGH)
            if(rd.data != wdata) begin
                error_cnt ++;
                error_cnt_by_k[nbits] ++;
            end
            // ---- Restore Data ----
            p_sequencer.mem_up.poke(addr, original_enc_word[ENC_WORD_WIDTH-1:(ENC_WORD_WIDTH/2)] );
            p_sequencer.mem_down.poke(addr, original_enc_word[(ENC_WORD_WIDTH/2)-1:0] );
        end
    end

    // Per-error-count correction rate (k = number of flipped bits).
    foreach (total_by_k[k]) begin
      if (total_by_k[k] > 0) begin
        correct_rate = 1.0 - (real'(error_cnt_by_k[k]) / real'(total_by_k[k]));
        `uvm_info(get_type_name(),
                  $sformatf("Continuous Burst N errors=%2d : corrected %0d/%0d (rate=%0f)",
                            k, total_by_k[k]-error_cnt_by_k[k],
                            total_by_k[k], correct_rate), UVM_LOW)
      end
    end

    // Overall correction rate across all burst patterns actually swept.
    total_tested = 0;
    foreach (total_by_k[k]) total_tested += total_by_k[k];
    correct_rate = 1.0 - (real'(error_cnt)/real'(total_tested));
    `uvm_info(get_type_name(),
              $sformatf("Overall Continuous-Burst Correction Rate = %0f (%0d patterns)",
                        correct_rate, total_tested), UVM_LOW)


  endtask

endclass
