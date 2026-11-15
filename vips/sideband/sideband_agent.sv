// Sideband agent: wraps the driver, sequencer and monitor. When active it
// builds the driver and sequencer; the monitor is always built. The virtual
// interface comes from the sideband_agent_cfg and is forwarded to the children.

class sideband_agent extends uvm_agent;

  `uvm_component_utils(sideband_agent)

  sideband_agent_cfg                     cfg;
  sideband_driver                        drv;
  sideband_sequencer                     seqr;
  sideband_monitor                       mon;
  uvm_analysis_port #(sideband_seq_item) ap;

  function new(string name, uvm_component parent);
    super.new(name, parent);
    ap = new("ap", this);
  endfunction

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);

    if (!uvm_config_db#(sideband_agent_cfg)::get(this, "", "cfg", cfg))
      `uvm_fatal(get_type_name(), "sideband_agent_cfg 'cfg' not set for agent")

    // Reflect the configured activity on the agent's built-in field.
    is_active = cfg.is_active;

    // Forward the virtual interface to the children via config_db.
    uvm_config_db#(virtual sideband_if)::set(this, "mon", "vif", cfg.vif);
    mon = sideband_monitor::type_id::create("mon", this);

    if (get_is_active() == UVM_ACTIVE) begin
      uvm_config_db#(virtual sideband_if)::set(this, "drv", "vif", cfg.vif);
      drv  = sideband_driver   ::type_id::create("drv",  this);
      seqr = sideband_sequencer::type_id::create("seqr", this);
    end
  endfunction

  function void connect_phase(uvm_phase phase);
    super.connect_phase(phase);
    mon.ap.connect(ap);
    if (get_is_active() == UVM_ACTIVE)
      drv.seq_item_port.connect(seqr.seq_item_export);
  endfunction

endclass
