# =============================================================================
# Makefile - dyn_ecc UVM verification (VCS)
#
#   make compile            # compile/elaborate the design + TB
#   make run                # compile (if needed) and run the simulation
#   make compile DEBUG=1    # compile with FSDB dump + debug access enabled
#   make run     DEBUG=1    # run and produce dyn_ecc_tb.fsdb
#   make run TEST_NAME=... SEQ_NAME=...   # choose the UVM test / sequence
#   make run SEQ_NAME=ecc_single_err_seq PLUSARGS=+ECC_MODE=TBEC
#   make run VERBOSITY=UVM_HIGH           # set UVM verbosity
#   make regress            # run every sequence, one log per sequence
#   make lint               # Verilator -Wall lint of the ASIC RTL
#   make equiv              # exhaustive codec equivalence vs. the recovered RTL + firmware C
#   make clean              # remove generated files
#
# DEBUG=0 (default) : no FSDB, fastest run.
# DEBUG=1           : adds debug access, KDB, and +define+DUMP_FSDB so the TB
#                     calls $fsdbDumpvars and writes dyn_ecc_tb.fsdb.
# =============================================================================

# ---- Tool / options ---------------------------------------------------------
VCS        := vcs
VERDI      := verdi
SIMV       := simv
FILELIST   := script/verif_filelist.f
TIMESCALE  := 1ns/1ps
UVM_OPT    := uvm-1.2
FSDB       := dyn_ecc_tb.fsdb

# Golden ECC model: the 8-bit firmware's encoders, compiled unmodified and
# called through DPI-C. dpi/NASCERR_ECCs.h stands in for the firmware header.
DPI_SRC    := nascerr_8to16_legacy/firmware/Projeto_NASCERR_UFC-image_processing/Firmware_CubeSat/NASCERR_slave/Core/Src/NASCERR_ECCs.c

VCS_OPTS   := -full64 -sverilog -ntb_opts $(UVM_OPT) -timescale=$(TIMESCALE) -f $(FILELIST) \
              $(DPI_SRC) -CFLAGS -I$(CURDIR)/dpi

# ---- Run-time selection ------------------------------------------------------
# TEST_NAME : UVM test to run (+UVM_TESTNAME)
# SEQ_NAME  : sequence the test runs, overridden by name (+SEQ_NAME)
# VERBOSITY : UVM verbosity (+UVM_VERBOSITY), e.g. UVM_LOW/UVM_MEDIUM/UVM_HIGH
TEST_NAME  ?= dyn_ecc_base_test
SEQ_NAME   ?= dyn_ecc_base_seq
VERBOSITY  ?= UVM_MEDIUM
PLUSARGS   ?=
LOG        ?= sim.log

SIM_OPTS   := +UVM_TESTNAME=$(TEST_NAME) +SEQ_NAME=$(SEQ_NAME) \
              +UVM_VERBOSITY=$(VERBOSITY) $(PLUSARGS) -l $(LOG)

# ---- DEBUG switch ------------------------------------------------------------
DEBUG ?= 0
ifeq ($(DEBUG),1)
  VCS_OPTS += -debug_access+all -kdb +define+DUMP_FSDB
endif

# ---- Targets -----------------------------------------------------------------
.PHONY: all compile run regress lint equiv verdi clean

DESIGN_FILELIST := script/design_filelist.f
LEGACY_8TO16    := nascerr_8to16_legacy/fpga
EQUIV_REFS      := $(LEGACY_8TO16)/2024-08_fpga_1memory_8to16/hdl/hamming_encoder.sv \
                   $(LEGACY_8TO16)/2024-08_fpga_1memory_8to16/hdl/hamming_decoder.sv \
                   $(LEGACY_8TO16)/2024-08_fpga_1memory_8to16/hdl/Encoder16bits.sv \
                   $(LEGACY_8TO16)/2024-10_fpga_1memory_saboteur/hdl/Decoder16bits.sv \
                   $(LEGACY_8TO16)/2024-08_fpga_1memory_8to16/hdl/EDACencoder.sv \
                   $(LEGACY_8TO16)/2024-08_fpga_1memory_8to16/hdl/EDACdecoder.sv

REGRESS_SEQS := write_read_seq ecc_pattern_seq ecc_single_err_seq ecc_burst_err_seq

all: run

compile:
	$(VCS) $(VCS_OPTS) -o $(SIMV)

run: compile
	./$(SIMV) $(SIM_OPTS)

# Compile once, run each sequence; print each log's UVM error/fatal counts.
regress: compile
	@for s in $(REGRESS_SEQS); do \
	  ./$(SIMV) +UVM_TESTNAME=$(TEST_NAME) +SEQ_NAME=$$s +UVM_VERBOSITY=$(VERBOSITY) \
	            $(PLUSARGS) -l sim_$$s.log > /dev/null; \
	  printf '%-22s %s\n' $$s "$$(grep -E '^UVM_(ERROR|FATAL) +:' sim_$$s.log | tr -s ' ' | tr '\n' ' ')"; \
	done

# Lint the ASIC RTL (waivers are inline, each with its reason).
lint:
	verilator --lint-only --language 1800-2017 -Wall -sv -f $(DESIGN_FILELIST) --top-module dyn_ecc_top

# Exhaustive equivalence of the rewritten codecs with the recovered FPGA RTL and
# the firmware's C encoders (see dyn_ecc_8to16/equiv/codec_equiv_tb.sv).
equiv:
	$(VCS) -full64 -sverilog -timescale=$(TIMESCALE) -Mdir=csrc_equiv -o simv_equiv -l equiv_compile.log \
	       $$(grep -E '_(enc|dec)\.sv$$' $(DESIGN_FILELIST)) $(EQUIV_REFS) \
	       dyn_ecc_8to16/equiv/codec_equiv_tb.sv $(DPI_SRC) -CFLAGS -I$(CURDIR)/dpi > /dev/null
	@./simv_equiv | grep -E '^EQUIV' | tee equiv.log
	@grep -q '^EQUIV PASS' equiv.log

# Open the FSDB in Verdi with the design + TB sources loaded, so signals can be
# traced back to RTL. Loads the same filelist/options used for compilation.
verdi:
	$(VERDI) -ssf $(FSDB) \
	         -sverilog -ntb_opts $(UVM_OPT) -timescale=$(TIMESCALE) \
	         -f $(FILELIST) &

clean:
	rm -rf $(SIMV) $(SIMV).daidir csrc ucli.key vc_hdrs.h simv_equiv simv_equiv.daidir csrc_equiv \
	       *.fsdb *.vpd *.vcd novas.* verdi* *.log DVEfiles AN.DB
