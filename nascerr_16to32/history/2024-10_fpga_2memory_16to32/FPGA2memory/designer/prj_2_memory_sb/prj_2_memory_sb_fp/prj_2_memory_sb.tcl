open_project -project {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\designer\prj_2_memory_sb\prj_2_memory_sb_fp\prj_2_memory_sb.pro}
enable_device -name {M2S025T} -enable 1
set_programming_file -name {M2S025T} -file {C:\Users\Lesc\Documents\LiberoProjects\prj_FPGA_2memory\FPGA2memory\designer\prj_2_memory_sb\prj_2_memory_sb.ppd}
set_programming_action -action {PROGRAM} -name {M2S025T} 
run_selected_actions
save_project
close_project
