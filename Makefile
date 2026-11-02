# =============================================================================
# Makefile - dyn_ecc UVM verification (VCS)
#
#   make compile            # compile/elaborate the design + TB
#   make run                # compile (if needed) and run the simulation
#   make compile DEBUG=1    # compile with FSDB dump + debug access enabled
#   make run     DEBUG=1    # run and produce dyn_ecc_tb.fsdb
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
	./$(SIMV)

# Open the FSDB in Verdi with the design + TB sources loaded, so signals can be
# traced back to RTL. Loads the same filelist/options used for compilation.
verdi:
	$(VERDI) -ssf $(FSDB) \
	         -sverilog -ntb_opts $(UVM_OPT) -timescale=$(TIMESCALE) \
	         -f $(FILELIST) &

clean:
	rm -rf $(SIMV) $(SIMV).daidir csrc ucli.key vc_hdrs.h \
	       *.fsdb *.vpd *.vcd novas.* verdi* *.log DVEfiles AN.DB
