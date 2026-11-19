new_project \
         -name {prj_2_memory_sb} \
         -location {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\designer\prj_2_memory_sb\prj_2_memory_sb_fp} \
         -mode {chain} \
         -connect_programmers {FALSE}
add_actel_device \
         -device {M2S025T} \
         -name {M2S025T}
enable_device \
         -name {M2S025T} \
         -enable {TRUE}
save_project
close_project
