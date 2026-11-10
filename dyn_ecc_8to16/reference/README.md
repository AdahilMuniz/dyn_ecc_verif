# Reference: other 8→16 RTL efforts

Read-only snapshots of parallel attempts at the 8→16 RTL. They're kept for what they do
well; the ASIC RTL is `../rtl/`.

## `NASCERR-develop/` (snapshot of 2026-11-04)

A clean SystemVerilog rewrite of an 8→16 TBEC-RSC design, from `NASCERR-develop.zip`
(apparently the `develop` branch of a `NASCERR` repo). It's stored unmodified, except that
its `misc/` folder was dropped: `run.sh`, `dump.vcd` and `stm_test_mem.rar` were
byte-identical to `nascerr_8to16_legacy/fpga/2024-07_tbec8_prototype`, and the rest was
build output.

**Adopted into `../rtl/`** (its good parts):
- a types package for the mode select
- a separate combinational control module with explicit output enables
- tri-states only in the top
- Verilator `-Wall` lint (`make lint`)

**Not adopted** (checked in VCS on 2026-11-10, all 256 data values):
- **The TBEC decoder doesn't correct single-bit errors.** With its own codewords, a flip in
  any of codeword bits 0–9 decodes wrong on 128/256 words. Error-free words decode
  correctly. Its own comment flags the correction-index mapping as the likely bug location
  ("Como a ordem dos vetores mudaram, aqui é o lugar mais provável de ocorrer bugs").
- **The TBEC encoder swaps check bits 13 and 14** compared with the 8-bit firmware C and the
  2024-08 FPGA (128/256 codewords differ). That's inherited from the July 2024 prototype.
- **Architecture conflicts with the decisions:**
  - two 8-bit memories (`ram_up`/`ram_down`) vs. one SRAM (D3)
  - a `BYPASS_UP/BYPASS_DOWN/TBEC_RSC/MRSC` select vs. bypass/Hamming/TBEC/EDAC (D2, D9)
  - MRSC is a TODO; there's no Hamming or EDAC
- **Its own flow fails:**
  - `just lint`: Verilator `UNOPTFLAT` on the bidirectional `cpu_data`
  - `just test`: `tb/memory_tb.sv` is the 2024 testbench and connects to ports that no longer
    exist
