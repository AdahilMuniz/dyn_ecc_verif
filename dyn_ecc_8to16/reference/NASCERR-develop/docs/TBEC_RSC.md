# TBEC RSC 8bits RTL implementation
The document aims to describe the algorithm used in RTL to encode 8bits of memory in TBEC RSC.

# Encode
`data_in` is the 8bits input of the algorithm and `data_out` is the 16bits output of the algorithm. The code of this implementation can be found at [rtl/tbec_rsc_encoder.sv](../rtl/tbec_rsc_encoder.sv).

data_in = {b<sub>7</sub>, b<sub>6</sub>, b<sub>5</sub>, b<sub>4</sub>, b<sub>3</sub>, b<sub>2</sub>, b<sub>1</sub>, b<sub>0</sub>}

Di<sub>0</sub> = b<sub>0</sub> ⊕ b<sub>2</sub> ⊕ b<sub>5</sub> ⊕ b<sub>7</sub>
Di<sub>1</sub> = b<sub>1</sub> ⊕ b<sub>3</sub> ⊕ b<sub>4</sub> ⊕ b<sub>6</sub>

P<sub>0</sub> = b<sub>0</sub> ⊕ b<sub>1</sub> ⊕ b<sub>4</sub> ⊕ b<sub>5</sub>
P<sub>1</sub> = b<sub>2</sub> ⊕ b<sub>3</sub> ⊕ b<sub>6</sub> ⊕ b<sub>7</sub>

Cb<sub>0,0</sub> = b<sub>0</sub> ⊕ b<sub>2</sub>
Cb<sub>0,1</sub> = b<sub>4</sub> ⊕ b<sub>6</sub>
Cb<sub>1,0</sub> = b<sub>1</sub> ⊕ b<sub>3</sub>
Cb<sub>1,1</sub> = b<sub>5</sub> ⊕ b<sub>7</sub>

data_out = {Cb<sub>1,1</sub>, Cb<sub>1,0</sub>, Cb<sub>0,1</sub>, Cb<sub>0,0</sub>, P<sub>1</sub>, P<sub>0</sub>, Di<sub>1</sub>, Di<sub>0</sub>, b<sub>7</sub>, b<sub>6</sub>, b<sub>5</sub>, b<sub>4</sub>, b<sub>3</sub>, b<sub>2</sub>, b<sub>1</sub>, b<sub>0</sub>}

# Decode
`data_in` is the 16bits input of the algorithm and `data_out` is the 8bits output of the algorithm. The code of this implementation can be found at [rtl/tbec_rsc_decoder](../rtl/tbec_rsc_decoder.sv).

data_in = {red<sub>7</sub>, red<sub>6</sub>, red<sub>5</sub>, red<sub>4</sub>, red<sub>3</sub>, red<sub>2</sub>, red<sub>1</sub>, red<sub>0</sub>, b<sub>7</sub>, b<sub>6</sub>, b<sub>5</sub>, b<sub>4</sub>, b<sub>3</sub>, b<sub>2</sub>, b<sub>1</sub>, b<sub>0</sub>}

SDi<sub>0</sub> = b<sub>0</sub> ⊕ b<sub>2</sub> ⊕ b<sub>5</sub> ⊕ b<sub>7</sub> ⊕ red<sub>0</sub>
SDi<sub>1</sub> = b<sub>1</sub> ⊕ b<sub>3</sub> ⊕ b<sub>4</sub> ⊕ b<sub>6</sub> ⊕ red<sub>1</sub>

SP<sub>0</sub> = b<sub>0</sub> ⊕ b<sub>1</sub> ⊕ b<sub>4</sub> ⊕ b<sub>5</sub> ⊕ red<sub>2</sub>
SP<sub>1</sub> = b<sub>2</sub> ⊕ b<sub>3</sub> ⊕ b<sub>6</sub> ⊕ b<sub>7</sub> ⊕ red<sub>3</sub>

linsin<sub>0,0</sub> = b<sub>0</sub> ⊕ b<sub>2</sub> ⊕ red<sub>4</sub>
linsin<sub>0,1</sub> = b<sub>4</sub> ⊕ b<sub>6</sub> ⊕ red<sub>5</sub>
linsin<sub>1,0</sub> = b<sub>1</sub> ⊕ b<sub>3</sub> ⊕ red<sub>6</sub>
linsin<sub>1,1</sub> = b<sub>5</sub> ⊕ b<sub>7</sub> ⊕ red<sub>7</sub>

sum_SP = SP<sub>0</sub> + SP<sub>1</sub>
sum_SDi = SDi<sub>0</sub> + SDi<sub>1</sub>
sum_SL = linsin<sub>0,0</sub> + linsin<sub>0,1</sub> + linsin<sub>1,0</sub> + linsin<sub>1,1</sub>

The error correction algorithm is activated as described in the code:
```sv
if (((SP != '0 && SDi != '0) || sumSL > 1) && !(sumSDi == 0 && sumSP == 1 && sumSL >= 2)) begin
      if (SP[0] > SP[1]) begin
        for (int i = 0; i < 2; i++) begin
          if (linsin[i] != '0) begin
            signal[i] ^= linsin[i][0];
            signal[i+4] ^= linsin[i][1];
          end
        end
      end else if (SP[0] < SP[1]) begin
        for (int i = 2; i < 4; i++) begin
          if (linsin[i-2] != '0) begin
            signal[i] ^= linsin[i-2][0];
            signal[i+4] ^= linsin[i-2][1];
          end
        end
      end else if (SP[0] == SP[1] && {SDi, SP} != '0) begin
        for (int i = 1; i < 3; i++) begin
          if (linsin[i-1] != '0) begin
            signal[i] ^= linsin[i-1][1];
            signal[i+4] ^= linsin[i-1][0];
          end
        end
      end
    end
  end
```

data_out = signal
