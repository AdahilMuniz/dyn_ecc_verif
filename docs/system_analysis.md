# NASCERR system analysis: firmware vs. RTL

Cross-reference of the two NASCERR firmware snapshots (zips in the repo root) with the
`FPGA_design` submodule, done to pin down what the 8-to-16-bit RTL/ASIC target must do.

| Source | Data → codeword | ECC select | When |
|---|---|---|---|
| `nascerr_16to32/history/2024-04_rtl_initial_16to32` (first RTL, EDA Playground) | 16 → 32 | n/a | Apr 2024 |
| `nascerr_8to16_legacy/fpga/2024-07_tbec8_prototype` (superseded) | 8 → 16 | 1 bit | Jul 2024 |
| **`nascerr_8to16_legacy/fpga/2024-08_fpga_1memory_8to16`** (Libero project `stm32`) | **8 → 16, one SRAM** | `ecc_sel[1:0]` | Aug 2024 |
| `nascerr_8to16_legacy/firmware/Projeto_NASCERR_UFC-image_processing` (`NASCERR_slave`) | 8 → 16 | 2 GPIOs (PC0, PC1) | Sep 2024 |
| `nascerr_16to32/history/2024-10_fpga_2memory_16to32` | 16 → 32, two SRAMs | `ecc_sel[1:0]` | Oct 15 2024 |
| `nascerr_16to32/history/Projeto_NASCERR_UFC-dev_experiment32bits` (`dev_nascerr_firmware`) | 16 → 32 | 3 GPIOs (PA4, PA5, PA8) | Oct 31 2024 + |
| `FPGA_design` submodule (`refactor`) | 16 → 32, two SRAMs | `ecc_sel[2:0]` | Nov 2024 → |

**Decision (2026-11-10):** the RTL/ASIC target is the **8→16** version.

> **Update 2026-11-10 (recovered Drive artifacts, see `nascerr_8to16_legacy/README.md`).** The 8→16 FPGA
> RTL *does* exist: `nascerr_8to16_legacy/fpga/2024-08_fpga_1memory_8to16`. Its Hamming, TBEC and EDAC encoders
> are bit-identical to the 8-bit firmware's C encoders, so this is the FPGA that firmware ran
> against. It uses **one** SRAM, and it settles the select encoding (§3b). The TBEC decoder
> had a one-character bug, fixed in the testbench's DUT copy `dyn_ecc_8to16/rtl`. Sections
> 3b, 4, 5 and 7 below are corrected accordingly. Text that still describes the 16→32
> design is about the submodule.

---

## 1. System overview (common to both versions)

```
 STM32 (FMC, 16-bit async SRAM mode, bank 1 @ 0x6000_0000)
   │  A[20:0], NE1, NWE, NOE, NBL0/1 ───────────────────────────► straight to both SRAMs
   │  D[15:0] ◄──► FPGA (encoder / decoder / bypass mux) ◄──► SRAM "up"   D[15:0]
   │  ecc_sel GPIOs ──► FPGA                               ◄──► SRAM "down" D[15:0]
   │  saboteur GPIO ──► FPGA (fault injection, see §5)
   ◄── flag GPIOs  ◄── FPGA
```

The FPGA only intercepts **data** and **chip selects**. The address, WE, OE and byte
lanes go straight from the MCU to the SRAMs (see `prj_2_memory_sb.v`). The ECC datapath is
**combinational**. The clock (on-chip 25/50 MHz RC oscillator, asynchronous to the
MCU) only clocks the `flag_out` register.

Hardware (32-bit board BOM, `PCB_NASCERR_V2`): STM32L4S5QII6, SmartFusion2
M2S025-VFG256I, **2 × IS61WV204816BLL-10TLI (2M × 16 async SRAM)**, W25N01GV SPI NAND.
The 8-bit firmware targets a STM32L4R5ZI (Nucleo-144).

The firmware experiment is in effect the system-level test this UVM environment should copy:
1. Pick a mode and write a pattern (`0x00/0xFF/0x55/0xAA/random`), word by word or as a block.
2. (Optional) pulse the saboteur to inject errors.
3. Read back **through the ECC** (decoded data) and **in bypass** (raw codeword).
4. Raw errors = `sw_encode(written) XOR bypass_read`. "Corrected" = number of words where
   `decoded == written`. The name is misleading: it counts correct words, not corrections.

The firmware C encoders are the de facto golden model: the 32-bit firmware's
`nascerr_eccs_tbec_encoder` / `nascerr_eccs_mrsc_encoder` match the RTL encoders bit
for bit (firmware bit 31 = RTL index `[0]`).

---

## 2. The real FPGA top: `prj_2_memory_sb`, not `fpga_top_design`

`FPGA2memory/component/work/prj_2_memory_sb/prj_2_memory_sb.v` (the SmartDesign top that
the pin constraints in `constraint/io/user.pdc` refer to) wraps `fpga_top_design`:

| `fpga_top_design` port | Board net | Notes |
|---|---|---|
| `write_en` | `MCU_WE` (FMC NWE) | active low |
| `chip_sel` | `MCU_CS` (FMC NE1) | active low |
| `output_en` | `MCU_OE` (FMC NOE) | active low; only gates the flag latch |
| `ecc_sel` | `{ecc_sel2, ecc_sel1, ecc_sel0}` | `ecc_sel0` = LSB |
| `chip_sel_out[0]` / `[1]` | `SRAM_CS1` / `SRAM_CS2` | active low |
| `flag_out[0]/[1]/[2]` | `flag0`+`LED1` / `flag1`+`LED2` / `flag3` | port `flag2` doesn't exist; it's named `flag3` |
| `fpga_mem_io_up` / `_down` | `mcu_mem_io_up` / `_down` | |
| `clk` | `OSC_C0` RC oscillator | |
| `rst_n` | **not connected** | the SmartDesign is out of date: `rst_n` was added in submodule commit `614aed9` |

Inside `design.sv`, memory **up** is enabled by `chip_sel_out[1]`, which is **`SRAM_CS2`**,
and memory **down** by `chip_sel_out[0]`, which is **`SRAM_CS1`**. So "memória 1"/up is
physically CS2. The names are counter-intuitive, but the wiring is self-consistent.

---

## 3. ECC-select encoding: three versions that don't agree

### 3a. 32-bit firmware vs. current RTL (`ecc_sel0` = LSB, confirmed by the SmartDesign concat)

| `ecc_sel` | 32-bit firmware `NASCERR_MODE` | RTL `ecc_design.sv` |
|---|---|---|
| 000 | `BYPASS_1` | bypass memory up |
| 001 | `BYPASS_2` | bypass memory down |
| 010 | `TBEC` | TBEC-RSC |
| 011 | `TBEC_1_TO_3` (TBEC + 1–3 injected errors) | **MRSC** ✗ |
| 100 | `TBEC_1_TO_7` | default (error) ✗ |
| 101 | `BYPASS_1_TO_7` | default (error) ✗ |
| 110 | **`MRSC`** | default (error) ✗ |

The firmware was written against an FPGA build with **7 modes, including error injection**.
That build isn't in the submodule. The RTL in the submodule is a reduced 4-mode version, and
its MRSC code (011) clashes with the firmware's.

### 3b. 8-bit firmware (`NASCERR_slave/Core/Src/NASCERR_Memory.c`) vs. the 8→16 RTL

**Resolved by the recovered RTL.** `hamming_top_design.sv` decodes `ecc_sel[1:0]` as
`00` bypass, `01` Hamming, `10` TBEC, `11` EDAC, and the board top (`stm32.v`) wires
`ecc_sel = {ecc_sel1, ecc_sel0}`. **PC0 drives `ecc_sel0` (the LSB).** That's the only
reading consistent with the firmware's reference encoders. It's confirmed by the
standalone test firmware written against this FPGA (`nascerr_8to16_legacy/firmware/2024-08_stm_test_mem_fw`),
whose `Hamming()` sets PC0=1, PC1=0 and `TBEC()` sets PC0=0, PC1=1.

The firmware's "TBEC / TBEC_RANDOM / EDAC" select comments aren't wrong: they describe the
**October 2024 saboteur build** (`nascerr_8to16_legacy/fpga/2024-10_fpga_1memory_saboteur`), where the map is
`01` TBEC, `10` TBEC_random, `11` EDAC. The firmware's reference encoders match the
August build. So the 8-bit firmware snapshot sits between the two FPGA builds.

Original analysis (2 select pins):

| Mode | PC0 | PC1 | Reference encoder used to check it (`NASCERR_ECCs.c`, `NASCERR_Errors.c`) |
|---|---|---|---|
| `BYPASS_TO_SRAM` | 0 | 0 | none |
| `ECC_MOD1` | 1 | 0 | Hamming (8 → 12, top 4 bits zero) |
| `ECC_MOD2` | 0 | 1 | TBEC (8 → 16) |
| `ECC_MOD3` | 1 | 1 | EDAC (8 → 16) |

The firmware's own labels disagree with each other. Inline comments in `select_ecc` say
MOD1 = "TBEC", MOD2 = "TBEC_RANDOM", MOD3 = "EDAC". The function doc says 01 = Hamming,
02 = TBEC, 03 = EDAC, 04 = bypass. The encoder and error-injection code (which actually
computes the expected values) say **MOD1 = Hamming, MOD2 = TBEC, MOD3 = EDAC**. That's the
most reliable reading. Which PC pin is the LSB of the FPGA select can't be recovered: the
8-bit FPGA project is gone.

PC3 = saboteur trigger (output pulse). PC2 = input with pull-up, probably an FPGA
status/flag line, but nothing reads it.

---

## 4. The 8 → 16 codes (golden models from `NASCERR_ECCs.c`)

Data `W[7:0]`. All three are systematic (EDAC in the upper byte; see below).

- **TBEC (8 → 16):** `W` arranged as a 4 × 2 matrix, `s[r][c] = W[r + 4c]`.
  - `cw[7:0] = W`
  - `cw[9:8] = Di[1:0]` (diagonal parity)
  - `cw[11:10] = P[1:0]` (2×2 quadrant parity)
  - `cw[15:12] = Cb[1][1], Cb[1][0], Cb[0][1], Cb[0][0]` (per-column pair parity)

  It's the half-size counterpart of the RTL's 16→32 TBEC-RSC, which uses a 4×4 matrix and
  `Di[3:0]`, `P[3:0]`, `Cb[3:0][1:0]`.
- **EDAC (8 → 16):** `V[7:0]` = a linear XOR transform T of `W`, then `V[15:8]` = P(`V[7:0]`),
  with P(T(W)) = W. So the codeword is `{W, T(W)}`: **systematic in the upper byte** (checked
  exhaustively by `make equiv`; an earlier version of this doc said "not systematic"). See
  `nascerr_eccs_edac_encoder` for the exact equations.
- **Hamming (8 → 12):** classic positional Hamming(12,8): parity at positions 0, 1, 3, 7 and
  data at 2, 4–6, 8–11; `cw[15:12] = 0`.

The firmware has no decoders, but the **recovered 8→16 RTL does**
(`nascerr_8to16_legacy/fpga/2024-08_fpga_1memory_8to16/hdl`: `hamming_decoder`, `Decoder16bits` (TBEC),
`EDACdecoder`). Checked in VCS over all 256 values and every single-bit error:
- the encoders are identical to the C ones;
- Hamming and EDAC correct every single-bit error;
- TBEC failed 1,664/4,096 because of `signal[0] = data_in[2]` (should be `data_in[0]`).
  With that fixed, it corrects them all.

The TBEC decoder also produces a 3-bit region `flag`, but nothing reaches a pin; the only
error output is Hamming's `erro`. Fixed TBEC on multi-bit errors: 75% of 2-bit errors (any
two positions) and 64% of 3-bit bursts that are adjacent in codeword index are decoded
wrong. TBEC targets specific *physically* adjacent cell patterns, which weren't modeled.

**Codeword placement:** the whole 16-bit codeword goes into **one** x16 SRAM word
(`fpga_mem_io[15:0]`; the folder is literally "FPGA 1 MEMORY"). That's why a single
16-bit bypass read returns it. My earlier guess of a split across two SRAMs was wrong. The
two-SRAM layout only belongs to the 16→32 designs.

---

## 5. Other loose ends

1. **Saboteur / fault injection.** Both firmwares pulse a "start saboteur" pin (8-bit: PC3;
   32-bit: PA9) and the 32-bit firmware has `*_1_TO_3` / `*_1_TO_7` modes. **Found**, for
   the 8→16 design: `nascerr_8to16_legacy/fpga/2024-10_fpga_1memory_saboteur` (2024-10-18).
   - Mode `10` (TBEC_random) XORs a 3-bit pattern into codeword bits [2:0] on every write.
   - The pattern comes from a 2-bit LFSR clocked by the FPGA's RC oscillator and reset by
     the `turn_on_generator` pin, which the firmware pulses. It cycles 001 → 111 → 011,
     i.e. 1/3/2 bit errors.
   - Hamming was removed to free its select code.
   - The fixed TBEC decoder corrects the 1-bit pattern and never the 2- and 3-bit ones
     (0/256 each).
   - The 32-bit firmware's `TBEC_1_TO_3` / `TBEC_1_TO_7` names fit a widened version of the
     same idea (a 3-bit LFSR value?). No 16→32 saboteur RTL was recovered.

   The testbench injects the same patterns through the SRAM backdoor
   (`ecc_burst_err_seq +BURST_POS=0`) and the firmware's random-position bursts
   (`ecc_burst_err_seq`).
2. **Flag pins.** The RTL drives 3 flag outputs (TBEC decoder only; MRSC produces no flag).
   The 32-bit firmware never reads them, and commit `d81f6f2` says they go to MCU pins C1, G8,
   A8. But **PA8 is `ECC_SEL_2` (an output)** in the 32-bit firmware, which contradicts that.
   Check against the pin spreadsheet in the `FPGA_design` README.
3. **MCU data bus contention.** `design.sv` drives `mcu_fpga_io` whenever it's *not* in a
   write, including when the MCU is idle (CS high). It doesn't use `output_en`/NOE for this.
4. **Idle chip select in bypass.** In modes 000/001, `chip_sel_out = {cs, !cs}` / `{!cs, cs}`.
   With the MCU idle (`cs = 1`), this *asserts* the other SRAM's CS (low).
5. **Default mode** de-asserts both SRAM CSs but still routes `data_in_right_up` to the MCU.
6. **Bit-ordering conventions.** The RTL uses ascending ranges (`[0:15]`, so index 0 is the MSB).
   The firmware uses bit 31 = RTL index 0. Pick one convention for the 8→16 rewrite.
7. **8-bit firmware address range:** experiments are limited to 1024 words at offset 0.

---

## 6. What this means for the UVM environment (`dyn_ecc_verif`)

> **Done (2026-11-10).** The environment now targets `dyn_ecc_8to16/rtl`: 8-bit data, one
> IS61WV2M16AL SRAM model, FMC-style active-low controls with the address going straight to
> the SRAM, the firmware's `NASCERR_ECCs.c` as the DPI-C golden model, and backdoor error
> injection. `make regress` passes all sequences, including 3 × 4,096 single-bit-error
> corrections. See `CLAUDE.md` for the structure. The original plan is kept below.

- MCU-side data is **8 bits** for the target: parameterize the SRAM VIP/sequence items
  accordingly. The memory side is 16-bit codeword halves, or bytes if the split in §4 is
  confirmed.
- Model the MCU bus as the FMC asynchronous SRAM interface it really is: NE/NWE/NOE
  active low, plus an address that **goes around the DUT straight to the memory models**.
  Today `mcu_if.addr` goes nowhere, and no memory models respond on `sram_if0/1`, so reads
  return Z/X.
- Current TB issues (from `CLAUDE.md`): the driver drives `we`/`cs` active high into
  active-low DUT pins, and `sram_if0` ("up") is tied to `chip_sel_out[0]`, but up is
  `chip_sel_out[1]` (`SRAM_CS2`).
- Scoreboard: copy the firmware experiment. The reference encoders are the C functions in
  `NASCERR_ECCs.c`, which could be reused as-is through DPI-C. Check both the bypass raw
  codeword and the decoded data.
- Fault injection: flip bits in the memory models (in the testbench) to stand in for the
  missing saboteur, with error classes matching the firmware (`*_1_TO_3`, `*_1_TO_7`).

## 7. Decisions for the 8 → 16 RTL/ASIC

### Decided (2026-11-10)

| # | Topic | Decision | What it implies for the current RTL / testbench |
|---|---|---|---|
| D1 | Purpose | The ASIC is an **invisible man-in-the-middle** between the MCU and a **fragile (error-prone) SRAM**: it ECC-encodes MCU writes and decodes/corrects reads, transparently to the firmware. | Bypass must stay a pure pass-through; ECC modes must look like plain 8-bit memory to the MCU (decoded read = written data). |
| D2 | Mode select | **2 bits for now**: `00` bypass, `01` Hamming, `10` TBEC, `11` EDAC (the 2024-08 map). | Matches `dyn_ecc_8to16/rtl` and `ecc_mode_e`. No change. |
| D3 | Memories | **8→16 uses one SRAM.** The 16→32 design uses two, 16 bits of the codeword in each. | Matches the recovered 8→16 RTL and the testbench (one `IS61WV2M16AL`). |
| D4 | Error reporting | **Unified**: one error-reporting scheme that behaves the same in every ECC mode. | Today only Hamming drives `erro`. The TBEC region `flag` is computed but goes nowhere (`Decoder16bits` `sig` is an undeclared net), and EDAC reports nothing. The decoders must expose a common status, and the scoreboard must check it. *Open details:* what is reported (detected / corrected / uncorrectable?), per access or sticky, and the pin(s). |
| D5 | Reset | **Deferred**, defined later if needed. | `erro` stays without reset. The testbench keeps ignoring its value until the first write. |
| D6 | Power | **As power-efficient as possible.** | Candidates to review: all three encoders and decoders evaluate in parallel on every bus toggle (isolate or gate the codecs that aren't selected); the FPGA drives the MCU data bus whenever it isn't writing, including when idle (gate on NE/NOE). A power target and how it's measured are still to be defined. **Done 2026-11-10:** the MCU bus is now driven only during reads (`dyn_ecc_ctrl`, new `oe_n` input). |
| D7 | Saboteur | **Not in the ASIC.** Error injection stays testbench-only (SRAM backdoor). | The TBEC_random build stays in `nascerr_8to16_legacy/fpga/` for reference only. |
| D8 | Repository | This repo is a **merge of all project artifacts** (several implementations of the same idea). **Organized 2026-11-10** into `nascerr_8to16_legacy/` (original), `nascerr_16to32/` (most developed) and `dyn_ecc_8to16/` (ASIC RTL), with the verification environment at the root. | The two reference folders are read-only history. ASIC work happens in `dyn_ecc_8to16/` and the verification environment. |
| D9 | Codes | **Probably all three** (Hamming, TBEC, EDAC), unless Hamming is dropped. | Hamming is really 8→12 (`cw[15:12]=0`) and corrects 1 bit only, so it's the weakest candidate if one goes. |
| D10 | Correction target | Asked for: correct **1 to 3 bit errors, adjacent or spaced**, which isn't achievable in full in 16 bits (see below). **Chosen for now (2026-11-10): option (c)**: keep the current codes, accept partial multi-bit coverage, and report what they can't correct as uncorrectable (D11). Options (a) and (b) are **parked as possible pivots**. | No code change now. A pattern counts as "reported" only if the decoder can *detect* it; a pattern it silently miscorrects can't be reported, so D11's "uncorrectable" category only covers what each code can detect (to be measured per code). |
| D11 | Error categories | Report **all** of: no error / corrected / detected-uncorrectable, if possible. | Each decoder needs a detection path, not just correction. Today only Hamming has one (`erro`, any nonzero syndrome). |
| D12 | Latency | The interface **shall not add delay**: seamless to the MCU. | No extra bus cycles or wait states, so the datapath stays combinational (as now). Encode, and decode + correct, must fit inside the FMC access timing with the SRAM's access time. More powerful codes mean deeper decoders, so D10 and D12 pull against each other. |
| D13 | Power baseline | **No baseline yet.** | Establish one: switching activity (SAIF/FSDB) from the regression on the current RTL, before any power optimization. |
| D14 | Architecture | The ASIC is an **8→16** design following the original 8→16 architecture, **reusing resources from the 16→32 version** where possible (it's the most developed). | Candidates to reuse: the C-decoder-with-`uncorrectable` pattern and the MRSC post-correction recheck (for D4/D11), the hardware-vs-C read check and the encoder/decoder self-test (as verification ideas), and the error statistics/telemetry. See `nascerr_16to32/README.md`. |

#### Feasibility of D10 (8 data bits, 16-bit codeword)

By the Hamming (sphere-packing) bound, correcting every error pattern of weight ≤ *t*
needs 2^8 × Σ_{i≤t} C(16,i) ≤ 2^16:

| Correct any pattern of… | Needed | Available | Feasible? |
|---|---|---|---|
| ≤ 1 bit | 256 × 17 = 4,352 | 65,536 | yes (all three current codes do) |
| ≤ 2 bits, anywhere | 256 × 137 = 35,072 | 65,536 | yes in principle. Best known: Nordstrom–Robinson (16, 256, d=6, nonlinear) corrects any 2 and detects any 3. Linear [16,8] codes reach at most d=5 |
| ≤ 3 bits, anywhere | 256 × 697 = 178,432 | 65,536 | **no, no code of any kind can** |
| bursts of ≤ 3 *adjacent* bits | 256 × 46 = 11,776 | 65,536 | yes in principle (burst-correcting codes; TBEC's goal) |

Measured today (`ecc_burst_err_seq`, random position): TBEC corrects 58.8% of 2-bit and
37.5% of 3-bit adjacent bursts; Hamming and EDAC are designed for single errors. Restatements of D10:
- (a) *parked*: correct any ≤2 and detect any 3 (e.g. Nordstrom–Robinson);
- (b) *parked*: correct ≤3 *adjacent* (plus any 1), and detect the rest where possible;
- (c) **chosen for now**: keep the current codes in 16 bits, accept partial multi-bit
  coverage, and report the cases they can't correct as uncorrectable where they're
  detectable (D11).

If the project pivots to (a) or (b), the encoder/decoder pair and the DPI golden model
change, but the select map, the one-SRAM layout, the error interface (D11) and the
testbench structure don't.

### Still open

1. **Per-code detection capability under (c)**: for 2- and 3-bit errors, what fraction each
   code corrects, detects (reportable as uncorrectable) or silently miscorrects. This
   sets what D11 can promise, and whether Hamming stays (D9).
2. **Decoder detection logic**: each decoder needs a detected/corrected output; today only
   Hamming has a syndrome flag.
3. **D11 details**: when the status is valid and cleared (per access or sticky), and the pins.
4. **D12 timing budget**: FMC setup/hold/wait-state settings, the SRAM access time, and the
   resulting combinational delay limit for encode and decode.
5. **D13**: produce the power baseline.
