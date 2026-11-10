// Code your testbench here
// or browse Examples
//`include "memory.v"

`timescale 1ns / 100ps



module memory_tb;

  //Controle e endereço
  logic [15:0] data_in;
  logic [15:0] data_out;
  logic        data_in_en;
  logic [20:0] Address;
  logic        OE_bar;
  logic        CE_bar;
  logic        WE_bar;
  logic        LB_bar;
  logic        UB_bar;
  logic        sel;

  //fio dados processador
  wire  [15:0] dataIO;

  //fio adding the error
  wire  [15:0] dataIO1;

  // Test wires
  //wire [15:0] data_error;
  //wire [15:0] data_error2;
  //wire [15:0] dataout;



  assign dataIO   = data_in_en ? data_in : 16'bz;
  assign data_out = dataIO;

  top test (
    .mcu_fpga_io(dataIO),
    .fpga_mem_io(dataIO1),
    //.dataerror(data_error),
    //.datasanserror(data_error2),
    //.outdecode(dataout),
    .sel(sel),
    .write_enable(data_in_en)
  );



  initial begin

    $dumpfile("dump.vcd");
    $dumpvars(0, test_memory);

    Address = 21'b0;
    data_in = 16'b0000000010001110;
    sel     = 1;

    $display("tempo inicial =    %d ns", $time);

    for (int i = 0; i < 10; i++) begin
      write;
      read;
      $display("------test %d------", i + 1);
      $display("data_in = %h", data_in);
      //display;
      $display("data_out = %h", data_out);


      data_in = data_in + 1;
      Address = Address + 16;

    end

    $display("tempo final =    %d ns", $time);



  end

  task write;
    data_in_en = 1;
    CE_bar     = 0;
    UB_bar     = 0;
    LB_bar     = 0;
    WE_bar     = 0;
    //#taw
    CE_bar     = 1;
    UB_bar     = 1;
    LB_bar     = 1;
    WE_bar     = 1;
    data_in_en = 0;
    #1;

    // 12 ns
  endtask


  task read;
    data_in_en = 0;
    CE_bar     = 0;
    UB_bar     = 0;
    LB_bar     = 0;
    OE_bar     = 0;
    //#trc
    CE_bar     = 1;
    UB_bar     = 1;
    LB_bar     = 1;
    OE_bar     = 1;
    data_in_en = 1;
    #1;
    //17 ns
  endtask


  /*task display;
    $display("data_sans_error = %b",data_error2);
    $display("data_error = %b",data_error);
  endtask*/

endmodule
