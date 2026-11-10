set_component prj_2_memory_sb_sb_MSS
# Microchip Technology Inc.
# Date: 2024-Oct-15 12:12:48
#

create_clock -period 40 [ get_pins { MSS_ADLIB_INST/CLK_CONFIG_APB } ]
set_false_path -ignore_errors -through [ get_pins { MSS_ADLIB_INST/CONFIG_PRESET_N } ]
