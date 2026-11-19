# Written by Synplify Pro version map202309act, Build 044R. Synopsys Run ID: sid1729023818 
# Top Level Design Parameters 

# Clocks 
create_clock -period 10.000 -waveform {0.000 5.000} -name {ecc_design|data_out_right_up7_inferred_clock} [get_pins {fpga_top_design_0/modulo/data_out_right_up7/Y}] 
create_clock -period 10.000 -waveform {0.000 5.000} -name {ecc_design|data_out_right_up8_inferred_clock} [get_pins {fpga_top_design_0/modulo/data_out_right_up8/Y}] 

# Virtual Clocks 

# Generated Clocks 

# Paths Between Clocks 

# Multicycle Constraints 

# Point-to-point Delay Constraints 

# False Path Constraints 

# Output Load Constraints 

# Driving Cell Constraints 

# Input Delay Constraints 

# Output Delay Constraints 

# Wire Loads 

# Other Constraints 

# syn_hier Attributes 

# set_case Attributes 

# Clock Delay Constraints 
set Inferred_clkgroup_0 [list ecc_design|data_out_right_up7_inferred_clock]
set Inferred_clkgroup_1 [list ecc_design|data_out_right_up8_inferred_clock]
set_clock_groups -asynchronous -group $Inferred_clkgroup_0
set_clock_groups -asynchronous -group $Inferred_clkgroup_1

set_clock_groups -asynchronous -group [get_clocks {ecc_design|data_out_right_up7_inferred_clock}]
set_clock_groups -asynchronous -group [get_clocks {ecc_design|data_out_right_up8_inferred_clock}]

# syn_mode Attributes 

# Cells 

# Port DRC Rules 

# Input Transition Constraints 

# Unused constraints (intentionally commented out) 


# Non-forward-annotatable constraints (intentionally commented out) 

# Block Path constraints 

