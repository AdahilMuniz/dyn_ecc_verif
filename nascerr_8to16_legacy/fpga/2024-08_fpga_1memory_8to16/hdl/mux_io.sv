module mux_io(inout wire [15:0]in1, inout wire [15:0]in2, inout wire [15:0] out1, input logic sel);

assign out1 = sel ? in1 : in2;

endmodule