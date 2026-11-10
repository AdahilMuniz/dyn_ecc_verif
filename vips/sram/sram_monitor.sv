// SRAM monitor: observes the sram_if and publishes ONE sram_seq_item per access
// through its analysis port. An access is a run of clocks with cs asserted; the
// item carries the values sampled on its last clock (so a READ carries the
// settled read data) and is published on the first clock with cs deasserted.
// Accesses must therefore be separated by at least one idle clock, which
// sram_driver guarantees.

class sram_monitor extends uvm_monitor;

  `uvm_component_utils(sram_monitor)

  virtual sram_if                     vif;
  uvm_analysis_port #(sram_seq_item)  ap;

  function new(string name, uvm_component parent);
    super.new(name, parent);
    ap = new("ap", this);
  endfunction

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    if (!uvm_config_db#(virtual sram_if)::get(this, "", "vif", vif))
      `uvm_fatal(get_type_name(), "virtual sram_if 'vif' not set for monitor")
  endfunction

  task run_phase(uvm_phase phase);
    sram_seq_item tr;
    bit           in_access = 0;
    forever begin
      @(posedge vif.clk);
      if (vif.rstn !== 1'b1) begin
        in_access = 0;
      end
      else if (vif.cs === 1'b1) begin
        if (!in_access)
          tr = sram_seq_item::type_id::create("tr");
        tr.op   = (vif.we === 1'b1) ? SRAM_WRITE : SRAM_READ;
        tr.addr = vif.addr;
        tr.data = (vif.we === 1'b1) ? vif.wdata : vif.rdata;
        in_access = 1;
      end
      else if (in_access) begin
        in_access = 0;
        `uvm_info(get_type_name(),
                  $sformatf("observed %s", tr.convert2string()), UVM_HIGH)
        ap.write(tr);
      end
    end
  endtask

endclass
