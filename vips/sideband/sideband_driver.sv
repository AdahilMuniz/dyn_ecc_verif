// Sideband driver: pulls sideband_seq_item transactions from the sequencer and
// drives sideband_if.wsig. After the driven value has settled it captures
// sideband_if.rsig back into the item so the sequence/scoreboard can observe
// the DUT's sideband output.

class sideband_driver extends uvm_driver #(sideband_seq_item);

  `uvm_component_utils(sideband_driver)

  virtual sideband_if vif;

  function new(string name, uvm_component parent);
    super.new(name, parent);
  endfunction

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    if (!uvm_config_db#(virtual sideband_if)::get(this, "", "vif", vif))
      `uvm_fatal(get_type_name(), "virtual sideband_if 'vif' not set for driver")
  endfunction

  task run_phase(uvm_phase phase);
    // Drive a known-idle state and wait until reset is released.
    drive_idle();
    wait (vif.rstn === 1'b1);

    forever begin
      seq_item_port.get_next_item(req);
      drive_item(req);
      seq_item_port.item_done();
    end
  endtask

  // Park wsig at zero.
  task drive_idle();
    vif.wsig <= '0;
  endtask

  // Drive one sideband value, then capture the DUT's response on rsig.
  task drive_item(sideband_seq_item item);
    @(posedge vif.clk);
    vif.wsig <= item.wdata;
    `uvm_info(get_type_name(),
              $sformatf("drive wsig=0x%0h", item.wdata), UVM_HIGH)

    @(posedge vif.clk);
    item.rdata = vif.rsig;
    `uvm_info(get_type_name(),
              $sformatf("drive wsig=0x%0h -> rsig=0x%0h", item.wdata, item.rdata),
              UVM_HIGH)
  endtask

endclass
