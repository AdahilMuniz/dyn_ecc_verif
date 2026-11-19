#!/bin/sh
# Exhaustive equivalence of the dyn_ecc 8->16 codecs with the recovered FPGA RTL
# and the 8-bit firmware's C encoders (see codec_equiv_tb.sv). Prints EQUIV
# PASS/FAIL and exits nonzero on FAIL. Needs Synopsys VCS. Build output goes to
# dyn_ecc_8to16/equiv/build/ (gitignored).
set -e
cd "$(dirname "$0")/../.."

EQ=dyn_ecc_8to16/equiv
OUT=$EQ/build
LEGACY=nascerr_8to16_legacy/fpga
DPI_SRC=nascerr_8to16_legacy/firmware/Projeto_NASCERR_UFC-image_processing/Firmware_CubeSat/NASCERR_slave/Core/Src/NASCERR_ECCs.c

mkdir -p "$OUT"
vcs -full64 -sverilog -timescale=1ns/1ps \
    -Mdir="$OUT/csrc" -o "$OUT/simv_equiv" -l "$OUT/compile.log" \
    $(grep -E '_(enc|dec)\.sv$' dyn_ecc_8to16/rtl/rtl_filelist.f) \
    $LEGACY/2024-08_fpga_1memory_8to16/hdl/hamming_encoder.sv \
    $LEGACY/2024-08_fpga_1memory_8to16/hdl/hamming_decoder.sv \
    $LEGACY/2024-08_fpga_1memory_8to16/hdl/Encoder16bits.sv \
    $LEGACY/2024-10_fpga_1memory_saboteur/hdl/Decoder16bits.sv \
    $LEGACY/2024-08_fpga_1memory_8to16/hdl/EDACencoder.sv \
    $LEGACY/2024-08_fpga_1memory_8to16/hdl/EDACdecoder.sv \
    $EQ/codec_equiv_tb.sv "$DPI_SRC" -CFLAGS "-I$(pwd)/$EQ" > /dev/null

# The legacy Hamming decoder's $display calls are filtered out.
"$OUT/simv_equiv" | grep -E '^EQUIV' | tee "$OUT/equiv.log"
grep -q '^EQUIV PASS' "$OUT/equiv.log"
