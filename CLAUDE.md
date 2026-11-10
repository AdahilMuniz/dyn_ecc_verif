# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

A SystemVerilog/UVM 1.2 verification environment (Synopsys VCS + Verdi) for the NASCERR "man-in-the-middle" ECC FPGA. The FPGA sits between an STM32 (FMC bus, async-SRAM mode) and an external SRAM, and encodes the MCU's data with a selectable ECC.

**Target: the 8→16-bit ASIC** (8-bit data → 16-bit codeword, **one** 2M×16 SRAM). It follows the architecture of the original 8→16 version and tries to reuse resources from the more developed 16→32 version.

**Repository layout:** three version folders, plus the verification environment at the root. Each folder has a README.
- `dyn_ecc_8to16/rtl/` is **the DUT**: the ASIC RTL. It's a clean rewrite combining the verified 2024-08 codecs (proven bit-exact by `make equiv`) with the structure of `dyn_ecc_8to16/reference/NASCERR-develop` (package, control module, tri-states only in the top, Verilator lint).
- `nascerr_8to16_legacy/`: the original 8→16 version (`fpga/`, `firmware/`, `archives/`). Its `firmware/Projeto_NASCERR_UFC-image_processing` holds the C encoders used as the golden model.
- `nascerr_16to32/`: the most developed 16→32 version.
  - `fpga/FPGA_design` (git submodule, branch `refactor`): its RTL.
  - `firmware/Projeto_NASCERR_UFC-dev_firmware_v3`: the latest firmware. Its C TBEC/MRSC decoders are bit-exact with the submodule RTL.
  - `history/`: older 16→32 artifacts.

  None of it is compiled. Initialize the submodule with `git submodule update --init`.

**Decided scope:** see `docs/system_analysis.md` §7 (D1–D14, 2026-11-10). In short: 2-bit select, one SRAM, unified error reporting, power-efficient, no saboteur, reset deferred, zero added latency, correction target option (c) for now. Treat `nascerr_8to16_legacy/` and `nascerr_16to32/` as read-only reference material: don't delete, reorganize or "fix" files there as a side effect of other work. Changes for the ASIC go in `dyn_ecc_8to16/` and the verification environment.

Background: `docs/system_analysis.md` covers how the versions relate and the open decisions. The folder READMEs record the provenance of every artifact.

Zips, PDFs, Libero databases, etc. are in Git LFS, and STM32CubeIDE `Debug/` output is gitignored.

The recovered archives are RAR5. Extract them with `python3 -I scripts/rar_extract.py ARCHIVE OUTDIR`, which uses the system libarchive. The system 7-Zip lacks RAR support, and the installed `unrar` is unrar-free 0.1.1, which silently writes garbage for RAR5.

## Commands

All commands run from the repo root (the filelists use repo-root-relative paths).

```sh
make compile                    # vcs compile/elaborate -> ./simv (includes the DPI-C golden model)
make regress                    # compile once, run every sequence; prints UVM_ERROR/FATAL per sequence (logs: sim_<seq>.log)
make run SEQ_NAME=ecc_pattern_seq                              # one sequence (log: sim.log)
make run SEQ_NAME=ecc_single_err_seq PLUSARGS=+ECC_MODE=TBEC   # one sequence, one ECC mode
make run VERBOSITY=UVM_HIGH     # per-access driver/monitor messages are UVM_HIGH
make run DEBUG=1                # adds -debug_access+all -kdb +define+DUMP_FSDB -> dyn_ecc_tb.fsdb
make verdi                      # open dyn_ecc_tb.fsdb in Verdi with sources loaded
make lint                       # Verilator -Wall lint of dyn_ecc_8to16/rtl (must stay clean)
make equiv                      # exhaustive codec equivalence vs. the recovered RTL + firmware C (prints EQUIV PASS/FAIL)
make clean
```

- "Running a single test" means choosing `SEQ_NAME`. The default `dyn_ecc_base_seq` is empty, so a plain `make run` checks nothing.
- `DEBUG=1` changes compile options, so set it for both compile and run.
- Sequences: `write_read_seq`, `ecc_pattern_seq` (firmware block experiment), `ecc_single_err_seq` (exhaustive single-bit correction), and `ecc_burst_err_seq` (the test firmware's n-adjacent-bit bursts; prints a correct-word count per mode and length, like the firmware does).
- Sequence plusargs:
  - `+ECC_MODE=BYPASS|HAMMING|TBEC|EDAC|ALL`
  - `+N_WORDS=<n>` (`ecc_pattern_seq`, `ecc_burst_err_seq`)
  - `+BURST=<n>` and `+BURST_POS=<p>` (`ecc_burst_err_seq`). `+BURST_POS=0` reproduces the Oct 2024 FPGA saboteur's cw[2:0] patterns.
- The scoreboard prints a per-mode summary table in `report_phase`. Pass/fail is the UVM_ERROR count.
- **After any change to a codec in `dyn_ecc_8to16/rtl`, run `make equiv`.** A deliberate behavior change breaks equivalence by design; say so explicitly, and change the reference only with the user's agreement.

## Architecture

**Compile order** is defined entirely by `script/verif_filelist.f` (which includes `script/design_filelist.f`): DUT → SRAM model → interfaces (`sram_if`, `sideband_if`, `sram_backdoor_if`) → `sram_pkg` → `dyn_ecc_test_pkg` → `tb/dyn_ecc_tb.sv`.
- The RTL is one module per file with no `` `include ``s; `design_filelist.f` lists every file, package first. `make lint` uses the same list.
- RTL structure:

  | File | Role |
  |---|---|
  | `dyn_ecc_pkg.sv` | `ecc_sel_t` |
  | `dyn_ecc_{hamming,tbec,edac}_{enc,dec}.sv` | Codecs: 8-bit data in/out, 16-bit codeword |
  | `dyn_ecc_ctrl.sv` | Combinational mode mux + bus output enables, no tri-states |
  | `dyn_ecc_top.sv` | The only tri-states, plus the `erro` register |

- Lint waivers are inline `verilator lint_off` pragmas, each with its reason.

**Packages use `` `include `` bundling**, not separate compilation units:
- `vips/sram/sram_pkg.sv` includes the SRAM VIP classes (item, cfg, sequencer typedef, driver, monitor, agent).
- `tests/dyn_ecc_test_pkg.sv` includes `env/dyn_ecc_types.sv`, every sequence (`seqs/`), the scoreboard, the env and the test. A new sequence/test file must be added as an `` `include `` there, not to the filelist.

**Golden model via DPI-C:**
- The scoreboard's encoders are the 8-bit firmware's own `nascerr_8to16_legacy/firmware/Projeto_NASCERR_UFC-image_processing/Firmware_CubeSat/NASCERR_slave/Core/Src/NASCERR_ECCs.c`, compiled **unmodified** (the `DPI_SRC` line in the Makefile).
- `dpi/NASCERR_ECCs.h` stands in for the firmware header, which drags in the STM32 HAL.
- `env/dyn_ecc_types.sv` holds the DPI imports, `ecc_mode_e` and the virtual-interface typedefs.

**Sequence selection via factory override:** `dyn_ecc_base_test` reads `+SEQ_NAME=<name>`, overrides `dyn_ecc_base_seq` by name, and starts it on `env.mcu_agt.seqr`. New sequences must extend `dyn_ecc_base_seq`. It provides:
- `set_mode`, `wr`, `rd`: MCU accesses and mode changes
- `flip`: backdoor bit-flip, standing in for the FPGA saboteur
- `get_modes`: parses `+ECC_MODE`

Its `pre_body` fetches the `mux_if` and `sram_bd` virtual interfaces.

**Testbench top (`tb/dyn_ecc_tb.sv`)** is wired like the board top `nascerr_8to16_legacy/fpga/2024-08_fpga_1memory_8to16/component/work/stm32/stm32.v`:
- the MCU agent's `mcu_if` drives the DUT data bus
- the `IS61WV2M16AL` SRAM model (`tb/models/`, an asynchronous model with real timing) gets the address and control signals **straight from the MCU**, not through the DUT
- `sram0_if` is a passive tap on the FPGA↔SRAM data bus
- `mux_if` connects `wsig` → `ecc_sel[1:0]` and `erro` → `rsig[0]`
- `sram_bd` is the backdoor peek/poke/flip into `u_sram`

**SRAM VIP signals are logical active high** (`cs=1` selects, `we=1` writes). The tb top converts them to the active-low pins (NE→`cs_n`/CE_n, NWE→`we_n`/WE_n, NOE→`oe_n`/OE_n); don't invert inside the VIP.

**Driver and monitor timing:**
- The driver holds each access for `sram_agent_cfg.access_cycles` clocks (default 4 = 40 ns). The SRAM model needs ≥10 ns of stable address to write or return read data.
- Every access is followed by one idle clock.
- The monitor publishes one item per access, on that idle clock, carrying the last-clock samples. Back-to-back accesses with no idle clock would merge.

**Scoreboard (`env/dyn_ecc_scoreboard.sv`)** reads the current mode from `mux_if.wsig` and the stored word through the backdoor. Checks:

| Check | What it verifies |
|---|---|
| ENC | After a write, the SRAM holds the firmware encoder's codeword |
| BYP | A bypass read returns the stored word |
| DEC | An ECC read with ≤1 bit error (vs. the written codeword) returns `{8'h00, data[7:0]}` |
| BUS | At the end of the test, the FPGA→SRAM bus carried the expected codewords |

- ECC reads with ≥2 bit errors, and reads in a mode other than the one written, are only counted.
- `set_mode` changes `ecc_sel` between accesses, with a clock of margin each side, so every access sees a stable mode.

## DUT behavior to keep in mind (8→16)

- `ecc_sel[1:0]`: `00` bypass (all 16 bits pass through), `01` Hamming (8→12, `cw[15:12]=0`), `10` TBEC-RSC, `11` EDAC. Encoders use `data[7:0]`; decoders return `{8'h00, data}`.
- `cs_n` (NE), `we_n` (NWE) and `oe_n` (NOE) are **active low**. Write = `cs_n=0, we_n=0`; read = `cs_n=0, we_n=1, oe_n=0`.
- The datapath is combinational. `clk` only clocks `erro`, which is set when the Hamming decoder sees a nonzero syndrome on the memory bus (in any mode) and cleared while `we_n` is low. It has no reset (X until the first write). This is the 2024-08 behavior, kept until the unified error report (D4/D11).
- **Differs from the 2024-08 FPGA:** the MCU data bus is driven only during a read, not whenever no write is in progress (D6). This needed the new `oe_n` input.
- EDAC is systematic in the upper byte: `cw = {data, T(data)}`.
- The 2-bit select, the one-SRAM layout and the `erro` pin differ from the 16→32 submodule design (3-bit select, two SRAMs, 3 flag pins). Don't mix their conventions.
