//////////////////////////////////////////////////////////////////////
// Created by SmartDesign Thu Oct 17 16:28:41 2024
// Version: 2024.1 2024.1.0.3
//////////////////////////////////////////////////////////////////////

`timescale 1ns / 100ps

// stm32
module stm32(
    // Inputs
    MCU_A_0,
    MCU_A_1,
    MCU_A_10,
    MCU_A_2,
    MCU_A_3,
    MCU_A_4,
    MCU_A_5,
    MCU_A_6,
    MCU_A_7,
    MCU_A_8,
    MCU_A_9,
    MCU_CS,
    MCU_LB,
    MCU_OE,
    MCU_UB,
    MCU_WE,
    ecc_sel0,
    ecc_sel1,
    turn_on_generator,
    // Outputs
    SRAM_A_0,
    SRAM_A_1,
    SRAM_A_10,
    SRAM_A_2,
    SRAM_A_3,
    SRAM_A_4,
    SRAM_A_5,
    SRAM_A_6,
    SRAM_A_7,
    SRAM_A_8,
    SRAM_A_9,
    SRAM_CS,
    SRAM_LB,
    SRAM_OE,
    SRAM_UB,
    SRAM_WE,
    // Inouts
    fpga_mem_io,
    mcu_fpga_io
);

//--------------------------------------------------------------------
// Input
//--------------------------------------------------------------------
input         MCU_A_0;
input         MCU_A_1;
input         MCU_A_10;
input         MCU_A_2;
input         MCU_A_3;
input         MCU_A_4;
input         MCU_A_5;
input         MCU_A_6;
input         MCU_A_7;
input         MCU_A_8;
input         MCU_A_9;
input         MCU_CS;
input         MCU_LB;
input         MCU_OE;
input         MCU_UB;
input         MCU_WE;
input         ecc_sel0;
input         ecc_sel1;
input         turn_on_generator;
//--------------------------------------------------------------------
// Output
//--------------------------------------------------------------------
output        SRAM_A_0;
output        SRAM_A_1;
output        SRAM_A_10;
output        SRAM_A_2;
output        SRAM_A_3;
output        SRAM_A_4;
output        SRAM_A_5;
output        SRAM_A_6;
output        SRAM_A_7;
output        SRAM_A_8;
output        SRAM_A_9;
output        SRAM_CS;
output        SRAM_LB;
output        SRAM_OE;
output        SRAM_UB;
output        SRAM_WE;
//--------------------------------------------------------------------
// Inout
//--------------------------------------------------------------------
inout  [15:0] fpga_mem_io;
inout  [15:0] mcu_fpga_io;
//--------------------------------------------------------------------
// Nets
//--------------------------------------------------------------------
wire          ecc_sel0;
wire          ecc_sel1;
wire   [15:0] fpga_mem_io;
wire   [2:0]  lfsr_random_generator_0_random_value_0;
wire          MCU_CS;
wire   [15:0] mcu_fpga_io;
wire          MCU_WE;
wire          OSC_C0_0_RCOSC_25_50MHZ_O2F;
wire          MCU_A_0;
wire          MCU_A_1;
wire          MCU_A_2;
wire          MCU_A_3;
wire          MCU_A_4;
wire          MCU_A_5;
wire          MCU_A_6;
wire          MCU_A_7;
wire          MCU_A_8;
wire          MCU_A_9;
wire          MCU_A_10;
wire          MCU_LB;
wire          MCU_OE;
wire          MCU_UB;
wire          turn_on_generator;
wire          SRAM_A_0_net_0;
wire          SRAM_A_1_net_0;
wire          SRAM_A_2_net_0;
wire          SRAM_A_3_net_0;
wire          SRAM_A_4_net_0;
wire          SRAM_A_5_net_0;
wire          SRAM_A_6_net_0;
wire          SRAM_A_7_net_0;
wire          SRAM_A_8_net_0;
wire          SRAM_A_9_net_0;
wire          SRAM_A_10_net_0;
wire          MCU_CS_net_0;
wire          MCU_WE_net_0;
wire          SRAM_OE_net_0;
wire          SRAM_UB_net_0;
wire          SRAM_LB_net_0;
wire   [1:0]  ecc_sel_net_0;
//--------------------------------------------------------------------
// Top level output port assignments
//--------------------------------------------------------------------
assign SRAM_A_0_net_0  = MCU_A_0;
assign SRAM_A_0        = SRAM_A_0_net_0;
assign SRAM_A_1_net_0  = MCU_A_1;
assign SRAM_A_1        = SRAM_A_1_net_0;
assign SRAM_A_2_net_0  = MCU_A_2;
assign SRAM_A_2        = SRAM_A_2_net_0;
assign SRAM_A_3_net_0  = MCU_A_3;
assign SRAM_A_3        = SRAM_A_3_net_0;
assign SRAM_A_4_net_0  = MCU_A_4;
assign SRAM_A_4        = SRAM_A_4_net_0;
assign SRAM_A_5_net_0  = MCU_A_5;
assign SRAM_A_5        = SRAM_A_5_net_0;
assign SRAM_A_6_net_0  = MCU_A_6;
assign SRAM_A_6        = SRAM_A_6_net_0;
assign SRAM_A_7_net_0  = MCU_A_7;
assign SRAM_A_7        = SRAM_A_7_net_0;
assign SRAM_A_8_net_0  = MCU_A_8;
assign SRAM_A_8        = SRAM_A_8_net_0;
assign SRAM_A_9_net_0  = MCU_A_9;
assign SRAM_A_9        = SRAM_A_9_net_0;
assign SRAM_A_10_net_0 = MCU_A_10;
assign SRAM_A_10       = SRAM_A_10_net_0;
assign MCU_CS_net_0    = MCU_CS;
assign SRAM_CS         = MCU_CS_net_0;
assign MCU_WE_net_0    = MCU_WE;
assign SRAM_WE         = MCU_WE_net_0;
assign SRAM_OE_net_0   = MCU_OE;
assign SRAM_OE         = SRAM_OE_net_0;
assign SRAM_UB_net_0   = MCU_UB;
assign SRAM_UB         = SRAM_UB_net_0;
assign SRAM_LB_net_0   = MCU_LB;
assign SRAM_LB         = SRAM_LB_net_0;
//--------------------------------------------------------------------
// Concatenation assignments
//--------------------------------------------------------------------
assign ecc_sel_net_0 = { ecc_sel1 , ecc_sel0 };
//--------------------------------------------------------------------
// Component instances
//--------------------------------------------------------------------
//--------fpga_top_design
fpga_top_design fpga_top_design_0(
        // Inputs
        .write_en     ( MCU_WE ),
        .chip_sel     ( MCU_CS ),
        .ecc_sel      ( ecc_sel_net_0 ),
        .random_value ( lfsr_random_generator_0_random_value_0 ),
        // Inouts
        .mcu_fpga_io  ( mcu_fpga_io ),
        .fpga_mem_io  ( fpga_mem_io ) 
        );

//--------lfsr_random_generator
lfsr_random_generator lfsr_random_generator_0(
        // Inputs
        .clk          ( OSC_C0_0_RCOSC_25_50MHZ_O2F ),
        .reset        ( turn_on_generator ),
        // Outputs
        .random_value ( lfsr_random_generator_0_random_value_0 ) 
        );

//--------OSC_C0
OSC_C0 OSC_C0_0(
        // Outputs
        .RCOSC_25_50MHZ_O2F ( OSC_C0_0_RCOSC_25_50MHZ_O2F ) 
        );


endmodule
