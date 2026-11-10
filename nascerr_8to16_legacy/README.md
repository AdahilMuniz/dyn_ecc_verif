# NASCERR 8→16: the original version (legacy reference)

The original form of the idea: an FPGA between the STM32 and **one** external SRAM, which
encodes each 8-bit MCU word into a 16-bit codeword (Hamming / TBEC / EDAC, 2-bit select).
It was later turned into the 16→32, two-SRAM design in `../nascerr_16to32/`. This folder
holds the core idea that the ASIC follows. The ASIC RTL itself lives in `../dyn_ecc_8to16/rtl/`.

Everything here is kept **unmodified** as a historical record, recovered on 2026-11-10.

## `fpga/`: FPGA designs

| Folder | Date | What it is | Source |
|---|---|---|---|
| `2024-07_tbec8_prototype/` | 2024-07-02 | Early 8→16 TBEC prototype (EDA Playground), one SRAM. **Superseded**: its encoder swaps codeword bits 13/14 compared with the firmware, and its decoder fails to correct most single-bit errors | Drive download `drive-download…zip` → `TBEC_RSC 8bits/` (`a.out` dropped) |
| `2024-07_fpga_1memory_8to16_early/` | 2024-07-11 | First Libero version of the 8→16 FPGA. Same codecs and select map as 2024-08; only `fpga_top_design.sv` / `hamming_top_design.sv` differ (13/17 lines). `synthesis/synwork/modulechange.db` was the one file that failed to extract and was dropped | Drive download `Documents of project…zip` → `FPGA 1 MEMORY/stm32_test_memory - Copy.rar` |
| `2024-08_fpga_1memory_8to16/` | 2024-08-01 | **The baseline of the ASIC RTL.** Libero v2024.1 project `stm32` (SmartFusion2), one SRAM, `ecc_sel[1:0]` = 00 bypass / 01 Hamming / 10 TBEC / 11 EDAC, `erro` output (Hamming error, also on an LED). Its encoders match the 8-bit firmware's C encoders bit for bit. Top: `component/work/stm32/stm32.v` → `hdl/fpga_top_design.sv` → `hdl/hamming_top_design.sv` | Drive download `Documents of project…zip` → `FPGA 1 MEMORY/stm32_test_memory - Copy (2).zip` |
| `2024-10_fpga_1memory_saboteur/` | 2024-10-18 | The saboteur build (see below). **Not carried into the ASIC** (decision D7) | Drive download `Documents of project…zip` → `stm32_test_memory - Copy (versão com sabotador).rar` |

## `firmware/`: STM32 firmware for the 8→16 FPGA

| Folder | Date | What it is |
|---|---|---|
| `2024-07_stm_test_mem_fw/`, `2024-08_stm_test_mem_fw/` | 2024-07-25, 2024-08-21 | Standalone STM32L4R5 test firmware (`Core/Src/main.c`). `bypass()/Hamming()/TBEC()/EDAC()` drive PC0/PC1 = 00/10/01/11 (PC0 listed first), which pins **PC0 = `ecc_sel0`**. The August version adds `burstErrorRSC/Hamming/EDAC`: write in ECC mode, flip *n* adjacent bits at a random position via a bypass write, read back, count correct words. Sources: the two `stm_test_mem.rar` copies in the Drive downloads |
| `2024-09_NASCERR_slave_early/` | 2024-08-28 – 09-03 | Earliest `NASCERR_slave` (I2C slave for the satellite's on-board computer). No software encoders, so its ECC-mode error counts compare data against raw codewords and are wrong. Source: `NASCERR_slave-master.zip` (not stored) |
| `Projeto_NASCERR_UFC-image_processing/` | 2024-09 → | The later 8-bit `NASCERR_slave` (`Firmware_CubeSat/NASCERR_slave`), with the C reference encoders `Core/Src/NASCERR_ECCs.c`. **The testbench compiles that file unmodified as its golden model.** Also the image-processing experiment and on-board-computer emulators. Source: `Projeto_NASCERR_UFC-image_processing.zip` |

`Debug/` build output is gitignored throughout.

## Original downloads (not in git)

The archives everything here was extracted from aren't stored in the repository: their
contents are extracted above, so they'd only double the size. On the machine that did
the import they're kept, gitignored, in `archives/`:
- `drive-download-20261110T130620Z-1-001.zip` and `Documents of project Libero and
  STMCubeIDE-20261110T131809Z-1-001.zip`: the recovered Google Drive folders. They also held
  16→32 items (extracted to `../nascerr_16to32/history/`) and `test_mem_controller/`
  (2024-07-01), a Libero project with its sources missing. Its `.prjx` lists `hamming_*`,
  `MRSC_*`, `TBEC_RSC_*`, `control_flow.sv`, `mux_io.sv`, `reg_16bits_parallel_parallel.sv`
  and a `CoreMemCtrl` block; it was not extracted.
- `Projeto_NASCERR_UFC-image_processing.zip`: the 8-bit firmware repository snapshot.

The RAR files inside are RAR5. The system 7-Zip has no RAR codec, and `unrar-free` 0.1.1
writes zero-filled or garbage files for RAR5. Extract them with
`python3 -I scripts/rar_extract.py ARCHIVE OUTDIR` (system libarchive through ctypes).

## The saboteur (`fpga/2024-10_fpga_1memory_saboteur`)

- **Select map changes:** `00` bypass, `01` TBEC, `10` **TBEC_random**, `11` EDAC. Hamming
  is commented out, and so is `erro`.
- **TBEC_random:** `Encoder16bits_random` is the TBEC encoder with its codeword XORed with a
  3-bit `random_value`, i.e. **1–3 errors on codeword bits [2:0]** (data bits 0–2). The
  normal TBEC decoder reads it back.
- **Error source:** `random_value` comes from `lfsr_random_generator`, a 2-bit LFSR clocked by
  the FPGA's free-running RC oscillator, with an asynchronous active-high reset on the new
  top-level pin `turn_on_generator`. That's what the firmware's `start_saboteur` pulse
  drives (PC3 in the 8-bit firmware). After reset the LFSR cycles 01 → 11 → 10, giving
  `random_value` 001 → 111 → 011. So the error injected on a write depends on the oscillator
  phase at that moment.
- **Firmware labels explained:** the 8-bit firmware's "TBEC / TBEC_RANDOM / EDAC" select
  comments describe this build. Its reference encoders (Hamming/TBEC/EDAC) describe 2024-08.
- **The TBEC decoder typo is fixed here too, identically** (`signal[0] = data_in[0]`). That
  independently confirms the fix in `../dyn_ecc_8to16/rtl`.
- **Smaller address bus:** 11 bits (`MCU_A_0..10`, 2K words). Device per the embedded
  programming map: **M2S005** (the PCB BOM has M2S025).
- **Correction results (VCS, all 256 values):** the fixed TBEC decoder corrects
  `random_value = 001` for every word, and **never** corrects `011` or `111` (0/256 each).

## Verification of the 2024-08 codecs (VCS, all 256 data values)

| | Hamming | TBEC (`Encoder16bits`/`Decoder16bits`) | EDAC |
|---|---|---|---|
| Encoder vs. 8-bit firmware C (`NASCERR_ECCs.c`) | identical | identical | identical |
| Error-free decode | correct | correct | correct |
| Every single-bit error corrected | yes | **no**: 128/256 words fail at 13 of 16 positions (all but bits 0, 8, 9); 1,664 of 4,096 | yes |

The TBEC failure is a typo in `hdl/Decoder16bits.sv` line 24, `signal[0] = data_in[2];`
(should be `data_in[0]`). It's fixed in `../dyn_ecc_8to16/rtl/`, not here.

Other quirks in this RTL: `hamming_decoder.sv` calls `$display` inside `always_comb`
(prints on every corrected word); `erro` has no reset; the FPGA drives the MCU data bus
whenever no write is in progress.
