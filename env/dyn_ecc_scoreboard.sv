// dyn_ecc scoreboard (8->16 design).
//
// Inputs: MCU-side accesses (mcu_imp) and memory-side accesses (mem_imp), the
// current ecc_sel (sideband vif) and a backdoor into the SRAM model.
//
// Checks
//   WRITE (MCU, mode m, data d):
//     - the SRAM now holds dyn_ecc_encode(m, d): the firmware's C encoder
//       (bypass: d itself) [ENC]
//   READ (MCU, mode m) of address a, with raw = word currently in the SRAM:
//     - bypass: data == raw [BYP]
//     - ECC, a last written in the same mode, raw differing from the written
//       codeword in <= 1 bit: data == {8'h00, d[7:0]}, i.e. clean words decode
//       and single-bit errors are corrected [DEC]
//     - ECC, >= 2 bit errors: not an error; counted (corrected / not), like
//       the firmware experiment's statistics
//     - ECC read of a word written in another mode: counted, not checked
//   End of test: the codeword observed on the FPGA->SRAM bus for every written
//   address equals the expected codeword [BUS]
//
// The first MAX_REPORTS mismatches per check/mode are reported as UVM_ERRORs;
// the rest are counted, and report_phase prints the totals.
`uvm_analysis_imp_decl(_mcu)
`uvm_analysis_imp_decl(_mem)

class dyn_ecc_scoreboard extends uvm_scoreboard;

  `uvm_component_utils(dyn_ecc_scoreboard)

  localparam int MAX_REPORTS = 5;

  uvm_analysis_imp_mcu #(sram_seq_item, dyn_ecc_scoreboard) mcu_imp;
  uvm_analysis_imp_mem #(sram_seq_item, dyn_ecc_scoreboard) mem_imp;

  mux_vif_t     mux_vif;
  sram_bd_vif_t bd_vif;

  typedef struct {
    bit [15:0] data;  // value the MCU wrote
    ecc_mode_e mode;  // mode it was written in
    bit [15:0] cw;    // codeword expected in the SRAM
  } entry_t;

  entry_t            model[bit [SRAM_ADDR_WIDTH-1:0]];
  logic [15:0]       bus_cw[bit [SRAM_ADDR_WIDTH-1:0]];

  // Statistics, indexed by mode.
  int unsigned n_write     [ecc_mode_e];
  int unsigned n_read      [ecc_mode_e];
  int unsigned n_checked   [ecc_mode_e]; // reads with a pass/fail expectation
  int unsigned n_fail      [string];     // "<CHECK>/<MODE>" -> mismatches
  int unsigned n_err1      [ecc_mode_e]; // reads with exactly 1 bit error in the SRAM
  int unsigned n_multi     [ecc_mode_e]; // reads with >= 2 bit errors
  int unsigned n_multi_ok  [ecc_mode_e]; // ... of which decoded correctly anyway
  int unsigned n_cross_mode;
  int unsigned n_unwritten;

  function new(string name, uvm_component parent);
    super.new(name, parent);
  endfunction

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    mcu_imp = new("mcu_imp", this);
    mem_imp = new("mem_imp", this);
    if (!uvm_config_db#(mux_vif_t)::get(this, "", "mux_if", mux_vif))
      `uvm_fatal(get_type_name(), "sideband vif 'mux_if' not found in config_db")
    if (!uvm_config_db#(sram_bd_vif_t)::get(this, "", "sram_bd", bd_vif))
      `uvm_fatal(get_type_name(), "SRAM backdoor vif 'sram_bd' not found in config_db")
  endfunction

  function void fail(string check, ecc_mode_e mode, string msg);
    string key = {check, "/", mode.name()};
    n_fail[key]++;
    if (n_fail[key] <= MAX_REPORTS)
      `uvm_error({"SB_", check}, {"[", mode.name(), "] ", msg,
                 (n_fail[key] == MAX_REPORTS) ? "  (further mismatches of this kind only counted)" : ""})
  endfunction

  // ---- MCU side -------------------------------------------------------------
  function void write_mcu(sram_seq_item t);
    ecc_mode_e   mode = ecc_mode_e'(mux_vif.wsig);
    logic [15:0] raw  = bd_vif.peek(t.addr);

    if (t.op == SRAM_WRITE) begin
      entry_t e;
      e.data = t.data;
      e.mode = mode;
      e.cw   = dyn_ecc_encode(mode, t.data);
      model[t.addr] = e;
      n_write[mode]++;
      if (raw !== e.cw)
        fail("ENC", mode, $sformatf("addr=0x%0h data=0x%04h: SRAM holds 0x%04h, firmware encoder expects 0x%04h",
                                    t.addr, t.data, raw, e.cw));
      return;
    end

    // READ
    n_read[mode]++;
    if (mode == ECC_BYPASS) begin
      n_checked[mode]++;
      if (t.data !== raw)
        fail("BYP", mode, $sformatf("addr=0x%0h: read 0x%04h, SRAM holds 0x%04h", t.addr, t.data, raw));
      return;
    end

    if (!model.exists(t.addr)) begin
      n_unwritten++;
      `uvm_warning(get_type_name(), $sformatf("[%s] read of never-written addr=0x%0h not checked", mode.name(), t.addr))
      return;
    end

    begin
      entry_t      e    = model[t.addr];
      bit   [15:0] exp  = {8'h00, e.data[7:0]};
      int          nerr = $countones(raw ^ e.cw);

      if (e.mode != mode) begin
        n_cross_mode++;
        `uvm_info(get_type_name(), $sformatf("[%s] read of addr=0x%0h written in %s: not checked",
                                             mode.name(), t.addr, e.mode.name()), UVM_HIGH)
        return;
      end

      if ($isunknown(raw) || nerr <= 1) begin
        n_checked[mode]++;
        if (nerr == 1) n_err1[mode]++;
        if (t.data !== exp)
          fail("DEC", mode, $sformatf("addr=0x%0h wrote 0x%02h, SRAM 0x%04h (cw 0x%04h, %0d bit error%s): read 0x%04h, expected 0x%04h",
                                      t.addr, e.data[7:0], raw, e.cw, nerr, (nerr == 1) ? "" : "s", t.data, exp));
      end
      else begin
        n_multi[mode]++;
        if (t.data === exp) n_multi_ok[mode]++;
      end
    end
  endfunction

  // ---- Memory side ----------------------------------------------------------
  function void write_mem(sram_seq_item t);
    if (t.op == SRAM_WRITE)
      bus_cw[t.addr] = t.data;
  endfunction

  function void check_phase(uvm_phase phase);
    super.check_phase(phase);
    foreach (model[a]) begin
      if (!bus_cw.exists(a))
        fail("BUS", model[a].mode, $sformatf("addr=0x%0h: no write seen on the FPGA->SRAM bus", a));
      else if (bus_cw[a] !== model[a].cw)
        fail("BUS", model[a].mode, $sformatf("addr=0x%0h: bus carried 0x%04h, expected 0x%04h", a, bus_cw[a], model[a].cw));
    end
  endfunction

  function void report_phase(uvm_phase phase);
    string s;
    int unsigned total_fail = 0;
    ecc_mode_e m;
    super.report_phase(phase);
    s = "\n  mode      writes  reads  checked  1-bit-err  >=2-bit-err (decoded ok)\n";
    m = m.first();
    do begin
      s = {s, $sformatf("  %-8s %7d %6d %8d %10d %12d (%0d)\n", m.name(),
                        n_write.exists(m)    ? n_write[m]    : 0,
                        n_read.exists(m)     ? n_read[m]     : 0,
                        n_checked.exists(m)  ? n_checked[m]  : 0,
                        n_err1.exists(m)     ? n_err1[m]     : 0,
                        n_multi.exists(m)    ? n_multi[m]    : 0,
                        n_multi_ok.exists(m) ? n_multi_ok[m] : 0)};
      m = m.next();
    end while (m != m.first());
    s = {s, $sformatf("  cross-mode reads (not checked): %0d, reads of unwritten addresses: %0d\n",
                      n_cross_mode, n_unwritten)};
    if (n_fail.num() == 0)
      s = {s, "  mismatches: none\n"};
    else
      foreach (n_fail[k]) begin
        s = {s, $sformatf("  mismatches %-14s %0d\n", k, n_fail[k])};
        total_fail += n_fail[k];
      end
    `uvm_info(get_type_name(), {"Scoreboard summary:", s}, UVM_NONE)
    if (total_fail != 0)
      `uvm_error(get_type_name(), $sformatf("%0d scoreboard mismatch(es) in total", total_fail))
  endfunction

endclass
