
module reg_16bits_parallel_parallel (inout wire [15:0] data,  
                   				     input logic clock, 
                                     input write_en,
                                     input logic chip_sel,
                                     input logic [10:0]addr,
                                     output logic sram_disable);
                                     
 
    localparam REG_ADDR = 11'h000;
    logic [15:0] temp_in;
    logic [15:0] temp_out;
    
    assign data = (!chip_sel && write_en && (addr == REG_ADDR)) ? temp_out : 16'bz ;
    assign temp_in = data;
    assign sram_disable = (addr == REG_ADDR) & write_en;
    
   always_ff @ (posedge clock) begin
       if(chip_sel == 0 && write_en == 0 ) begin
            temp_out <= temp_in;
            
       end
     end
endmodule