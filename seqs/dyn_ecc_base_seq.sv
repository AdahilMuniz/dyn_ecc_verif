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

endclass
