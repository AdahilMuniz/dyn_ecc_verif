// Configuration object for the SRAM agent: the virtual interface it drives and
// whether it is active (has a driver/sequencer) or passive (monitor only).

class sram_agent_cfg extends uvm_object;

  virtual sram_if         vif;
  uvm_active_passive_enum is_active = UVM_ACTIVE;

  `uvm_object_utils_begin(sram_agent_cfg)
    `uvm_field_enum(uvm_active_passive_enum, is_active, UVM_ALL_ON)
  `uvm_object_utils_end

  function new(string name = "sram_agent_cfg");
    super.new(name);
  endfunction

endclass
