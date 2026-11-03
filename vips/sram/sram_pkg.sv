// SRAM verification IP package: sequence item, config, sequencer, driver,
// monitor and agent. Compile vips/sram/sram_if.sv before this package and add
// +incdir+vips/sram so the includes below resolve.

`ifndef SRAM_PKG_SV
`define SRAM_PKG_SV

package sram_pkg;

  import uvm_pkg::*;
  `include "uvm_macros.svh"

  // Data/address widths of the SRAM transactions (match sram_if defaults).
  parameter int SRAM_DATA_WIDTH = 16;
  parameter int SRAM_ADDR_WIDTH = 16;

  typedef enum bit { SRAM_READ = 1'b0, SRAM_WRITE = 1'b1 } sram_op_e;

  `include "sram_seq_item.sv"
  `include "sram_agent_cfg.sv"
  `include "sram_sequencer.sv"
  `include "sram_driver.sv"
  `include "sram_monitor.sv"
  `include "sram_agent.sv"

endpackage : sram_pkg

`endif // SRAM_PKG_SV
