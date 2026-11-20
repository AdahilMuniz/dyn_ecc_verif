class tbec_rsc_all_patterns_seq extends dyn_ecc_base_seq;

  `uvm_object_utils(tbec_rsc_all_patterns_seq)

  function new(string name = "tbec_rsc_all_patterns_seq");
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
    int                            error_cnt;
    real                           correct_rate;

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

    //@NOTE: The number of total patterns is basically 2^(Encoded Word Width)
    for (int i=0; i<(2**ENC_WORD_WIDTH); ++i) begin //Number patterns: i become a mask
       mask = i;

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
               $sformatf("READ  addr=0x%0h -> data=0x%0h", rd.addr, rd.data), UVM_HIGH)
        if(rd.data != wdata) begin
            error_cnt ++;
        end
        // ---- Restore Data ----
        p_sequencer.mem_up.poke(addr, original_enc_word[ENC_WORD_WIDTH-1:(ENC_WORD_WIDTH/2)] );
        p_sequencer.mem_down.poke(addr, original_enc_word[(ENC_WORD_WIDTH/2)-1:0] );
    end

    correct_rate = 1.0 - (real'(error_cnt)/real'(2**ENC_WORD_WIDTH));
    `uvm_info(get_type_name(), $sformatf("Correction Rate = %0f", correct_rate), UVM_HIGH)


  endtask

endclass
