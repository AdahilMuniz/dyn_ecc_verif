open_project -project {C:\Users\Elena\Documents\Elena\stm32_test_memory - Copy\designer\stm32\stm32_fp\stm32.pro}
enable_device -name {M2S005} -enable 1
set_programming_file -name {M2S005} -file {C:\Users\Elena\Documents\Elena\stm32_test_memory - Copy\designer\stm32\stm32.ppd}
set_programming_action -action {PROGRAM} -name {M2S005} 
run_selected_actions
save_project
close_project
