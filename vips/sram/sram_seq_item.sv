// SRAM transaction item: operation (read or write), address and data.
// For a WRITE, `data` is the value to drive; for a READ, the driver fills
// `data` with the value captured from the bus so the sequence can see it.

class sram_seq_item extends uvm_sequence_item;

  rand sram_op_e                 op;
  rand bit [SRAM_ADDR_WIDTH-1:0] addr;
  rand bit [SRAM_DATA_WIDTH-1:0] data;

  `uvm_object_utils_begin(sram_seq_item)
    `uvm_field_enum(sram_op_e, op,   UVM_ALL_ON)
    `uvm_field_int (addr,            UVM_ALL_ON)
    `uvm_field_int (data,            UVM_ALL_ON)
  `uvm_object_utils_end

  function new(string name = "sram_seq_item");
    super.new(name);
  endfunction

  function string convert2string();
    return $sformatf("op=%s addr=0x%0h data=0x%0h", op.name(), addr, data);
  endfunction

endclass
