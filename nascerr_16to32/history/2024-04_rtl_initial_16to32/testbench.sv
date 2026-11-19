// Code your testbench here
// or browse Examples
`include "memory.v"

`timescale  1ns/100ps

module test_memory;
  
  //Controle e endereço
  logic [15:0] data_in;
  logic [15:0] data_out;
  logic data_in_en;
  logic [20:0] Address;
  logic OE_bar;
  logic CE_bar;
  logic WE_bar;
  logic LB_bar; 
  logic UB_bar;
  
  //fio dados processador
  wire [15:0]dataIO;
  
  //fio dados memoria 1
  wire [15:0] dataIO1;
    
  //fio dados memoria 2
  wire [15:0] dataIO2;
 
   
  assign dataIO = data_in_en ? data_in : 16'bz;
  assign data_out = dataIO;
  
  IS61WV2M16AL memory1(.Address(Address),.dataIO(dataIO1),.OE_bar(OE_bar),.CE_bar(CE_bar),.WE_bar(WE_bar),
                      .LB_bar(LB_bar),.UB_bar(UB_bar));

  IS61WV2M16AL memory2(.Address(Address),.dataIO(dataIO2),.OE_bar(OE_bar),.CE_bar(CE_bar),.WE_bar(WE_bar),
                      .LB_bar(LB_bar),.UB_bar(UB_bar));
  
  combinacional bloco(.data_out(dataIO),.data_out1(dataIO1),.data_out2(dataIO2),.data_write_en(data_in_en));
  

//Parametros de escrita
parameter       twc     = 10 ;
parameter       tscs    = 8 ;
parameter       taw     = 8 ;
parameter       tpwb    = 8 ;
parameter       tha     = 0 ;
parameter       tsa     = 0 ;  
parameter       tpwe1   = 8 ;
parameter       tpwe2   = 10;
parameter       tsd     = 6 ;
parameter       thd     = 0 ;
parameter       thzwe   = 4 ;//tempo máximo
parameter       tlzwe   = 2 ;
  
//Parametros de leitura  
parameter       trc     = 10 ;
parameter       taa     = 10 ;//tempo máximo
parameter       toha    = 2.5 ;
parameter       tace    = 10 ;//tempo máximo  
parameter       tdoe    = 6 ;//tempo máximo
parameter       thzoe   = 5 ;//tempo maximo 
parameter       tlzoe   = 0 ;//tempo maximo 
parameter       thzce   = 5 ;//tempo maximo 
parameter       tlzce   = 5 ;
parameter       tba     = 6 ;//tempo maximo 
parameter       thzb    = 5 ;//tempo maximo
parameter       tlzb    = 0 ;//tempo maximo 
  initial begin
  	
    $dumpfile("dump.vcd");
    $dumpvars(0,test_memory);
    
    Address = 21'b0;
    data_in = 1235;
    
    $display ("tempo inicial =    %d ns", $time );
    
    for(int i = 0;i < 10 ; i++) begin
      write;     
      read;
      	
      $display("data_out = %d",data_out);
      //$display("mem1[%2d] = %d", Address, {memory1.mem_array_UB[Address], memory1.mem_array_LB[Address]});
      //$display("mem2[%2d] = %d", Address, {memory2.mem_array_UB[Address], memory2.mem_array_LB[Address]});
      data_in = data_in + 1;
      Address = Address + 16;

    end
    
    $display ("tempo final =    %d ns", $time );
   
    
    
  end
  
  task write;
     	data_in_en = 1;
    	CE_bar = 0;
        UB_bar = 0;
        LB_bar = 0;
        WE_bar = 0;
        #taw
        CE_bar = 1;
        UB_bar = 1;
        LB_bar = 1;
        WE_bar = 1;
        data_in_en = 0;
        #1;
    
    // 12 ns
  endtask
  
  
  task read;
     	data_in_en = 0;
    	CE_bar = 0;
        UB_bar = 0;
        LB_bar = 0;
        OE_bar = 0;
        #trc
        CE_bar = 1;
        UB_bar = 1;
        LB_bar = 1;
        OE_bar = 1;
        data_in_en = 1;
    	#1;
    	//17 ns
  endtask
    
   
  
  
endmodule