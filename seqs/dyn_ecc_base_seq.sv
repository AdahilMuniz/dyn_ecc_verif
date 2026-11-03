// Base sequence for the dyn_ecc environment. Registered in the factory so it
// can be replaced (by type/instance name) with a derived sequence at run time.
// Does nothing yet.
class dyn_ecc_base_seq extends uvm_sequence #(sram_seq_item);

  `uvm_object_utils(dyn_ecc_base_seq)

  function new(string name = "dyn_ecc_base_seq");
    super.new(name);
  endfunction

  virtual task body();
    // Intentionally empty for now.
  endtask

endclass
