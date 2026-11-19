#!/bin/sh
# Verilator -Wall lint of the dyn_ecc 8->16 ASIC RTL. Must stay clean.
# Run from anywhere; paths are resolved from the repo root.
set -e
cd "$(dirname "$0")/.."
verilator --lint-only --language 1800-2017 -Wall -sv \
          -f dyn_ecc_8to16/rtl/rtl_filelist.f --top-module dyn_ecc_top
echo "LINT CLEAN"
