alias f := format
alias l := lint
alias t := test

format:
  verible-verilog-format --column_limit=140 --indentation_spaces=2 --compact_indexing_and_selections=true \
  --port_declarations_alignment=align --port_declarations_indentation=indent --module_net_variable_alignment=align \
  --named_port_alignment=flush-left --named_port_indentation=indent --assignment_statement_alignment=align \
  --case_items_alignment=flush-left --inplace {rtl,tb}/*.sv

lint:
  verilator --lint-only --language 1800-2017 -Wall -Irtl -sv -DMY_DEFINE -F rtlfiles.f --top-module top

test:
  verilator --binary --language 1800-2017 --timing -Wall --top-module memory_tb rtl/*.sv tb/memory_tb.sv

