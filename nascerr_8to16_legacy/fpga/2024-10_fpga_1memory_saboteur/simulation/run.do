quietly set ACTELLIBNAME SmartFusion2
quietly set PROJECT_DIR "C:/Users/Lesc/Desktop/ECC/Libero/stm32_test_memory - Copy"

if {[file exists postsynth/_info]} {
   echo "INFO: Simulation library postsynth already exists"
} else {
   file delete -force postsynth 
   vlib postsynth
}
vmap postsynth postsynth
vmap SmartFusion2 "C:/Microchip/Libero_SoC_v2024.1/Designer/lib/modelsimpro/precompiled/vlog/smartfusion2"

vlog -sv -work postsynth "${PROJECT_DIR}/synthesis/stm32.v"

vsim -L SmartFusion2 -L postsynth  -t 1fs postsynth.stm32
# The following lines are commented because no testbench is associated with the project
# add wave /testbench/*
# run 1000ns
