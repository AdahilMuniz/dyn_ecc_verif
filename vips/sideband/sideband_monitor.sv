// Sideband monitor: observes sideband_if and publishes a sideband_seq_item
// through its analysis port.
//
// NOTE: sideband_if has no valid/handshake qualifier (unlike sram_if's cs), so
// there is no unambiguous "a transaction happened" event to sample on. This
// monitor publishes whenever wsig or rsig changes value (after reset), which
// captures every observable sideband state transition without flooding the
// analysis port on every clock. Change this policy if a cycle-accurate sample
// is required.

class sideband_monitor extends uvm_monitor;

  `uvm_component_utils(sideband_monitor)

  virtual sideband_if                     vif;
  uvm_analysis_port #(sideband_seq_item)  ap;

  function new(string name, uvm_component parent);
    super.new(name, parent);
    ap = new("ap", this);
  endfunction

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    if (!uvm_config_db#(virtual sideband_if)::get(this, "", "vif", vif))
      `uvm_fatal(get_type_name(), "virtual sideband_if 'vif' not set for monitor")
  endfunction

  task run_phase(uvm_phase phase);
    sideband_seq_item     tr;
    bit [SB_WWIDTH-1:0]   last_wsig;
    bit [SB_RWIDTH-1:0]   last_rsig;
    bit                   first = 1'b1;

    forever begin
      @(posedge vif.clk);
      if (vif.rstn === 1'b1) begin
        if (first || vif.wsig !== last_wsig || vif.rsig !== last_rsig) begin
          tr       = sideband_seq_item::type_id::create("tr");
          tr.wdata = vif.wsig;
          tr.rdata = vif.rsig;
          `uvm_info(get_type_name(),
                    $sformatf("observed %s", tr.convert2string()), UVM_HIGH)
          ap.write(tr);

          last_wsig = vif.wsig;
          last_rsig = vif.rsig;
          first     = 1'b0;
        end
      end
    end
  endtask

endclass
