# NASCERR Image Experiment — Implementation Plan

**Project:** NASCERR CubeSat — SEU Characterization via Image Processing  
**Branch:** `dev_image_experiment`  
**Date:** 2026-04-29  
**Status:** Planning

---

## 1. Executive Summary

The image experiment writes a known test image from MCU internal flash into the two SRAM chips (via the FPGA ECC subsystem), reads the data back after a configurable delay or radiation exposure, and evaluates degradation through standard image quality metrics (PSNR, SSIM) and memory reliability metrics (BER, corrected/uncorrectable error counts). Each run produces a paired result: one under the selected ECC mode and one under Dual Bypass (no ECC), so the benefit of error correction is directly measurable. The methodology is identical for local saboteur validation and the SEU radiation lab — only the error source changes.

---

## 2. System Context

### Hardware topology

```
PC / Raspberry Pi
    │  USB (FT2232H Dual VCP)
    │  Channel A → MCU UART4 (PC10/PA1, 2 Mbps)
    │  Channel B → FPGA UART
    ↓
STM32L4S5 (MCU)
    ├── Internal Flash  ← source images (const arrays, barbara_16/48/96/192)
    ├── External NAND Flash (W25N01G via SPI3)  ← experiment result metadata
    ├── I2C slave  ← primary command interface (Python smbus2 → /dev/i2c-X)
    └── FMC → FPGA (M2S025)
                    ↓ ECC encode/decode on every SRAM word
              SRAM_LOW (U201, IS61WV204816BLL, 2M×16-bit)
              SRAM_HIGH (U200, IS61WV204816BLL, 2M×16-bit)
              Combined: 32-bit data bus, 2 MB addressable
```

### ECC modes (FPGA-implemented)

| Mode | SRAM_LOW | SRAM_HIGH | Description |
|---|---|---|---|
| `BYPASS_1` | data | — | No ECC, memory 1 only |
| `BYPASS_2` | — | data | No ECC, memory 2 only |
| `DUAL_BYPASS` | data | data | Both memories, no ECC |
| `TBEC` | data | syndrome | Triple-Bit Error Correction |
| `MRSC` | data | syndrome | Multi-Row Single-Column |

For this experiment: **any one ECC mode** is paired with **DUAL_BYPASS** in the same experiment run so results are directly comparable.

### Existing firmware modules used

- `NASCERR_ImageProcessing.h/.c` — image serialization, RLE codec, PSNR, SSIM, SRAM write/read
- `NASCERR_Memory_32bits.h/.c` — `nascerr_memory_write_image_buffer()`, block read/write
- `NASCERR_Scheduler.h/.c` — task orchestration, command queue
- `NASCERR_Flash.h/.c` + `flash_w25n01g.h/.c` — result metadata persistence
- `NASCERR_DB.h/.c` — database header, experiment page management
- `uart_handler.h/.c` — UART packet layer (available, not yet in command loop)
- `nascerr_protocol.py` + `nascerr_command.py` — Python command/response layer

---

## 3. Experiment Design

### 3.1 Source data

Test images live as `const uint8_t` arrays in MCU internal flash (`NASCERR_ImageProcessing.c`):

| Image | Size | Pixels | Bytes (16-bit words) |
|---|---|---|---|
| `barbara_16` | 16×16 | 256 | 512 B |
| `barbara_48` | 48×48 | 2,304 | 4,608 B |
| `barbara_96` | 96×96 | 9,216 | 18,432 B |
| `barbara_192` | 192×192 | 36,864 | 73,728 B |

`barbara_96` and `barbara_192` const arrays need to be added (placeholders already exist in the header).

The MCU reads these directly from its own flash via the const pointer — no external flash read is required for the source image. External NAND flash is used exclusively for storing experiment results and metadata.

### 3.2 Pre-processing (write side)

Two options, selectable per command:

| Mode | Description | Trade-off |
|---|---|---|
| `IMG_LOSSLESS` | No compression, raw pixels | Exact byte-for-byte comparison on read-back |
| `IMG_LOSS_FF` | RLE compression | Smaller SRAM footprint per tile; exposes RLE robustness to SEU |

For SEU characterization, **`IMG_LOSSLESS` is the primary mode** — direct pixel comparison is unambiguous. RLE can be run as a secondary experiment to observe whether bit errors in the compressed stream cause cascade degradation during decompression.

### 3.3 SRAM filling strategy

The SRAM is 2 MB (1,048,576 × 16-bit words), divided into 64 blocks of 32,768 words each.

The image is tiled across SRAM until a configured number of blocks is filled:

```
for block_idx in 0 .. N_blocks:
    offset = block_idx × BLOCK_SIZE_WORDS
    tile the image repeatedly until the block is full
    nascerr_memory_write_image_buffer(ecc_mode, offset, buffer, BLOCK_SIZE_WORDS)
```

This means a 48×48 image (2,304 pixels) tiles ≈14 times to fill one block. For full SRAM coverage (64 blocks), the image is written ≈896 times total, maximizing the number of memory cells at risk during radiation exposure.

**Configurable parameters:**
- `n_blocks` (1–64): how many SRAM blocks to fill
- `ecc_mode`: ECC mode for this half of the experiment
- `compression`: RLE or none

### 3.4 ECC run strategy

Each experiment command triggers **two sequential SRAM runs**:

```
Run A: selected ECC (TBEC / MRSC / BYPASS_1 / BYPASS_2)
    → write image to N_blocks with ECC mode
    → wait (delay_ms or external trigger for radiation exposure)
    → read back, compare, collect metrics
    → store result_A

Run B: DUAL_BYPASS (no ECC, reference)
    → write image to N_blocks
    → wait (same delay_ms)
    → read back, compare, collect metrics
    → store result_B
```

Run B provides the unprotected baseline. The delta between A and B quantifies ECC effectiveness under the same conditions.

If the command specifies `ecc_mode = DUAL_BYPASS`, Run A and Run B are identical (both bypass). This is useful for baseline calibration.

### 3.5 Read-back and post-processing

After the delay/exposure window:

1. Read back all written blocks from SRAM (`nascerr_memory_read_block()`)
2. If RLE was used, decompress (`img_rle_decompress()`)
3. Reconstruct 2D image from linear block vector (`img_reconstruct_from_blocks()`)
4. Compare pixel-by-pixel with original source image

**Metrics computed per run:**

| Metric | Description | Relevance |
|---|---|---|
| `n_errors` | Total pixels that differ from original | Primary SEU count |
| `error_rate` | `n_errors / total_pixels` | Normalized rate |
| `n_bits_flipped` | Hamming distance at bit level | BER numerator |
| `BER` | `n_bits_flipped / total_bits` | Bit Error Rate |
| `n_corrected` | Errors corrected by FPGA ECC | ECC effectiveness |
| `n_uncorrectable` | Errors that passed through ECC | Residual damage |
| `MSE` | Mean Squared Error of pixel values | Distortion magnitude |
| `PSNR` | Peak Signal-to-Noise Ratio (dB) | Image quality |
| `SSIM` | Structural Similarity Index (0–1) | Perceptual quality |
| `affected_blocks` | Bitmask of SRAM blocks with errors | Spatial distribution |

All metrics computed on-chip. The MCU has sufficient compute for the floating-point PSNR/SSIM for images up to 192×192.

### 3.6 Result data structure

A new `NASCERR_IMAGE_RESULT` struct (to be defined alongside `NASCERR_EXPERIMENT`):

```c
typedef struct {
    id_db        id;               // Experiment ID
    uint32_t     timestamp;        // FreeRTOS tick at end of read-back
    NASCERR_MODE ecc_mode;         // ECC mode used
    ImgSelect_t  image;            // Which test image
    ImgCompression_t compression;  // Compression mode used
    uint8_t      n_blocks;         // Number of SRAM blocks filled
    uint32_t     n_errors;         // Pixel-level error count
    uint32_t     n_bits_flipped;   // Bit-level error count
    uint32_t     n_corrected;      // ECC-corrected errors
    uint32_t     n_uncorrectable;  // Errors that passed through ECC
    float        BER;              // Bit Error Rate
    float        PSNR;             // dB
    float        SSIM;             // 0.0–1.0
    float        MSE;
    uint64_t     affected_blocks;  // 64-bit bitmask
    uint16_t     crc;              // CRC16 of all above
} NASCERR_IMAGE_RESULT;            // ~64 bytes
```

Two of these (Run A + Run B) are stored in external NAND flash and also buffered for transmission via the `TXB` register.

---

## 4. Firmware Implementation

### 4.1 New command type

Add to `CommandType_t` enum in `NASCERR_Scheduler.h`:

```c
CMD_IMAGE_EXPERIMENT = 6,   // Trigger image experiment
CMD_IMAGE_RESULT_REQUEST = 7, // Request image result page download
```

And a new command payload struct (`NASCERR_IMAGE_CMD` already in `NASCERR_ImageProcessing.h`):

```c
typedef struct {
    id_db            id;          // Experiment ID assigned by host
    NASCERR_MODE     ecc;         // ECC mode for Run A
    nascerr_rpt      repeat;      // Number of full experiment cycles
    ImgSelect_t      image;       // Image to use
    ImgCompression_t comp;        // Compression mode
    uint8_t          n_blocks;    // SRAM blocks to fill (1–64)
    uint32_t         delay_ms;    // Wait time between write and read-back
} NASCERR_IMAGE_CMD;
```

### 4.2 Task flow

The existing FreeRTOS task structure is reused:

```
masterCommTask
    receives CMD_IMAGE_EXPERIMENT via I2C protocol
    validates and enqueues to commandQ

nascerrWriteExp (expWriteTask)
    dequeues CMD_IMAGE_EXPERIMENT
    → calls nascerr_image_run_experiment_write(cmd)
        - loads image from const array
        - optional RLE compress
        - fills N_blocks of SRAM with ECC mode (Run A)
        - fills N_blocks of SRAM with DUAL_BYPASS (Run B)
    → sets FLAG_EXP_READ

nascerrReadExp (expReadTask)
    triggered by FLAG_EXP_READ
    → waits delay_ms
    → calls nascerr_image_run_experiment_read(cmd)
        - reads back N_blocks (Run A) → metrics A
        - reads back N_blocks (Run B) → metrics B
        - packs two NASCERR_IMAGE_RESULT structs
        - stores in NAND flash (nascerr_db_addExperiment)
        - loads TXB buffer with result A + result B
        - sets EXP_STAT = EXPERIMENT_DONE
```

### 4.3 Splitting write and read phases

The delay between write and read is the radiation exposure window. For the radiation lab, this window is not a software timer — it is triggered externally (the beam can be turned on/off). The firmware must therefore support:

- **Timer mode**: `delay_ms > 0` → OS delay, then automatic read-back
- **External trigger mode**: `delay_ms = 0` → write phase completes, EXP_STAT signals WRITE_DONE, host sends separate `CMD_IMAGE_RESULT_REQUEST` to start read-back

External trigger mode decouples write from read, giving the radiation lab operator control of the exposure window. Both modes use the same read-back and metrics logic.

### 4.4 Result transmission

Result size: 2 × `NASCERR_IMAGE_RESULT` ≈ 128 bytes → fits in a single `TXB` read (2066 bytes max).

For full reconstructed image data (needed for visualization):
- Add an `image_data[]` field to the result, or use a separate command
- Image is chunked across multiple TXB reads (page index in command payload)
- `barbara_192` = 36,864 bytes → ~18 TXB reads of 2,048 bytes each

Phase 1: transmit only the metrics struct (sufficient for radiation lab characterization).  
Phase 2: add full image data download for visualization on the PC.

---

## 5. Communication Protocol

### 5.1 Command (host → STM32)

Using the existing `NASCERR_CTRL` wrapper over I2C register `CMD` (0x04):

```
[2B size][1B cmd_type=CMD_IMAGE_EXPERIMENT][NASCERR_IMAGE_CMD payload][2B CRC16]
```

### 5.2 Status polling (host reads)

Host polls `EXP_STAT` register (0x13, 18 bytes) until `req_exp = REQ_EXP_TX_RESULT`.

States during experiment:
```
WRITE_STARTED  →  WRITE_DONE  →  (exposure window)  →  READ_STARTED  →  READ_DONE  →  TX_RESULT_READY
```

### 5.3 Result retrieval (host reads TXB)

Host reads `TXB` register (0x03). Response contains:
```
[NASCERR_IMAGE_RESULT run_A][NASCERR_IMAGE_RESULT run_B]
```
Host ACKs with `ACK` register (0x0D) as usual.

### 5.4 Image data download (Phase 2)

New sub-command or separate `CMD_IMAGE_RESULT_REQUEST` with `page_idx` field. Host iterates pages, each TXB read returns 2,048 bytes of image data until all pixels are transferred.

---

## 6. Python Host Application

### 6.1 Experiment controller (`nascerr_image_experiment.py`)

```python
class ImageExperiment:
    def send_command(image, ecc_mode, n_blocks, delay_ms, compression)
    def wait_for_write_done()        # poll EXP_STAT
    def trigger_read_back()          # for external trigger mode
    def wait_for_results()           # poll EXP_STAT
    def read_results() -> (ResultA, ResultB)
    def download_image_data(run) -> np.ndarray   # Phase 2
    def run_full_experiment(...)     # convenience wrapper
```

### 6.2 Metrics display

Console table after each run:

```
┌─────────────────┬──────────────┬──────────────┐
│ Metric          │ ECC (TBEC)   │ BYPASS       │
├─────────────────┼──────────────┼──────────────┤
│ PSNR (dB)       │ 42.1         │ 28.7         │
│ SSIM            │ 0.9923       │ 0.7841       │
│ BER             │ 1.2e-6       │ 3.8e-5       │
│ Errors          │ 12           │ 389          │
│ Corrected       │ 11           │ 0            │
│ Uncorrectable   │ 1            │ 389          │
│ Affected blocks │ 0x0000000C   │ 0x3FFFFFFFF  │
└─────────────────┴──────────────┴──────────────┘
```

### 6.3 Image visualization (Phase 2)

After downloading pixel data:

```
┌─────────────┬─────────────┬─────────────┐
│  ORIGINAL   │ TBEC result │ BYPASS ref  │
│  (source)   │ PSNR=42.1   │ PSNR=28.7   │
│   [image]   │   [image]   │   [image]   │
└─────────────┴─────────────┴─────────────┘
```

Implemented with `matplotlib` or `PIL`. Saved as PNG for lab reports.

### 6.4 Data logging

Every experiment run is saved to a JSON/CSV file:

```json
{
  "timestamp": "2026-05-15T14:32:10",
  "image": "barbara_48",
  "n_blocks": 16,
  "ecc_mode": "TBEC",
  "compression": "lossless",
  "environment": "radiation_lab",
  "run_A": { "PSNR": 42.1, "SSIM": 0.9923, "BER": 1.2e-6, ... },
  "run_B": { "PSNR": 28.7, "SSIM": 0.7841, "BER": 3.8e-5, ... }
}
```

---

## 7. Test Methodology

### 7.1 Local validation — saboteur mode

The STM32 firmware has a built-in error injector (`ERROR_ENABLE` compile-time macro in `NASCERR_Memory_32bits.c`). When enabled, it flips a configured bit every `ERROR_STRIDE_WORDS` words during SRAM writes, simulating SEU.

**Validation procedure:**
1. Flash firmware with `ERROR_ENABLE=1`, `ERROR_BIT_TO_FLIP=1`, `ERROR_STRIDE_WORDS=1024`
2. Run experiment on `barbara_48`, TBEC mode, 16 blocks, no compression
3. Verify: `n_corrected > 0`, PSNR(TBEC) > PSNR(BYPASS), image visually intact
4. Increase error density (`ERROR_STRIDE_WORDS=128`) until TBEC saturates
5. Verify BYPASS shows more errors than TBEC at the same density
6. Disable saboteur (`ERROR_ENABLE=0`), verify clean run: `n_errors=0`, `PSNR=∞`

This validates the full pipeline (write → ECC → read → metrics → transmission) before going to the radiation lab.

### 7.2 Radiation lab — SEU environment

**Pre-test preparation:**
- Flash firmware with `ERROR_ENABLE=0` (no artificial errors)
- Calibrate: run one clean baseline experiment before beam on
- Confirm `n_errors=0` in baseline

**During irradiation:**
- Use external trigger mode (`delay_ms=0`)
- Operator executes write phase → confirms WRITE_DONE on host
- Beam is turned on for configured fluence/time
- Beam is turned off → operator triggers read-back from host
- Results logged automatically

**Experiment matrix (example):**

| Run | Image | ECC | Blocks | Repetitions |
|---|---|---|---|---|
| 1 | barbara_48 | TBEC | 16 | 3 |
| 2 | barbara_48 | MRSC | 16 | 3 |
| 3 | barbara_192 | TBEC | 64 | 1 |
| 4 | barbara_192 | MRSC | 64 | 1 |

Each run automatically produces the paired BYPASS comparison.

**Repeatability:** The `repeat` field in `NASCERR_IMAGE_CMD` runs the write+read+analyze cycle N times consecutively. Statistics are averaged across repetitions for the final report.

---

## 8. Implementation Roadmap

### Phase 1 — Core experiment loop (firmware)

- [ ] Add `barbara_96` and `barbara_192` const arrays to `NASCERR_ImageProcessing.c`
- [ ] Implement N-block SRAM fill loop (tile image across N blocks)
- [ ] Implement read-back loop across N blocks
- [ ] Add `NASCERR_IMAGE_RESULT` struct
- [ ] Split `nascerr_image_run_experiment()` into write and read phases
- [ ] Wire `CMD_IMAGE_EXPERIMENT` into scheduler command decode path
- [ ] Implement dual-run logic (ECC + BYPASS companion)
- [ ] Store results in NAND flash via existing DB layer
- [ ] Load metrics into TXB buffer for host retrieval

### Phase 2 — Protocol and host (Python)

- [ ] Add `NASCERR_IMAGE_CMD` serialization to `nascerr_command.py`
- [ ] Add `CMD_IMAGE_EXPERIMENT` to command dispatch in `nascerr_protocol.py`
- [ ] Add `EXP_STAT` state polling with experiment-specific states
- [ ] Implement `NASCERR_IMAGE_RESULT` deserialization
- [ ] Write `nascerr_image_experiment.py` controller class
- [ ] Console metrics table display
- [ ] JSON result logging

### Phase 3 — External trigger mode (radiation lab)

- [ ] Implement `delay_ms=0` external trigger path in firmware
- [ ] Add `CMD_IMAGE_RESULT_REQUEST` to trigger read-back phase
- [ ] Add EXP_STAT states: `WRITE_DONE` and `READ_DONE`
- [ ] Test external trigger flow locally with saboteur

### Phase 4 — Full image download and visualization

- [ ] Add chunked image data download to firmware (page-indexed TXB reads)
- [ ] Add `download_image_data()` to Python controller
- [ ] Implement `matplotlib` side-by-side visualization
- [ ] Add PNG export for lab reports

### Phase 5 — Extended image set and data analysis

- [ ] Add other standard test images if needed (Lena, cameraman, etc.)
- [ ] Statistical analysis across multiple repetitions (confidence intervals)
- [ ] Automated report generation (PDF or LaTeX)

---

## 9. Key Design Decisions and Rationale

| Decision | Choice | Reason |
|---|---|---|
| Source image location | MCU internal flash (const arrays) | Already implemented, zero read latency, no external flash complexity |
| SRAM filling | Tile image across N blocks | Maximizes memory cells at risk; N is configurable per test |
| ECC pairing strategy | Selected ECC + automatic BYPASS companion | Direct same-conditions comparison, single command per experiment |
| Write/read separation | Timer mode + external trigger mode | Timer for local tests, external trigger for radiation lab |
| Result transmission | Metrics-only (Phase 1), full image data (Phase 2) | Metrics fit in one TXB read; image data needs chunking |
| Error injection | Firmware saboteur (`ERROR_ENABLE` macro) | Validates full pipeline without radiation equipment |
| Compression default | `IMG_LOSSLESS` (no compression) | Unambiguous pixel comparison; RLE is a secondary experiment |

---

*Document version: 1.0 — Initial planning*  
*Authors: Danilo Alencar + Claude Sonnet 4.6*
