// Configuration object for the SRAM agent: the virtual interface it drives,
// whether it is active (has a driver/sequencer) or passive (monitor only), and
// how many clock cycles each access is held for.

class sram_agent_cfg extends uvm_object;

  virtual sram_if         vif;
  uvm_active_passive_enum is_active = UVM_ACTIVE;

  // Clock cycles cs stays asserted per access. The asynchronous SRAM model needs
  // >= 10 ns of stable address before a write lands or read data is valid
  // (twc/taa), so with a 10 ns clock keep this >= 2; 4 leaves margin.
  int unsigned            access_cycles = 4;

  `uvm_object_utils_begin(sram_agent_cfg)
    `uvm_field_enum(uvm_active_passive_enum, is_active, UVM_ALL_ON)
    `uvm_field_int (access_cycles,                      UVM_ALL_ON)
  `uvm_object_utils_end

  function new(string name = "sram_agent_cfg");
    super.new(name);
  endfunction

endclass
