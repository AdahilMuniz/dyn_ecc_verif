module top
  import typedefs::*;
(
  input sel_t       sel,
  input logic       read_en,
  write_en,
  inout wire  [7:0] cpu_data,
  ram_up_data,
  ram_down_data
);
  logic cpu_oe, ram_up_oe, ram_down_oe;
  logic [7:0] cpu_data_out, ram_up_data_out, ram_down_data_out;

  control control0 (
    .sel,
    .read_en,
    .write_en,
    .cpu_data_in(cpu_data),
    .cpu_data_out,
    .ram_up_data_in(ram_up_data),
    .ram_up_data_out,
    .ram_down_data_in(ram_down_data),
    .ram_down_data_out,
    .cpu_oe,
    .ram_up_oe,
    .ram_down_oe
  );

  assign cpu_data      = cpu_oe ? cpu_data_out : 8'hZZ;
  assign ram_up_data   = ram_up_oe ? ram_up_data_out : 8'hZZ;
  assign ram_down_data = ram_down_oe ? ram_down_data_out : 8'hZZ;
endmodule
