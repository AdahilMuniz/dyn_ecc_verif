// Virtual sequencer for the dyn_ecc environment. It owns no driver of its own;
// it just holds handles to the real agent sequencers so a virtual sequence can
// coordinate traffic across them (SRAM/MCU transactions + sideband mode select).
// The env wires these handles in its connect_phase.

class dyn_ecc_virtual_sequencer extends uvm_sequencer;

  `uvm_component_utils(dyn_ecc_virtual_sequencer)

  sram_sequencer     mcu_seqr;
  sideband_sequencer mux_seqr;

  function new(string name, uvm_component parent);
    super.new(name, parent);
  endfunction

endclass
