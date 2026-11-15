// Sideband transaction item.
//   - wdata : value the driver drives onto sideband_if.wsig (toward the DUT,
//             e.g. the ecc mode select).
//   - rdata : value captured from sideband_if.rsig (from the DUT, e.g. flag_out).
// There is no read/write op: the sideband interface has a continuously driven
// input (wsig) and a continuously observed output (rsig), so every item both
// drives wsig and samples rsig.

class sideband_seq_item extends uvm_sequence_item;

  rand bit [SB_WWIDTH-1:0] wdata;
       bit [SB_RWIDTH-1:0] rdata;

  `uvm_object_utils_begin(sideband_seq_item)
    `uvm_field_int(wdata, UVM_ALL_ON)
    `uvm_field_int(rdata, UVM_ALL_ON)
  `uvm_object_utils_end

  function new(string name = "sideband_seq_item");
    super.new(name);
  endfunction

  function string convert2string();
    return $sformatf("wsig=0x%0h rsig=0x%0h", wdata, rdata);
  endfunction

endclass
