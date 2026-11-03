// SRAM monitor: observes the sram_if and publishes an sram_seq_item for every
// active cycle (cs asserted) through its analysis port.

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
    forever begin
      @(posedge vif.clk);
      if (vif.rstn === 1'b1 && vif.cs === 1'b1) begin
        tr      = sram_seq_item::type_id::create("tr");
        tr.op   = (vif.we === 1'b1) ? SRAM_WRITE : SRAM_READ;
        tr.addr = vif.addr;
        tr.data = (vif.we === 1'b1) ? vif.wdata : vif.rdata;
        `uvm_info(get_type_name(),
                  $sformatf("observed %s", tr.convert2string()), UVM_HIGH)
        ap.write(tr);
      end
    end
  endtask

endclass
