// dyn_ecc environment: instantiates and configures the SRAM verification IP.
// The virtual interfaces are published by the testbench through the config_db;
// here we retrieve the one the MCU agent needs, build the agent config, and
// create the agent.
class dyn_ecc_env extends uvm_env;

  `uvm_component_utils(dyn_ecc_env)

  sram_agent     mcu_agt;
  sram_agent_cfg mcu_cfg;

  function new(string name, uvm_component parent);
    super.new(name, parent);
  endfunction

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);

    // Configure the MCU agent (drives the mcu_if).
    mcu_cfg = sram_agent_cfg::type_id::create("mcu_cfg");
    mcu_cfg.is_active = UVM_ACTIVE;
    if (!uvm_config_db#(virtual sram_if)::get(this, "", "mcu_if", mcu_cfg.vif))
      `uvm_fatal(get_type_name(),
                 "virtual sram_if 'mcu_if' not found in config_db")

    // Hand the config to the agent and build it.
    uvm_config_db#(sram_agent_cfg)::set(this, "mcu_agt", "cfg", mcu_cfg);
    mcu_agt = sram_agent::type_id::create("mcu_agt", this);
  endfunction

  function void connect_phase(uvm_phase phase);
    super.connect_phase(phase);
    // No scoreboard/connections yet.
  endfunction

endclass
