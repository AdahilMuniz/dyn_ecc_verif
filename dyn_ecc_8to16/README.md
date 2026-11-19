# dyn_ecc 8→16: the ASIC RTL

The RTL headed for the ASIC: a transparent ECC man-in-the-middle between the STM32 (FMC
asynchronous-SRAM bus) and **one** fragile 2M×16 SRAM. It encodes each 8-bit MCU word into
a 16-bit codeword. It follows the **8→16 architecture** of the original design
(`../nascerr_8to16_legacy/`) and aims to reuse the resources of the more developed 16→32
version (`../nascerr_16to32/`).

## `rtl/`: the best of each version

| Taken from | What |
|---|---|
| `../nascerr_8to16_legacy/fpga/2024-08_fpga_1memory_8to16` (verified FPGA) | The Hamming / TBEC / EDAC algorithms, the 2-bit select map, one SRAM, the `erro` behavior. TBEC decoder typo fixed (`signal[0] = data_in[0]`, as in the 2024-10 build) |
| `../NASCERR` submodule (`develop`, 2026 rewrite) | Code structure: types package, separate control module with explicit output enables, tri-states only in the top, Verilator lint |
| Decisions (`../docs/system_analysis.md` §7) | MCU bus driven only during reads (D6), via a new `oe_n` input; combinational datapath (D12) |

| File | Role |
|---|---|
| `dyn_ecc_pkg.sv` | `ecc_sel_t`: `00` bypass / `01` Hamming / `10` TBEC / `11` EDAC |
| `dyn_ecc_hamming_enc.sv`, `dyn_ecc_hamming_dec.sv` | Hamming(12,8), `cw[15:12] = 0`. The decoder also outputs its syndrome |
| `dyn_ecc_tbec_enc.sv`, `dyn_ecc_tbec_dec.sv` | TBEC-RSC 8→16: data in `cw[7:0]`; Di, P, Cb check bits in `cw[15:8]` |
| `dyn_ecc_edac_enc.sv`, `dyn_ecc_edac_dec.sv` | EDAC 8→16: `cw = {data, T(data)}` |
| `dyn_ecc_ctrl.sv` | Combinational mode mux and bus output enables (write: memory bus; read: MCU bus). No tri-states |
| `dyn_ecc_top.sv` | Pins (`mcu_data`, `mem_data`, `cs_n`, `we_n`, `oe_n`, `ecc_sel`, `clk`, `erro`), the two tri-state buses, the `erro` register |

The FPGA-era RTL this replaces (`fpga_top_design.sv` + `` `include ``d codecs) is in git
history and, unfixed, in `nascerr_8to16_legacy/`.

## Defects of the original 8→16 RTL and how they're handled

Reviewed against `../nascerr_8to16_legacy/fpga/2024-08_fpga_1memory_8to16/hdl` (the
verified FPGA). Codec fixes keep the codewords bit-exact with the firmware (`equiv/`);
"kept" items need a decision first.

| # | Original (2024-08) | Effect | In `rtl/` |
|---|---|---|---|
| 1 | `Decoder16bits.sv`: `signal[0] = data_in[2]` (typo for `data_in[0]`) | TBEC decodes data bit 0 from bit 2; wrong output whenever they differ | **Fixed** (`s = cw[7:0]`), same as the 2024-10 build |
| 2 | `Decoder16bits.sv`: `assign sig = flag;` with `sig` undeclared | The TBEC error flag is an implicit net going nowhere | **Dropped**; a real status output is D4/D11 |
| 3 | `hamming_decoder.sv`: `$display` inside `always_comb` | Prints on every evaluation; not synthesizable intent | **Removed** |
| 4 | `hamming_decoder.sv`: `received_data[verify_bit-1]` with syndrome 13–15 on a 12-bit vector | Out-of-range write: ignored in simulation, tool-dependent in synthesis | **Made explicit**: flip only for syndrome 1–12 (same simulated behavior) |
| 5 | `EDACdecoder.sv`: `SV1[15:0] != 4'h0000` | Width mismatch (4-bit literal vs. 16-bit vector); works only because the value is 0 | **Fixed** (`16'd0`) |
| 6 | `EDACdecoder.sv`: the `sumP1 < sumP2` branch corrects `V[7:0]` | Dead code: only `V[15:8]` reaches the output. Harmless: in that case `V[15:8]` already equals the data byte | **Omitted** |
| 7 | `fpga_top_design.sv`: MCU bus driven whenever `!(write && cs)`, including `cs_n=1` and idle | Contention with other FMC devices on the shared bus (the firmware also has `BYPASS_TO_FLASH`), and wasted power | **Fixed** (D6): driven only on a read (`cs_n=0, we_n=1, oe_n=0`), new `oe_n` pin |
| 8 | `fpga_top_design.sv`: `decoder_input = write ? 16'bz : fpga_mem_io` | Assigns Z to an internal net (meaningless after synthesis) | **Removed**; tri-states only on the pins in `dyn_ecc_top` |
| 9 | `hamming_top_design.sv`: `erro` register written with blocking `=` in `always_ff` | Simulation/synthesis mismatch risk | **Fixed** (`<=`) |
| 10 | `erro` set by any nonzero Hamming syndrome on the memory bus, **in every mode** and also with the bus idle (floating) | In TBEC/EDAC/bypass the Hamming syndrome of the stored word is meaningless, so `erro` is set spuriously; with the bus idle it samples X | **Fixed 2026-11-19**: set only on a read in Hamming mode |
| 11 | `erro` cleared whenever NWE is low | A write to another FMC device clears it | **Fixed 2026-11-19**: cleared only by a write to this SRAM (`cs_n=0`) |
| 12 | `erro` has no reset | X until the first write | **Kept** (D5, deferred) |
| 13 | Only Hamming reports errors; TBEC and EDAC report nothing | No uniform error reporting | **Kept** until D4/D11 |
| 14 | `` `include `` chain (`fpga_top_design` → `hamming_top_design` → codecs), 16-bit codec ports using only 8 bits | Fragile compile order, lint noise | **Restructured**: one module per file, 8-bit data ports, package for the select |

**Not a defect: TBEC's partial burst coverage.** The 8→16 TBEC corrects every 1-bit error,
but only 9 of the 15 2-bit adjacent bursts (and 5 of 14 3-bit ones). The code itself can't
do better, because different bursts share a syndrome: errors on data bits (0,1) and (2,3)
both give `sx=1100, sp=00, sd=11`. Doing better takes a different code (D10, options a/b),
not a decoder fix.

## `equiv/`

`codec_equiv_tb.sv`, run by `equiv/run_equiv.sh` (needs VCS; works from any directory,
build output in `equiv/build/`). It proves the rewritten codecs
**bit-exact** with the recovered FPGA codecs and the 8-bit firmware's C encoders:
all 256 encoder inputs and all 65,536 decoder inputs. Status 2026-11-10: `EQUIV PASS`, 0
mismatches. A planted one-bit change in the TBEC decoder gave 14,336 mismatches.
`NASCERR_ECCs.h` is a host-side stand-in for the firmware header (which pulls in the STM32
HAL), so the firmware's `NASCERR_ECCs.c` compiles unmodified.

## Why not the `NASCERR` submodule's TBEC as-is

Checked in VCS on 2026-11-10 (snapshot equal to `NASCERR` `cc8933a`), all 256 data values:
- **The TBEC decoder doesn't correct single-bit errors.** With its own codewords, a flip in
  any of codeword bits 0–9 decodes wrong on 128/256 words. Its own comment flags the
  correction-index mapping as the likely bug location.
- **The TBEC encoder swaps check bits 13 and 14** compared with the 8-bit firmware C and the
  2024-08 FPGA (128/256 codewords differ), inherited from the July 2024 prototype.
- Architecture differs from the decisions: two 8-bit memories vs. one SRAM (D3); a
  `BYPASS_UP/BYPASS_DOWN/TBEC_RSC/MRSC` select vs. bypass/Hamming/TBEC/EDAC (D2, D9).

Not re-checked against `93071b6` ("Width fixes"), which changes only `top.sv` port
declarations.

## Status

- `lint.sh`: clean (Verilator `-Wall`; waivers inline, with reasons).
- `equiv/run_equiv.sh`: PASS.
- Not integrated into the UVM environment at the repo root (which targets `NASCERR`).
- Not ASIC-ready yet:
  - no unified error status (D4/D11); only Hamming drives `erro` (Hamming-mode reads only)
  - no reset (D5)
  - no synthesis scripts or SDC timing constraints
  - power not reviewed beyond the bus gating (D6): all codecs still evaluate in parallel
