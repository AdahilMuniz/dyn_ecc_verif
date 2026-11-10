# dyn_ecc_verif

The NASCERR / dyn_ecc project: an ECC "man-in-the-middle" placed between a microcontroller
(STM32, FMC bus) and a fragile external SRAM. It encodes writes and corrects reads,
invisibly to the firmware. This repository gathers every version of the project for
reference, plus the RTL and UVM verification environment of the **8→16 ASIC**.

| Folder | Contents |
|---|---|
| [`nascerr_8to16_legacy/`](nascerr_8to16_legacy/README.md) | The **original** 8→16 version (one SRAM; Hamming / TBEC / EDAC): FPGA projects, STM32 firmware, recovered archives. The architecture the ASIC follows |
| [`nascerr_16to32/`](nascerr_16to32/README.md) | The **most developed** 16→32 version (two SRAMs; TBEC / MRSC): RTL (`fpga/FPGA_design` submodule), latest firmware v3, history. The source of resources to reuse |
| [`dyn_ecc_8to16/`](dyn_ecc_8to16/README.md) | The **ASIC RTL** (8→16): the verified 2024-08 algorithms in the clean structure of the 2026 rewrite, plus its equivalence proof and the reference rewrite |
| `tb/` `env/` `vips/` `seqs/` `tests/` `dpi/` `script/` `Makefile` | UVM 1.2 verification environment for `dyn_ecc_8to16/rtl` (Synopsys VCS / Verdi) |
| `docs/` | `system_analysis.md`: how the versions relate, scope decisions (D1–D14) and open items |
| `scripts/` | Helpers (`rar_extract.py` for the RAR5 archives) |

## Getting started

```sh
git clone <repo> && cd dyn_ecc_verif
git submodule update --init      # nascerr_16to32/fpga/FPGA_design
make regress                     # compile once, run all sequences (needs VCS + UVM 1.2)
make lint                        # Verilator -Wall lint of the ASIC RTL
make equiv                       # exhaustive codec equivalence vs. the recovered RTL + firmware C
make run SEQ_NAME=ecc_single_err_seq PLUSARGS=+ECC_MODE=TBEC
```

`CLAUDE.md` describes the verification environment in detail.
