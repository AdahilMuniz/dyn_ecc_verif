package typedefs;
  typedef enum logic [1:0] {
    BYPASS_UP = 2'b00,
    BYPASS_DOWN = 2'b01,
    TBEC_RSC = 2'b10,
    MRSC = 2'b11
  } sel_t;
endpackage
