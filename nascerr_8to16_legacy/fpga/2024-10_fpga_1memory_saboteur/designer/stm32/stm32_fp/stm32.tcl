open_project -project {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\designer\stm32\stm32_fp\stm32.pro}\
         -connect_programmers {FALSE}
load_programming_data \
    -name {M2S005} \
    -fpga {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\designer\stm32\stm32.map} \
    -header {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\designer\stm32\stm32.hdr} \
    -spm {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\designer\stm32\stm32.spm} \
    -dca {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\designer\stm32\stm32.dca}
export_single_ppd \
    -name {M2S005} \
    -file {C:\Users\Lucas\OneDrive\Documentos\Projeto Nascerr\FPGA_project-main\stm32_test_memory - Copy\designer\stm32\stm32.ppd}

save_project
close_project
