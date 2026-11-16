// Behavioral single-port SRAM storage, implemented as a class so its backing
// array is shared by reference. The testbench creates one per physical memory
// and publishes the handle on the uvm_config_db; a UVM test that retrieves the
// same handle edits the SAME array, giving live backdoor peek/poke.
//
// Width note: the array is SRAM_DATA_WIDTH/SRAM_ADDR_WIDTH wide (16/16). When
// the testbench wires an 8-bit data bus, only the low 8 bits carry meaning;
// stored values are zero-extended.

`ifndef SRAM_MEM_MODEL_SV
`define SRAM_MEM_MODEL_SV

class sram_mem_model extends uvm_object;

  // Sparse backing store, keyed by address. Public on purpose: a test may index
  // `mem` directly for backdoor access, or use the methods below.
  logic [SRAM_DATA_WIDTH-1:0] mem [bit [SRAM_ADDR_WIDTH-1:0]];

  `uvm_object_utils(sram_mem_model)

  function new(string name = "sram_mem_model");
    super.new(name);
  endfunction

  // Backdoor write.
  function void poke(bit [SRAM_ADDR_WIDTH-1:0] addr,
                     logic [SRAM_DATA_WIDTH-1:0] data);
    mem[addr] = data;
  endfunction

  // Backdoor read; unwritten locations read as 0.
  function logic [SRAM_DATA_WIDTH-1:0] peek(bit [SRAM_ADDR_WIDTH-1:0] addr);
    return mem.exists(addr) ? mem[addr] : '0;
  endfunction

  function bit is_written(bit [SRAM_ADDR_WIDTH-1:0] addr);
    return mem.exists(addr);
  endfunction

endclass

`endif // SRAM_MEM_MODEL_SV
