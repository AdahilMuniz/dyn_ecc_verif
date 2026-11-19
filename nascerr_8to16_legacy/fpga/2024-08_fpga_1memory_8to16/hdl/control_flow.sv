module control_flow(inout wire [0:15]data_in, 
                    input logic write_en,
                    input logic chip_sel,
                    output logic [0:15]data_encoder,
                    inout wire [0:15]data_mem);

assign data_encoder = (write_en == 0 && chip_sel == 0) ? data_in : 16'bz;
assign data_in = (write_en == 0 && chip_sel == 0) ?  16'bz : data_mem ;
endmodule