//////////////////////////////////////////////////////////////////////
// Created by SmartDesign Tue Oct 15 17:23:22 2024
// Version: 2024.1 2024.1.0.3
//////////////////////////////////////////////////////////////////////

`timescale 1ns / 100ps

// prj_2_memory_sb
module prj_2_memory_sb(
    // Inputs
    MCU_A_0,
    MCU_A_1,
    MCU_A_10,
    MCU_A_11,
    MCU_A_12,
    MCU_A_13,
    MCU_A_14,
    MCU_A_15,
    MCU_A_16,
    MCU_A_17,
    MCU_A_18,
    MCU_A_19,
    MCU_A_2,
    MCU_A_20,
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
    // Outputs
    LED1,
    LED2,
    SRAM_A_0,
    SRAM_A_1,
    SRAM_A_10,
    SRAM_A_11,
    SRAM_A_12,
    SRAM_A_13,
    SRAM_A_14,
    SRAM_A_15,
    SRAM_A_16,
    SRAM_A_17,
    SRAM_A_18,
    SRAM_A_19,
    SRAM_A_2,
    SRAM_A_20,
    SRAM_A_3,
    SRAM_A_4,
    SRAM_A_5,
    SRAM_A_6,
    SRAM_A_7,
    SRAM_A_8,
    SRAM_A_9,
    SRAM_CS1,
    SRAM_CS2,
    SRAM_LB,
    SRAM_OE,
    SRAM_UB,
    SRAM_WE,
    // Inouts
    mcu_fpga_io,
    mcu_mem_io_down,
    mcu_mem_io_up
);

//--------------------------------------------------------------------
// Input
//--------------------------------------------------------------------
input         MCU_A_0;
input         MCU_A_1;
input         MCU_A_10;
input         MCU_A_11;
input         MCU_A_12;
input         MCU_A_13;
input         MCU_A_14;
input         MCU_A_15;
input         MCU_A_16;
input         MCU_A_17;
input         MCU_A_18;
input         MCU_A_19;
input         MCU_A_2;
input         MCU_A_20;
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
//--------------------------------------------------------------------
// Output
//--------------------------------------------------------------------
output        LED1;
output        LED2;
output        SRAM_A_0;
output        SRAM_A_1;
output        SRAM_A_10;
output        SRAM_A_11;
output        SRAM_A_12;
output        SRAM_A_13;
output        SRAM_A_14;
output        SRAM_A_15;
output        SRAM_A_16;
output        SRAM_A_17;
output        SRAM_A_18;
output        SRAM_A_19;
output        SRAM_A_2;
output        SRAM_A_20;
output        SRAM_A_3;
output        SRAM_A_4;
output        SRAM_A_5;
output        SRAM_A_6;
output        SRAM_A_7;
output        SRAM_A_8;
output        SRAM_A_9;
output        SRAM_CS1;
output        SRAM_CS2;
output        SRAM_LB;
output        SRAM_OE;
output        SRAM_UB;
output        SRAM_WE;
//--------------------------------------------------------------------
// Inout
//--------------------------------------------------------------------
inout  [15:0] mcu_fpga_io;
inout  [15:0] mcu_mem_io_down;
inout  [15:0] mcu_mem_io_up;
//--------------------------------------------------------------------
// Nets
//--------------------------------------------------------------------
wire          ecc_sel0;
wire          ecc_sel1;
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
wire          MCU_A_11;
wire          MCU_A_12;
wire          MCU_A_13;
wire          MCU_A_20;
wire          MCU_CS;
wire   [15:0] mcu_fpga_io;
wire   [15:0] mcu_mem_io_down;
wire   [15:0] mcu_mem_io_up;
wire          MCU_OE;
wire          MCU_WE;
wire          MCU_A_14;
wire          MCU_A_15;
wire          MCU_A_16;
wire          MCU_A_17;
wire          MCU_A_18;
wire          MCU_A_19;
wire          MCU_LB;
wire          MCU_UB;
wire          SRAM_A_19_net_0;
wire          SRAM_A_18_net_0;
wire          MCU_A_9_net_0;
wire          SRAM_A_17_net_0;
wire          MCU_A_8_net_0;
wire          SRAM_A_16_net_0;
wire          MCU_A_7_net_0;
wire          SRAM_A_15_net_0;
wire          MCU_A_6_net_0;
wire          SRAM_A_14_net_0;
wire          MCU_A_5_net_0;
wire          MCU_A_4_net_0;
wire          MCU_A_13_net_0;
wire          MCU_A_3_net_0;
wire          MCU_A_12_net_0;
wire          MCU_A_2_net_0;
wire          MCU_A_11_net_0;
wire          MCU_A_1_net_0;
wire          MCU_A_10_net_0;
wire          MCU_A_0_net_0;
wire          MCU_WE_net_0;
wire          MCU_OE_net_0;
wire          SRAM_LB_net_0;
wire          SRAM_UB_net_0;
wire          MCU_A_20_net_0;
wire          MCU_CS_net_0;
wire          MCU_CS_net_1;
wire          LED1_net_0;
wire          ecc_sel0_net_0;
wire   [1:0]  chip_sel_net_0;
wire   [1:0]  ecc_sel_net_0;
//--------------------------------------------------------------------
// Top level output port assignments
//--------------------------------------------------------------------
assign SRAM_A_19_net_0 = MCU_A_19;
assign SRAM_A_19       = SRAM_A_19_net_0;
assign SRAM_A_18_net_0 = MCU_A_18;
assign SRAM_A_18       = SRAM_A_18_net_0;
assign MCU_A_9_net_0   = MCU_A_9;
assign SRAM_A_9        = MCU_A_9_net_0;
assign SRAM_A_17_net_0 = MCU_A_17;
assign SRAM_A_17       = SRAM_A_17_net_0;
assign MCU_A_8_net_0   = MCU_A_8;
assign SRAM_A_8        = MCU_A_8_net_0;
assign SRAM_A_16_net_0 = MCU_A_16;
assign SRAM_A_16       = SRAM_A_16_net_0;
assign MCU_A_7_net_0   = MCU_A_7;
assign SRAM_A_7        = MCU_A_7_net_0;
assign SRAM_A_15_net_0 = MCU_A_15;
assign SRAM_A_15       = SRAM_A_15_net_0;
assign MCU_A_6_net_0   = MCU_A_6;
assign SRAM_A_6        = MCU_A_6_net_0;
assign SRAM_A_14_net_0 = MCU_A_14;
assign SRAM_A_14       = SRAM_A_14_net_0;
assign MCU_A_5_net_0   = MCU_A_5;
assign SRAM_A_5        = MCU_A_5_net_0;
assign MCU_A_4_net_0   = MCU_A_4;
assign SRAM_A_4        = MCU_A_4_net_0;
assign MCU_A_13_net_0  = MCU_A_13;
assign SRAM_A_13       = MCU_A_13_net_0;
assign MCU_A_3_net_0   = MCU_A_3;
assign SRAM_A_3        = MCU_A_3_net_0;
assign MCU_A_12_net_0  = MCU_A_12;
assign SRAM_A_12       = MCU_A_12_net_0;
assign MCU_A_2_net_0   = MCU_A_2;
assign SRAM_A_2        = MCU_A_2_net_0;
assign MCU_A_11_net_0  = MCU_A_11;
assign SRAM_A_11       = MCU_A_11_net_0;
assign MCU_A_1_net_0   = MCU_A_1;
assign SRAM_A_1        = MCU_A_1_net_0;
assign MCU_A_10_net_0  = MCU_A_10;
assign SRAM_A_10       = MCU_A_10_net_0;
assign MCU_A_0_net_0   = MCU_A_0;
assign SRAM_A_0        = MCU_A_0_net_0;
assign MCU_WE_net_0    = MCU_WE;
assign SRAM_WE         = MCU_WE_net_0;
assign MCU_OE_net_0    = MCU_OE;
assign SRAM_OE         = MCU_OE_net_0;
assign SRAM_LB_net_0   = MCU_LB;
assign SRAM_LB         = SRAM_LB_net_0;
assign SRAM_UB_net_0   = MCU_UB;
assign SRAM_UB         = SRAM_UB_net_0;
assign MCU_A_20_net_0  = MCU_A_20;
assign SRAM_A_20       = MCU_A_20_net_0;
assign MCU_CS_net_0    = MCU_CS;
assign SRAM_CS1        = MCU_CS_net_0;
assign MCU_CS_net_1    = MCU_CS;
assign SRAM_CS2        = MCU_CS_net_1;
assign LED1_net_0      = ecc_sel1;
assign LED1            = LED1_net_0;
assign ecc_sel0_net_0  = ecc_sel0;
assign LED2            = ecc_sel0_net_0;
//--------------------------------------------------------------------
// Concatenation assignments
//--------------------------------------------------------------------
assign chip_sel_net_0 = { MCU_CS , MCU_CS };
assign ecc_sel_net_0  = { ecc_sel1 , ecc_sel0 };
//--------------------------------------------------------------------
// Component instances
//--------------------------------------------------------------------
//--------fpga_top_design
fpga_top_design fpga_top_design_0(
        // Inputs
        .write_en         ( MCU_WE ),
        .chip_sel         ( chip_sel_net_0 ),
        .ecc_sel          ( ecc_sel_net_0 ),
        // Inouts
        .mcu_fpga_io      ( mcu_fpga_io ),
        .fpga_mem_io_up   ( mcu_mem_io_up ),
        .fpga_mem_io_down ( mcu_mem_io_down ) 
        );


endmodule
