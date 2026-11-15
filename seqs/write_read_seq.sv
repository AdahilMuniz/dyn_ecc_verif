// write_read_seq: performs one write followed by one read (to the same
// address) on the SRAM agent. The driver fills the read item's data field with
// the value captured from the bus.
class write_read_seq extends dyn_ecc_base_seq;

  `uvm_object_utils(write_read_seq)

  // ECC/bypass mode select, matching NASCERR typedefs::sel_t encoding:
  //   0 = BYPASS_UP, 1 = BYPASS_DOWN, 2 = TBEC_RSC, 3 = MRSC.
  rand int ecc_sel;

  // TEMPORARY: exclude MRSC (3). It is still a Todo / unimplemented in the
  // NASCERR control, so selecting it exercises an incomplete path. Remove this
  // constraint once MRSC is implemented.
  constraint c_no_mrsc { ecc_sel inside {0, 1, 2}; }

  function new(string name = "write_read_seq");
    super.new(name);
  endfunction

  virtual task body();
    sram_seq_item                  wr;
    sram_seq_item                  rd;
    bit [SRAM_ADDR_WIDTH-1:0]      addr;
    sideband_seq_item              sb;

    // ---- Drive the mode onto the sideband interface ----
    // Reach the sideband sequencer through the virtual sequencer. The sideband
    // driver holds wsig until its next item, so the mode stays applied through
    // the write/read below.
    sb = sideband_seq_item::type_id::create("sb");
    start_item(sb, -1, p_sequencer.mux_seqr);
    sb.wdata = ecc_sel;         // deterministic: apply the chosen mode
    finish_item(sb);

    // ---- WRITE ----
    wr = sram_seq_item::type_id::create("wr");
    start_item(wr, -1, p_sequencer.mcu_seqr);
    if (!wr.randomize() with { op == SRAM_WRITE; })
      `uvm_error(get_type_name(), "WRITE item randomize failed")
    addr = wr.addr;
    finish_item(wr);
    `uvm_info(get_type_name(),
              $sformatf("WRITE addr=0x%0h data=0x%0h", wr.addr, wr.data), UVM_LOW)

    // ---- READ (same address) ----
    rd = sram_seq_item::type_id::create("rd");
    start_item(rd, -1, p_sequencer.mcu_seqr);
    if (!rd.randomize() with { op == SRAM_READ; addr == local::addr; })
      `uvm_error(get_type_name(), "READ item randomize failed")
    finish_item(rd);
    `uvm_info(get_type_name(),
              $sformatf("READ  addr=0x%0h -> data=0x%0h", rd.addr, rd.data), UVM_LOW)
  endtask

endclass
