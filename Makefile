# =============================================================================
# Makefile - dyn_ecc UVM verification (VCS)
#
#   make compile            # compile/elaborate the design + TB
#   make run                # compile (if needed) and run the simulation
#   make compile DEBUG=1    # compile with FSDB dump + debug access enabled
#   make run     DEBUG=1    # run and produce dyn_ecc_tb.fsdb
#   make run TEST_NAME=... SEQ_NAME=...   # choose the UVM test / sequence
#   make run VERBOSITY=UVM_HIGH           # set UVM verbosity
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

VCS_OPTS   := -full64 -sverilog -ntb_opts $(UVM_OPT) -timescale=$(TIMESCALE) -f $(FILELIST)

# ---- Run-time selection ------------------------------------------------------
# TEST_NAME : UVM test to run (+UVM_TESTNAME)
# SEQ_NAME  : sequence the test runs, overridden by name (+SEQ_NAME)
# VERBOSITY : UVM verbosity (+UVM_VERBOSITY), e.g. UVM_LOW/UVM_MEDIUM/UVM_HIGH
TEST_NAME  ?= dyn_ecc_base_test
SEQ_NAME   ?= dyn_ecc_base_seq
VERBOSITY  ?= UVM_MEDIUM

SIM_OPTS   := +UVM_TESTNAME=$(TEST_NAME) +SEQ_NAME=$(SEQ_NAME) \
              +UVM_VERBOSITY=$(VERBOSITY) -l sim.log

# ---- DEBUG switch ------------------------------------------------------------
DEBUG ?= 0
ifeq ($(DEBUG),1)
  VCS_OPTS += -debug_access+all -kdb +define+DUMP_FSDB
endif

# ---- Targets -----------------------------------------------------------------
.PHONY: all compile run verdi clean

all: run

compile:
	$(VCS) $(VCS_OPTS) -o $(SIMV)

run: compile
	./$(SIMV) $(SIM_OPTS)

# Open the FSDB in Verdi with the design + TB sources loaded, so signals can be
# traced back to RTL. Loads the same filelist/options used for compilation.
verdi:
	$(VERDI) -ssf $(FSDB) \
	         -sverilog -ntb_opts $(UVM_OPT) -timescale=$(TIMESCALE) \
	         -f $(FILELIST) &

clean:
	rm -rf $(SIMV) $(SIMV).daidir csrc ucli.key vc_hdrs.h \
	       *.fsdb *.vpd *.vcd novas.* verdi* *.log DVEfiles AN.DB
