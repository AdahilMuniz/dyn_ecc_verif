// SRAM driver: pulls sram_seq_item transactions from the sequencer and drives
// the sram_if like the STM32 FMC drives an asynchronous SRAM:
//   - an access asserts cs (and we for a write) with a stable address (and wdata)
//     for `access_cycles` clocks;
//   - a READ captures rdata on the last clock of the access and returns it in
//     the item, so the sequence can see the value after finish_item();
//   - every access is followed by one idle clock (cs = 0), which the monitor
//     uses to tell accesses apart.
//
// Signals are logical active high (we=1 => write); the testbench top maps them
// to the active-low pins.

class sram_driver extends uvm_driver #(sram_seq_item);

  `uvm_component_utils(sram_driver)

  virtual sram_if vif;
  int unsigned    access_cycles = 4; // set by the agent from sram_agent_cfg

  function new(string name, uvm_component parent);
    super.new(name, parent);
  endfunction

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    if (!uvm_config_db#(virtual sram_if)::get(this, "", "vif", vif))
      `uvm_fatal(get_type_name(), "virtual sram_if 'vif' not set for driver")
    if (access_cycles == 0)
      `uvm_fatal(get_type_name(), "access_cycles must be >= 1")
  endfunction

  task run_phase(uvm_phase phase);
    // Park the bus and wait until reset is released.
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

  // Perform one read or write access, then one idle clock.
  task drive_item(sram_seq_item item);
    @(posedge vif.clk);
    vif.cs   <= 1'b1;
    vif.addr <= item.addr;
    if (item.op == SRAM_WRITE) begin
      vif.we    <= 1'b1;
      vif.wdata <= item.data;
    end
    else begin
      vif.we    <= 1'b0;
    end

    // Hold the access.
    repeat (access_cycles) @(posedge vif.clk);

    // On a read, return the data present on the last clock of the access.
    if (item.op == SRAM_READ)
      item.data = vif.rdata;

    `uvm_info(get_type_name(),
              $sformatf("%s addr=0x%0h data=0x%0h",
                        (item.op == SRAM_WRITE) ? "WRITE" : "READ ",
                        item.addr, item.data), UVM_HIGH)

    // End the access (the address is left unchanged) and idle for one clock.
    vif.cs <= 1'b0;
    vif.we <= 1'b0;
    @(posedge vif.clk);
  endtask

endclass
