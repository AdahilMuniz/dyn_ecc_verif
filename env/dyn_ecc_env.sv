// dyn_ecc environment (8->16 design): an active SRAM agent driving the MCU (FMC)
// bus, a passive SRAM agent observing the FPGA->SRAM bus, and the scoreboard.
// The virtual interfaces are published by the testbench through the config_db.
class dyn_ecc_env extends uvm_env;

  `uvm_component_utils(dyn_ecc_env)

  sram_agent         mcu_agt;
  sram_agent_cfg     mcu_cfg;
  sram_agent         mem_agt;
  sram_agent_cfg     mem_cfg;
  dyn_ecc_scoreboard sb;

  function new(string name, uvm_component parent);
    super.new(name, parent);
  endfunction

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);

    // MCU agent (drives mcu_if, the FMC side of the FPGA).
    mcu_cfg = sram_agent_cfg::type_id::create("mcu_cfg");
    mcu_cfg.is_active = UVM_ACTIVE;
    if (!uvm_config_db#(virtual sram_if)::get(this, "", "mcu_if", mcu_cfg.vif))
      `uvm_fatal(get_type_name(), "virtual sram_if 'mcu_if' not found in config_db")
    uvm_config_db#(sram_agent_cfg)::set(this, "mcu_agt", "cfg", mcu_cfg);
    mcu_agt = sram_agent::type_id::create("mcu_agt", this);

    // Memory-side agent (passive: observes the codewords the FPGA writes).
    mem_cfg = sram_agent_cfg::type_id::create("mem_cfg");
    mem_cfg.is_active = UVM_PASSIVE;
    if (!uvm_config_db#(virtual sram_if)::get(this, "", "sram0_if", mem_cfg.vif))
      `uvm_fatal(get_type_name(), "virtual sram_if 'sram0_if' not found in config_db")
    uvm_config_db#(sram_agent_cfg)::set(this, "mem_agt", "cfg", mem_cfg);
    mem_agt = sram_agent::type_id::create("mem_agt", this);

    sb = dyn_ecc_scoreboard::type_id::create("sb", this);
  endfunction

  function void connect_phase(uvm_phase phase);
    super.connect_phase(phase);
    mcu_agt.ap.connect(sb.mcu_imp);
    mem_agt.ap.connect(sb.mem_imp);
  endfunction

endclass
