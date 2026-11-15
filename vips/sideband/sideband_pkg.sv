// Sideband verification IP package: sequence item, config, sequencer, driver,
// monitor and agent. Compile tb/sideband_if.sv before this package and add
// +incdir+vips/sideband so the includes below resolve.

`ifndef SIDEBAND_PKG_SV
`define SIDEBAND_PKG_SV

package sideband_pkg;

  import uvm_pkg::*;
  `include "uvm_macros.svh"

  // Width of the sideband write (toward DUT) and read (from DUT) signals.
  // Match the sideband_if defaults; the item fields are sized from these, so
  // keep them >= the widths wired in the testbench to avoid truncation.
  parameter int SB_WWIDTH = 16;
  parameter int SB_RWIDTH = 16;

  `include "sideband_seq_item.sv"
  `include "sideband_agent_cfg.sv"
  `include "sideband_sequencer.sv"
  `include "sideband_driver.sv"
  `include "sideband_monitor.sv"
  `include "sideband_agent.sv"

endpackage : sideband_pkg

`endif // SIDEBAND_PKG_SV
