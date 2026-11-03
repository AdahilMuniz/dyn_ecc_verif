// write_read_seq: performs one write followed by one read (to the same
// address) on the SRAM agent. The driver fills the read item's data field with
// the value captured from the bus.
class write_read_seq extends dyn_ecc_base_seq;

  `uvm_object_utils(write_read_seq)

  function new(string name = "write_read_seq");
    super.new(name);
  endfunction

  virtual task body();
    sram_seq_item                  wr;
    sram_seq_item                  rd;
    bit [SRAM_ADDR_WIDTH-1:0]      addr;

    // ---- WRITE ----
    wr = sram_seq_item::type_id::create("wr");
    start_item(wr);
    if (!wr.randomize() with { op == SRAM_WRITE; })
      `uvm_error(get_type_name(), "WRITE item randomize failed")
    addr = wr.addr;
    finish_item(wr);
    `uvm_info(get_type_name(),
              $sformatf("WRITE addr=0x%0h data=0x%0h", wr.addr, wr.data), UVM_LOW)

    // ---- READ (same address) ----
    rd = sram_seq_item::type_id::create("rd");
    start_item(rd);
    if (!rd.randomize() with { op == SRAM_READ; addr == local::addr; })
      `uvm_error(get_type_name(), "READ item randomize failed")
    finish_item(rd);
    `uvm_info(get_type_name(),
              $sformatf("READ  addr=0x%0h -> data=0x%0h", rd.addr, rd.data), UVM_LOW)
  endtask

endclass
