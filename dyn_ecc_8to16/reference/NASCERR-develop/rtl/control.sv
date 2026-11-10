module control
  import typedefs::*;
(
  input  sel_t       sel,
  input  logic       read_en,
  write_en,
  input  logic [7:0] cpu_data_in,
  output logic [7:0] cpu_data_out,
  input  logic [7:0] ram_up_data_in,
  output logic [7:0] ram_up_data_out,
  input  logic [7:0] ram_down_data_in,
  output logic [7:0] ram_down_data_out,
  output logic       cpu_oe,
  ram_up_oe,
  ram_down_oe
);
  logic [15:0] tbec_rsc_encoder_output;
  logic [ 7:0] tbec_rsc_decoder_output;

  tbec_rsc_encoder encoder0 (
    .data_in(cpu_data_in),
    .data_out(tbec_rsc_encoder_output)
  );
  tbec_rsc_decoder decoder0 (
    .data_in({ram_up_data_in, ram_down_data_in}),
    .data_out(tbec_rsc_decoder_output)
  );

  always_comb begin
    cpu_data_out      = '0;
    ram_up_data_out   = '0;
    ram_down_data_out = '0;

    cpu_oe            = 0;
    ram_up_oe         = 0;
    ram_down_oe       = 0;

    case (sel)
      BYPASS_UP: begin
        if (read_en && !write_en) begin
          cpu_data_out = ram_up_data_in;
          cpu_oe       = 1;
        end
        if (!read_en && write_en) begin
          ram_up_data_out = cpu_data_in;
          ram_up_oe       = 1;
        end
      end
      BYPASS_DOWN: begin
        if (read_en && !write_en) begin
          cpu_data_out = ram_down_data_in;
          cpu_oe       = 1;
        end
        if (!read_en && write_en) begin
          ram_down_data_out = cpu_data_in;
          ram_down_oe       = 1;
        end
      end
      TBEC_RSC: begin
        if (read_en && !write_en) begin
          cpu_data_out = tbec_rsc_decoder_output;
          cpu_oe       = 1;
        end
        if (!read_en && write_en) begin
          {ram_up_data_out, ram_down_data_out} = tbec_rsc_encoder_output;
          ram_up_oe                            = 1;
          ram_down_oe                          = 1;
        end
      end
      MRSC: begin
        // Todo
      end
    endcase
  end
endmodule

