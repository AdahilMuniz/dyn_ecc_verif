// dyn_ecc environment: instantiates and configures the SRAM verification IP.
// The virtual interfaces are published by the testbench through the config_db;
// here we retrieve the one the MCU agent needs, build the agent config, and
// create the agent.
class dyn_ecc_env extends uvm_env;

  `uvm_component_utils(dyn_ecc_env)

  sram_agent                 mcu_agt;
  sram_agent_cfg             mcu_cfg;

  sideband_agent             mux_agt;
  sideband_agent_cfg         mux_cfg;

  dyn_ecc_virtual_sequencer  v_seqr;

  sram_mem_model             mem_up;
  sram_mem_model             mem_down;

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

    // Configure the sideband agent (drives the mux_if sideband select/flags).
    mux_cfg = sideband_agent_cfg::type_id::create("mux_cfg");
    mux_cfg.is_active = UVM_ACTIVE;
    if (!uvm_config_db#(virtual sideband_if)::get(this, "", "mux_if", mux_cfg.vif))
      `uvm_fatal(get_type_name(),
                 "virtual sideband_if 'mux_if' not found in config_db")

    uvm_config_db#(sideband_agent_cfg)::set(this, "mux_agt", "cfg", mux_cfg);
    mux_agt = sideband_agent::type_id::create("mux_agt", this);

    // Virtual sequencer: coordinates the MCU and sideband agents.
    v_seqr = dyn_ecc_virtual_sequencer::type_id::create("v_seqr", this);

    // Retrieve the backdoor memory models published by the testbench.
    if (!uvm_config_db#(sram_mem_model)::get(this, "", "mem_up", mem_up))
      `uvm_fatal(get_type_name(),
                 "sram_mem_model 'mem_up' not found in config_db")
    if (!uvm_config_db#(sram_mem_model)::get(this, "", "mem_down", mem_down))
      `uvm_fatal(get_type_name(),
                 "sram_mem_model 'mem_down' not found in config_db")
  endfunction

  function void connect_phase(uvm_phase phase);
    super.connect_phase(phase);

    // Hook the virtual sequencer's handles to the real agent sequencers and the
    // backdoor memory models, so sequences reach them through p_sequencer.
    v_seqr.mcu_seqr = mcu_agt.seqr;
    v_seqr.mux_seqr = mux_agt.seqr;
    v_seqr.mem_up   = mem_up;
    v_seqr.mem_down = mem_down;
  endfunction

endclass
