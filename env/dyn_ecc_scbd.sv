// dyn_ecc scoreboard. Receives transactions from each monitored interface via
// its own analysis implementation port. Separate ports are declared with
// `uvm_analysis_imp_decl so that a single component can expose one write method
// per source (write_mcu, write_sb, write_sram0, write_sram1).
//
// The write methods are intentionally empty for now - this wires up the TLM
// plumbing; the checking logic will be added later.

// Declare the specialized analysis imp ports (one suffix per source).
`uvm_analysis_imp_decl(_mcu)
`uvm_analysis_imp_decl(_sb)
`uvm_analysis_imp_decl(_sram0)
`uvm_analysis_imp_decl(_sram1)

class dyn_ecc_scbd extends uvm_scoreboard;

  `uvm_component_utils(dyn_ecc_scbd)

  // One analysis imp per interface. The MCU and the two SRAMs carry
  // sram_seq_item; the sideband carries sideband_seq_item.
  uvm_analysis_imp_mcu   #(sram_seq_item,     dyn_ecc_scbd) mcu_imp;
  uvm_analysis_imp_sb    #(sideband_seq_item, dyn_ecc_scbd) sb_imp;
  uvm_analysis_imp_sram0 #(sram_seq_item,     dyn_ecc_scbd) sram0_imp;
  uvm_analysis_imp_sram1 #(sram_seq_item,     dyn_ecc_scbd) sram1_imp;

  // Per-interface FIFOs holding the observed data words, in arrival order.
  // Split by operation: write transactions and read transactions each queue.
  bit [SRAM_DATA_WIDTH-1:0] mcu_write_t_q   [$];
  bit [SRAM_DATA_WIDTH-1:0] mcu_read_t_q    [$];
  bit [SRAM_DATA_WIDTH-1:0] sram0_write_t_q [$];
  bit [SRAM_DATA_WIDTH-1:0] sram0_read_t_q  [$];
  bit [SRAM_DATA_WIDTH-1:0] sram1_write_t_q [$];
  bit [SRAM_DATA_WIDTH-1:0] sram1_read_t_q  [$];
  sel_t ecc_write_cfg_q [$]; // ECC Configuration when the Data is written
  sel_t ecc_read_cfg_q [$]; // ECC Configuration when the Data is read

  // ECC Selection
  sel_t ecc_sel; // Current ECC configuration

  function new(string name, uvm_component parent);
    super.new(name, parent);
  endfunction

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    mcu_imp   = new("mcu_imp",   this);
    sb_imp    = new("sb_imp",    this);
    sram0_imp = new("sram0_imp", this);
    sram1_imp = new("sram1_imp", this);
  endfunction

  // ---- Analysis write methods (one per source; empty for now) ---------------
  virtual function void write_mcu(sram_seq_item t);
    if (t.op == SRAM_WRITE) begin
      mcu_write_t_q.push_back(t.data);
      `uvm_info(get_type_name(),
                $sformatf("pushed mcu_write_t_q <= 0x%0h", t.data), UVM_HIGH)
      ecc_write_cfg_q.push_back(ecc_sel);
    end
    else begin
      mcu_read_t_q.push_back(t.data);
      `uvm_info(get_type_name(),
                $sformatf("pushed mcu_read_t_q <= 0x%0h", t.data), UVM_HIGH)
      ecc_read_cfg_q.push_back(ecc_sel);
    end
    
  endfunction

  virtual function void write_sb(sideband_seq_item t);
    // TODO: collect the sideband (mode-select) transaction.
    ecc_sel = t.wdata;
  endfunction

  virtual function void write_sram0(sram_seq_item t);
    if (t.op == SRAM_WRITE) begin
      sram0_write_t_q.push_back(t.data);
      `uvm_info(get_type_name(),
                $sformatf("pushed sram0_write_t_q <= 0x%0h", t.data), UVM_HIGH)
    end
    else begin
      sram0_read_t_q.push_back(t.data);
      `uvm_info(get_type_name(),
                $sformatf("pushed sram0_read_t_q <= 0x%0h", t.data), UVM_HIGH)
    end
  endfunction

  virtual function void write_sram1(sram_seq_item t);
    if (t.op == SRAM_WRITE) begin
      sram1_write_t_q.push_back(t.data);
      `uvm_info(get_type_name(),
                $sformatf("pushed sram1_write_t_q <= 0x%0h", t.data), UVM_HIGH)
    end
    else begin
      sram1_read_t_q.push_back(t.data);
      `uvm_info(get_type_name(),
                $sformatf("pushed sram1_read_t_q <= 0x%0h", t.data), UVM_HIGH)
    end
  endfunction

  // ---- End-of-test checking -------------------------------------------------
  function void check_phase(uvm_phase phase);
    sel_t                     cfg;
    bit [SRAM_DATA_WIDTH-1:0] mcu_d, s0_d, s1_d;
    logic [15:0]              enc;
    logic [7:0]               dec;

    super.check_phase(phase);

    // ---- Queue-size consistency ---------------------------------------------
    assert (mcu_write_t_q.size() == sram0_write_t_q.size() &&
            mcu_write_t_q.size() == sram1_write_t_q.size() &&
            mcu_write_t_q.size() == ecc_write_cfg_q.size())
      else `uvm_error(get_type_name(),
        $sformatf("WRITE queue size mismatch: mcu=%0d sram0=%0d sram1=%0d cfg=%0d",
                  mcu_write_t_q.size(), sram0_write_t_q.size(),
                  sram1_write_t_q.size(), ecc_write_cfg_q.size()))

    assert (mcu_read_t_q.size() == sram0_read_t_q.size() &&
            mcu_read_t_q.size() == sram1_read_t_q.size() &&
            mcu_read_t_q.size() == ecc_read_cfg_q.size())
      else `uvm_error(get_type_name(),
        $sformatf("READ queue size mismatch: mcu=%0d sram0=%0d sram1=%0d cfg=%0d",
                  mcu_read_t_q.size(), sram0_read_t_q.size(),
                  sram1_read_t_q.size(), ecc_read_cfg_q.size()))

    // ---- WRITE path: MCU data vs what landed in the SRAMs -------------------
    while (ecc_write_cfg_q.size() > 0) begin
      cfg   = ecc_write_cfg_q.pop_front();
      case (cfg)
        BYPASS_UP: begin
          mcu_d = mcu_write_t_q.pop_front();
          s0_d  = sram0_write_t_q.pop_front();
          s1_d  = sram1_write_t_q.pop_front();
          `uvm_info(get_type_name(),
                    $sformatf("WRITE BYPASS_UP: comparing mcu=0x%0h vs sram0=0x%0h",
                              mcu_d[7:0], s0_d[7:0]), UVM_HIGH)
          assert (mcu_d[7:0] == s0_d[7:0])
            else `uvm_error(get_type_name(),
              $sformatf("WRITE BYPASS_UP mismatch: mcu=0x%0h sram0=0x%0h",
                        mcu_d[7:0], s0_d[7:0]))
        end
        BYPASS_DOWN: begin
          mcu_d = mcu_write_t_q.pop_front();
          s0_d  = sram0_write_t_q.pop_front();
          s1_d  = sram1_write_t_q.pop_front();
          `uvm_info(get_type_name(),
                    $sformatf("WRITE BYPASS_DOWN: comparing mcu=0x%0h vs sram1=0x%0h",
                              mcu_d[7:0], s1_d[7:0]), UVM_HIGH)
          assert (mcu_d[7:0] == s1_d[7:0])
            else `uvm_error(get_type_name(),
              $sformatf("WRITE BYPASS_DOWN mismatch: mcu=0x%0h sram1=0x%0h",
                        mcu_d[7:0], s1_d[7:0]))
        end
        TBEC_RSC: begin
          mcu_d = mcu_write_t_q.pop_front();
          s0_d  = sram0_write_t_q.pop_front();
          s1_d  = sram1_write_t_q.pop_front();
          enc   = enc_tbec_rsc(mcu_d[7:0]);
          `uvm_info(get_type_name(),
                    $sformatf("WRITE TBEC_RSC: enc(mcu=0x%0h)=0x%0h; comparing msb=0x%0h vs sram0=0x%0h, lsb=0x%0h vs sram1=0x%0h",
                              mcu_d[7:0], enc, enc[15:8], s0_d[7:0], enc[7:0], s1_d[7:0]),
                    UVM_HIGH)
          assert (enc[15:8] == s0_d[7:0])
            else `uvm_error(get_type_name(),
              $sformatf("WRITE TBEC_RSC up mismatch: enc_msb=0x%0h sram0=0x%0h",
                        enc[15:8], s0_d[7:0]))
          assert (enc[7:0] == s1_d[7:0])
            else `uvm_error(get_type_name(),
              $sformatf("WRITE TBEC_RSC down mismatch: enc_lsb=0x%0h sram1=0x%0h",
                        enc[7:0], s1_d[7:0]))
        end
        MRSC: begin
          // @TODO: MRSC write check.
        end
      endcase
    end

    // ---- READ path: SRAM contents (decoded) vs MCU read data ----------------
    while (ecc_read_cfg_q.size() > 0) begin
      cfg   = ecc_read_cfg_q.pop_front();
      case (cfg)
        BYPASS_UP: begin
          mcu_d = mcu_read_t_q.pop_front();
          s0_d  = sram0_read_t_q.pop_front();
          s1_d  = sram1_read_t_q.pop_front();
          `uvm_info(get_type_name(),
                    $sformatf("READ BYPASS_UP: comparing mcu=0x%0h vs sram0=0x%0h",
                              mcu_d[7:0], s0_d[7:0]), UVM_HIGH)
          assert (mcu_d[7:0] == s0_d[7:0])
            else `uvm_error(get_type_name(),
              $sformatf("READ BYPASS_UP mismatch: mcu=0x%0h sram0=0x%0h",
                        mcu_d[7:0], s0_d[7:0]))
        end
        BYPASS_DOWN: begin
          mcu_d = mcu_read_t_q.pop_front();
          s0_d  = sram0_read_t_q.pop_front();
          s1_d  = sram1_read_t_q.pop_front();
          `uvm_info(get_type_name(),
                    $sformatf("READ BYPASS_DOWN: comparing mcu=0x%0h vs sram1=0x%0h",
                              mcu_d[7:0], s1_d[7:0]), UVM_HIGH)
          assert (mcu_d[7:0] == s1_d[7:0])
            else `uvm_error(get_type_name(),
              $sformatf("READ BYPASS_DOWN mismatch: mcu=0x%0h sram1=0x%0h",
                        mcu_d[7:0], s1_d[7:0]))
        end
        TBEC_RSC: begin
          mcu_d = mcu_read_t_q.pop_front();
          s0_d  = sram0_read_t_q.pop_front();
          s1_d  = sram1_read_t_q.pop_front();
          dec   = dec_tbec_rsc({s0_d[7:0], s1_d[7:0]});
          `uvm_info(get_type_name(),
                    $sformatf("READ TBEC_RSC: dec({sram0=0x%0h,sram1=0x%0h})=0x%0h; comparing vs mcu=0x%0h",
                              s0_d[7:0], s1_d[7:0], dec, mcu_d[7:0]), UVM_HIGH)
          assert (dec == mcu_d[7:0])
            else `uvm_error(get_type_name(),
              $sformatf("READ TBEC_RSC mismatch: dec=0x%0h mcu=0x%0h",
                        dec, mcu_d[7:0]))
        end
        MRSC: begin
          // @TODO: MRSC read check.
        end
      endcase
    end
  endfunction

  // ---- Reference models -----------------------------------------------------
  // Software mirror of NASCERR/rtl/tbec_rsc_encoder.sv: 8-bit data in, 16-bit
  // codeword out.
  // @TODO: This is a RTL mirror what is not ideal, but we do not have the 
  // documentation yet for 8 bits. There is probaly a issue with this model. When
  // we have, adapt this methos
  function logic [15:0] enc_tbec_rsc(logic [7:0] data_in);
    logic [1:0] Di, P, Cb0, Cb1;

    Di  = {data_in[1] ^ data_in[3] ^ data_in[4] ^ data_in[6],
           data_in[0] ^ data_in[2] ^ data_in[5] ^ data_in[7]};
    P   = {data_in[2] ^ data_in[3] ^ data_in[6] ^ data_in[7],
           data_in[0] ^ data_in[1] ^ data_in[4] ^ data_in[5]};
    Cb1 = {data_in[5] ^ data_in[7], data_in[1] ^ data_in[3]};
    Cb0 = {data_in[4] ^ data_in[6], data_in[0] ^ data_in[2]};

    return {Cb1, Cb0, P, Di, data_in};
  endfunction

  // Software mirror of NASCERR/rtl/tbec_rsc_decoder.sv: 16-bit codeword in,
  // 8-bit corrected data out.
  // @TODO: This is a RTL mirror what is not ideal, but we do not have the 
  // documentation yet for 8 bits. There is probaly a issue with this model. When
  // we have, adapt this methos
  function logic [7:0] dec_tbec_rsc(logic [15:0] data_in);
    logic [7:0] signal, red;
    logic [1:0] linsin [0:1];
    logic [1:0] SP, SDi;
    logic [2:0] sumSL;
    logic [1:0] sumSP, sumSDi;

    {red, signal} = data_in;

    linsin[1] = {signal[5] ^ signal[7] ^ red[7], signal[4] ^ signal[6] ^ red[6]};
    linsin[0] = {signal[1] ^ signal[3] ^ red[5], signal[0] ^ signal[2] ^ red[4]};

    SP  = {signal[2] ^ signal[3] ^ signal[6] ^ signal[7] ^ red[3],
           signal[0] ^ signal[1] ^ signal[4] ^ signal[5] ^ red[2]};
    SDi = {signal[1] ^ signal[3] ^ signal[4] ^ signal[6] ^ red[1],
           signal[0] ^ signal[2] ^ signal[5] ^ signal[7] ^ red[0]};

    sumSP  = {1'b0, SP[0]}  + {1'b0, SP[1]};
    sumSDi = {1'b0, SDi[0]} + {1'b0, SDi[1]};
    sumSL  = {2'b0, linsin[0][0]} + {2'b0, linsin[0][1]}
           + {2'b0, linsin[1][0]} + {2'b0, linsin[1][1]};

    if (((SP != '0 && SDi != '0) || sumSL > 1) &&
        !(sumSDi == 0 && sumSP == 1 && sumSL >= 2)) begin
      if (SP[0] > SP[1]) begin
        for (int i = 0; i < 2; i++) begin
          if (linsin[i] != '0) begin
            signal[i]   ^= linsin[i][0];
            signal[i+4] ^= linsin[i][1];
          end
        end
      end else if (SP[0] < SP[1]) begin
        for (int i = 2; i < 4; i++) begin
          if (linsin[i-2] != '0) begin
            signal[i]   ^= linsin[i-2][0];
            signal[i+4] ^= linsin[i-2][1];
          end
        end
      end else if (SP[0] == SP[1] && {SDi, SP} != '0) begin
        for (int i = 1; i < 3; i++) begin
          if (linsin[i-1] != '0) begin
            signal[i]   ^= linsin[i-1][1];
            signal[i+4] ^= linsin[i-1][0];
          end
        end
      end
    end

    return signal;
  endfunction

endclass
