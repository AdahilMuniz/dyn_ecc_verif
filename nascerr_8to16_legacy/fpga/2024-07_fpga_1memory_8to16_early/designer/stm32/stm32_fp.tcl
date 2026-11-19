new_project \
         -name {stm32} \
         -location {C:\Users\Lesc\Documents\LiberoProjects\stm32_test_memory\designer\stm32\stm32_fp} \
         -mode {chain} \
         -connect_programmers {FALSE}
add_actel_device \
         -device {M2S005} \
         -name {M2S005}
enable_device \
         -name {M2S005} \
         -enable {TRUE}
save_project
close_project
