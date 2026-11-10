// Backdoor access to the SRAM model's storage (dyn_ecc_tb.u_sram, an
// IS61WV2M16AL instance), with no bus activity. Used by the scoreboard to see
// what the DUT really stored, and by sequences to inject bit errors (standing in
// for the FPGA saboteur).
interface sram_backdoor_if;

  function automatic logic [15:0] peek(input logic [20:0] addr);
    return {dyn_ecc_tb.u_sram.mem_array_UB[addr], dyn_ecc_tb.u_sram.mem_array_LB[addr]};
  endfunction

  function automatic void poke(input logic [20:0] addr, input logic [15:0] data);
    dyn_ecc_tb.u_sram.mem_array_UB[addr] = data[15:8];
    dyn_ecc_tb.u_sram.mem_array_LB[addr] = data[7:0];
  endfunction

  // Flip the bits set in `mask` at `addr`.
  function automatic void flip(input logic [20:0] addr, input logic [15:0] mask);
    poke(addr, peek(addr) ^ mask);
  endfunction

endinterface
