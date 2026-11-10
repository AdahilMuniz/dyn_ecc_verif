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
| `reference/NASCERR-develop` (2026 rewrite) | Code structure: types package, separate control module with explicit output enables, tri-states only in the top, Verilator lint |
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

## `equiv/`

`codec_equiv_tb.sv`, run by `make equiv` from the repo root. It proves the rewritten codecs
**bit-exact** with the recovered FPGA codecs and the 8-bit firmware's C encoders:
all 256 encoder inputs and all 65,536 decoder inputs. Status 2026-11-10: `EQUIV PASS`, 0
mismatches. A planted one-bit change in the TBEC decoder gave 14,336 mismatches.

## `reference/`

Other 8→16 RTL efforts, kept read-only. See `reference/README.md`.

## Status

- `make lint`: clean (Verilator `-Wall`; waivers inline, with reasons).
- `make equiv`: PASS.
- `make regress`: all sequences pass, with the same results as on the original FPGA RTL.
- Not ASIC-ready yet:
  - no unified error status (D4/D11); only Hamming drives `erro`, and it reacts in any mode
  - no reset (D5)
  - no synthesis scripts or SDC timing constraints
  - power not reviewed beyond the bus gating (D6): all codecs still evaluate in parallel
