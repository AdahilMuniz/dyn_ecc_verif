// SRAM driver: pulls sram_seq_item transactions from the sequencer and drives
// the sram_if accordingly.
//   - WRITE: assert cs, we; present addr and wdata for one clock.
//   - READ : assert cs, deassert we; present addr, then capture rdata back into
//            the item so the sequence/scoreboard can observe the read value.
//
// NOTE on polarity: this driver treats `we` as ACTIVE HIGH (we=1 => write).
// The dyn_ecc DUT uses an active-low write_en; adapt the mapping in the
// testbench bridge (or here) when wiring this agent to that DUT.

class sram_driver extends uvm_driver #(sram_seq_item);

  `uvm_component_utils(sram_driver)

  virtual sram_if vif;

  function new(string name, uvm_component parent);
    super.new(name, parent);
  endfunction

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    if (!uvm_config_db#(virtual sram_if)::get(this, "", "vif", vif))
      `uvm_fatal(get_type_name(), "virtual sram_if 'vif' not set for driver")
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

  // Park the bus in an inactive state.
  task drive_idle();
    vif.cs    <= 1'b0;
    vif.we    <= 1'b0;
    vif.addr  <= '0;
    vif.wdata <= '0;
  endtask

  // Perform one read or write transaction.
  task drive_item(sram_seq_item item);
    @(posedge vif.clk);
    vif.cs   <= 1'b1;
    vif.addr <= item.addr;

    if (item.op == SRAM_WRITE) begin
      vif.we    <= 1'b1;
      vif.wdata <= item.data;
      `uvm_info(get_type_name(),
                $sformatf("WRITE addr=0x%0h data=0x%0h", item.addr, item.data),
                UVM_HIGH)
    end
    else begin
      vif.we <= 1'b0;
      `uvm_info(get_type_name(),
                $sformatf("READ  addr=0x%0h", item.addr), UVM_HIGH)
    end

    @(posedge vif.clk);
    // On a read, return the captured data in the item.
    if (item.op == SRAM_READ) begin
      item.data = vif.rdata;
      `uvm_info(get_type_name(),
                $sformatf("READ  addr=0x%0h -> data=0x%0h", item.addr, item.data),
                UVM_HIGH)
    end

    // Return to idle.
    vif.cs <= 1'b0;
    vif.we <= 1'b0;
  endtask

endclass
