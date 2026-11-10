# NASCERR 16→32: the most developed version (reference of recency)

The evolution of the original 8→16 idea: the FPGA encodes each 16-bit MCU word into a
32-bit codeword (TBEC or MRSC) and stores **16 bits in each of two SRAMs**. This version has
had the most development: flight-style firmware, C reference decoders, on-board
self-checks, telemetry and test campaigns. It's the reference for **resources to reuse** in
the 8→16 ASIC (`../dyn_ecc_8to16/`), which keeps the 8→16 architecture.

Kept for reference only; nothing here is compiled by the testbench.

## `fpga/FPGA_design`: RTL (git submodule)

- Fork of `AdahilMuniz/FPGA_design`. Libero project `FPGA2memory` for a SmartFusion2
  M2S025, top `component/work/prj_2_memory_sb/prj_2_memory_sb.v` → `hdl/design.sv` →
  `hdl/ecc_design.sv`.
- Pinned to branch `refactor` (`614aed9`). That's upstream `main` (`cfbdb5d`, 2025-05-19)
  plus one commit: `rst_n` clears `flag_out`, and the `` `include ``s are removed. The
  codecs are identical on both branches. `FPGA_design-main.zip` (2026-11-10) was
  byte-identical to `main`, so it isn't stored.
- **Select map** `ecc_sel[2:0]`: `000` bypass memory 1 (up, `SRAM_CS2`), `001` bypass
  memory 2 (down, `SRAM_CS1`), `010` TBEC-RSC, `011` MRSC, other values disabled (both chip
  selects off). It matches the v3 firmware exactly.
- The `rst_n` port exists only on `refactor`. The SmartDesign top was generated from `main`
  and never regenerated.
- After cloning: `git submodule update --init`.

## `firmware/Projeto_NASCERR_UFC-dev_firmware_v3`: latest firmware

The STM32L4S5 payload firmware (`Firmware_CubeSat/dev_nascerr_firmware`, FreeRTOS, I2C
register protocol to the satellite's on-board computer, OTA bootloader, external NAND
flash database), plus PC tools (`Software_NASCERR`, `Software_SERIAL`) and board documents.
The serial/test-campaign logs from `Software_SERIAL/img_uploader/` (18 files, 171 MB, including
`logs_campanhas_ieav/`) are **not in git**. They're in the original `Projeto_NASCERR_UFC-dev_firmware_v3.zip`,
and gitignored on the machine that did the import. The
firmware has its own `CLAUDE.md`; it's informative only and doesn't apply to this repo.

ECC testing (`Modules/NASCERR/Experiments/`, `Sram/`):
- `nascerr_select_ecc` drives `ECC_SEL_0..2` = PA4/PA5/PA8 with the map above.
- **C decoders** `nascerr_eccs_tbec_decoder` / `nascerr_eccs_mrsc_decoder` return raw data,
  corrected data and an `uncorrectable` flag.
- `nascerr_ecc_avaliation` reads each word three ways: both memories in bypass (the raw
  codeword), decoded by the C model, and decoded by the FPGA. A disagreement between the
  FPGA and the C model reports `ECC_ERR_UNKNOWN`. It also counts 1/2/≥3-bit errors and the
  error region.
- `nascerr_ecc_check_encoders_decoders` is a periodic self-test of the FPGA codecs against
  the C ones.
- There's no deliberate SRAM error injection (`ERROR_ENABLE 0`); `flash_saboteur` targets
  the external flash, not the ECC path.

**Checked against the RTL (VCS + DPI-C, 2026-11-10):** the C encoders and decoders are
**bit-exact** with `fpga/FPGA_design` (0 mismatches over 65,536 encodes and about 5.3M
decodes: clean, every 1-bit error, and random/adjacent 2- and 3-bit errors):

| | TBEC | MRSC |
|---|---|---|
| Any 1-bit error | 100% corrected | 100% corrected |
| 2-/3-bit adjacent bursts | 100% / 100% | 100% / 96.7% |
| 2-bit random | 56% corrected | 56% corrected |
| Wrong data **not** flagged `uncorrectable` (2-bit / 3-bit random) | 19% / 69% | 0% / 2.9% |

MRSC's flag (recompute the syndromes after correcting) is the model to follow for the
ASIC's unified error report (D4/D11). TBEC's rule-based flag isn't.

Firmware bugs found in v3:
- `nascerr_select_ecc` returns before applying the "disable" levels for an invalid mode.
- `nascerr_ecc_encode` leaves `aux` uninitialized for non-TBEC/MRSC modes.
- The self-test loops `j < 255`, so value 255 is never tested.
- `nascerr_ecc_avaliation`'s 1/2/≥3-bit counts look at data bits only.

## `history/`: earlier 16→32 artifacts

| Folder | Date | What it is |
|---|---|---|
| `2024-04_rtl_initial_16to32/` | 2024-04-02 | First RTL of the project (EDA Playground): 16→32 TBEC encoder/decoder, codeword split across two SRAMs. `memory.v` is a behavioral **IS61WV2M16AL** 2M×16 async SRAM model (no license header; origin unknown, probably the vendor). The testbench uses a copy, `tb/models/IS61WV2M16AL.v`. From the Drive download `drive-download…zip` → `RTL (versão inicial)/result.zip` |
| `2024-10_fpga_2memory_16to32/` | 2024-10-15 | Earlier `FPGA2memory` Libero project (2-bit select) plus `stm_TEST_2_MEMORY/` STM32 test firmware. From the Drive download `Documents of project…zip` → `FPGA 2 MEMORY/*.zip` |
| `Projeto_NASCERR_UFC-dev_experiment32bits/` | 2024-10-31 → | 16→32 firmware snapshot before v3. It uses a 7-mode select (`TBEC_1_TO_3`, `TBEC_1_TO_7`, …) for an FPGA build that wasn't recovered; it **doesn't match** the submodule RTL. From `Projeto_NASCERR_UFC-dev_experiment32bits.zip` (the archive isn't stored in git) |
