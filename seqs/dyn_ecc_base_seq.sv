// Base sequence for the dyn_ecc environment. Registered in the factory so it
// can be replaced (by type/instance name) with a derived sequence at run time
// (see dyn_ecc_base_test, +SEQ_NAME=...). Its body does nothing.
//
// Helpers for derived sequences, mirroring what the firmware does:
//   set_mode(m)       drive ecc_sel (the firmware's GPIO select)
//   wr(addr, data)    one MCU write
//   rd(addr, data)    one MCU read
//   flip(addr, mask)  flip bits in the SRAM via the backdoor (stands in for the
//                     FPGA saboteur)
class dyn_ecc_base_seq extends uvm_sequence #(sram_seq_item);

  `uvm_object_utils(dyn_ecc_base_seq)

  mux_vif_t     mux_vif;
  sram_bd_vif_t bd_vif;

  function new(string name = "dyn_ecc_base_seq");
    super.new(name);
  endfunction

  virtual task pre_body();
    if (!uvm_config_db#(mux_vif_t)::get(null, get_full_name(), "mux_if", mux_vif))
      `uvm_fatal(get_type_name(), "sideband vif 'mux_if' not found in config_db")
    if (!uvm_config_db#(sram_bd_vif_t)::get(null, get_full_name(), "sram_bd", bd_vif))
      `uvm_fatal(get_type_name(), "SRAM backdoor vif 'sram_bd' not found in config_db")
  endtask

  virtual task body();
    // Intentionally empty.
  endtask

  // Change ecc_sel between accesses (one clock before and after for margin, so
  // the monitors/scoreboard always see a stable mode for each access).
  task set_mode(ecc_mode_e mode);
    @(posedge mux_vif.clk);
    mux_vif.wsig <= mode;
    @(posedge mux_vif.clk);
  endtask

  task wr(bit [SRAM_ADDR_WIDTH-1:0] addr, bit [SRAM_DATA_WIDTH-1:0] data);
    sram_seq_item it = sram_seq_item::type_id::create("wr");
    start_item(it);
    it.op   = SRAM_WRITE;
    it.addr = addr;
    it.data = data;
    finish_item(it);
  endtask

  task rd(bit [SRAM_ADDR_WIDTH-1:0] addr, output bit [SRAM_DATA_WIDTH-1:0] data);
    sram_seq_item it = sram_seq_item::type_id::create("rd");
    start_item(it);
    it.op   = SRAM_READ;
    it.addr = addr;
    finish_item(it);
    data = it.data;
  endtask

  function void flip(bit [SRAM_ADDR_WIDTH-1:0] addr, bit [15:0] mask);
    bd_vif.flip(addr, mask);
  endfunction

  // +ECC_MODE=BYPASS|HAMMING|TBEC|EDAC|ALL (default ALL) -> modes to exercise.
  function void get_modes(ref ecc_mode_e modes[$], input bit include_bypass);
    string     arg = "ALL";
    ecc_mode_e m;
    void'($value$plusargs("ECC_MODE=%s", arg));
    modes.delete();
    m = m.first();
    do begin
      if ((arg == "ALL" && (include_bypass || m != ECC_BYPASS)) || arg == {m.name().substr(4, m.name().len()-1)})
        modes.push_back(m);
      m = m.next();
    end while (m != m.first());
    if (modes.size() == 0)
      `uvm_fatal(get_type_name(), $sformatf("+ECC_MODE=%s selects no mode (use BYPASS, HAMMING, TBEC, EDAC or ALL)", arg))
  endfunction

endclass
