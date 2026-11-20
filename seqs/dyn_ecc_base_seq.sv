// Base (virtual) sequence for the dyn_ecc environment. It runs on the
// dyn_ecc_virtual_sequencer, so derived sequences reach the real agent
// sequencers through p_sequencer (p_sequencer.mcu_seqr / p_sequencer.mux_seqr).
// Registered in the factory so it can be replaced (by type/instance name) with
// a derived sequence at run time. Does nothing yet.
class dyn_ecc_base_seq extends uvm_sequence;

  `uvm_object_utils(dyn_ecc_base_seq)
  `uvm_declare_p_sequencer(dyn_ecc_virtual_sequencer)

  function new(string name = "dyn_ecc_base_seq");
    super.new(name);
  endfunction

  virtual task body();
    // Intentionally empty for now.
  endtask

  virtual task flip_bits(int bit_pos[], bit up_down, bit [SRAM_ADDR_WIDTH-1:0] addr);
    bit [SRAM_DATA_WIDTH-1:0] bits_to_flip;
    foreach (bit_pos[i]) begin
      bits_to_flip[bit_pos[i]] = 1'b1;
    end
    if (up_down) begin
      p_sequencer.mem_up.poke(addr, p_sequencer.mem_up.peek(addr) ^ bits_to_flip);
    end
    else begin
      p_sequencer.mem_down.poke(addr, p_sequencer.mem_down.peek(addr) ^ bits_to_flip);
    end
  endtask

  virtual task flip_bits_mask(bit [SRAM_DATA_WIDTH-1:0] mask, bit up_down, bit [SRAM_ADDR_WIDTH-1:0] addr);
    if (up_down) begin
      p_sequencer.mem_up.poke(addr, p_sequencer.mem_up.peek(addr) ^ mask);
    end
    else begin
      p_sequencer.mem_down.poke(addr, p_sequencer.mem_down.peek(addr) ^ mask);
    end
  endtask

endclass
