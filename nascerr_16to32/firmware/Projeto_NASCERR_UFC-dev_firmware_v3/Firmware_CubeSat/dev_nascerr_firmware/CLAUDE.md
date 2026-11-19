# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

This is an STM32CubeIDE (Eclipse/CDT) project targeting the **STM32L4S5QI** (Cortex-M4F). The makefile is auto-generated under `Debug/`.

```bash
# Build from the project root
make -C Debug all

# Clean
make -C Debug clean

# Flash via OpenOCD (with ST-Link connected)
make -C Debug flash
```

Alternatively, open the project in STM32CubeIDE and use the standard Build/Run buttons. The `.ioc` file (`dev_nascerr_firmware.ioc`) regenerates HAL init code in `Core/` — do not manually edit files in `Core/Src/` between `USER CODE BEGIN` / `USER CODE END` markers unless you intend those changes to survive re-generation.

There are no automated tests. Validation is done on hardware via UART/I2C logs.

## Architecture

### Hardware context

- **MCU**: STM32L4S5QI — 2 MB internal flash (dual-bank), Cortex-M4F
- **External SRAM**: 2 MB at `0x60000000` via FMC (two 1 MB memories, accessed as 16-bit words)
- **External flash**: W25N01G NAND (1 Gbit, SPI) for experiment data, logs, and firmware images
- **Primary bus**: I2C slave — receives commands from an OBC (master board)
- **Secondary bus**: UART — used by the bootloader (firmware download) and image upload (DMA + idle-line detection)

### Module layout (`Modules/NASCERR/`)

All application logic lives here. `Core/` contains only HAL-generated peripheral init.

| Folder | Purpose |
|---|---|
| `Application/` | `NASCERR_Scheduler` — FreeRTOS task orchestrator; `NASCERR_Control` — shared type definitions and command parsing |
| `Communication/` | `NASCERR_Protocol` — I2C register protocol state machine; `NASCERR_Registers` — register map; `NASCERR_UartImage` — image upload via UART DMA |
| `Bootloader/` | OTA firmware update FSM (`NASCERR_Bootloader`), UART boot interface (`NASCERR_BootInterface`) |
| `Database/` | `NASCERR_DB` — CRUD for experiments, logs, scheduler entries, and image metadata on external flash |
| `Experiments/` | `NASCERR_Experiment` — SRAM stress-test orchestration; `NASCERR_ECCs_32bits` — ECC algorithms (BYPASS_1/2, TBEC, MRSC); `NASCERR_ImageProcessing` |
| `ExternalFlash/` | `flash_w25n01g` — low-level W25N01G driver; `NASCERR_Flash` — higher-level flash API; `flash_saboteur` — probabilistic fault injection |
| `Hardware/` | ADC-based telemetry (`NASCERR_HW_Status`), GPIO control (`NASCERR_IOs`) |
| `InternalFlash/` | Internal flash read/write/erase — used by the bootloader for OTA |
| `Sram/` | `NASCERR_Memory_32bits` — FMC-backed SRAM read/write; divides the 2 MB into 64 blocks × 32 pages |
| `System/` | `NASCERR_Log` — in-RAM log buffer (flushed to flash by `loggerTask`); `NASCERR_System` — shared enums/structs |
| `Utils/` | `NASCERR_SysTime` — millisecond tick helpers; `NASCERR_Util` — CRC16, endian helpers |

The single top-level config header is `Modules/NASCERR/NASCERR_Config.h`.

### FreeRTOS tasks (all spawned in `NASCERR_Scheduler.c`)

The single FreeRTOS thread declared in `freertos.c` is `masterCommTask` (priority HIGH, 4 KB stack). It calls `nascerr_sch_initScheduler()` once, then loops `nascerr_sch_masterCommTask()`, which drains the I2C command queue and drives the protocol state machine. `initScheduler` spawns the remaining tasks:

| Task | Priority | Role |
|---|---|---|
| `schedulerTask` | MED | Fires timed `FLAG_EXP_WRITE` / `FLAG_EXP_READ` / `FLAG_HW_CHECK` / `FLAG_MEMORY_CHECK` events |
| `expWriteTask` | IO_HIGH | Writes SRAM blocks per `NASCERR_SCHEDULER` command |
| `expReadTask` | IO_HIGH | Reads SRAM, computes ECC telemetry, stores result via `NASCERR_DB` |
| `hwCheckTask` | MED | Polls ADC telemetry (temp, current, power) |
| `bootloaderTask` | COMM | Handles OTA download FSM over UART |
| `loggerTask` | LOW | Flushes in-RAM log buffer to external flash |
| `memCheckTask` | BG | Periodic SRAM health self-check (encoder/decoder + full memory scan) |
| `imgUploadTask` | IO_HIGH | Receives firmware image chunks over UART DMA, writes to external flash |

Shared state is protected by two mutexes: `dbMutex` (external flash / DB access) and `sramMutex` (FMC SRAM access). Use the `DB_LOCK()` / `DB_UNLOCK()` and `SRAM_LOCK()` / `SRAM_UNLOCK()` macros defined in `NASCERR_Scheduler.h`.

### I2C register protocol

The master reads/writes named registers (see `NASCERR_Registers.h`). Key registers:

| Register | Address | Direction | Purpose |
|---|---|---|---|
| `STAT` | 0x01 | R | System operating status |
| `CMD` | 0x04 | W | Experiment scheduling command |
| `EXP_STAT` | 0x13 | R | Experiment counters (total / running / finished / unread) |
| `IMAGE_STAT` | 0x1B | R | Image upload state and progress |
| `BOOT_DATA` | 0x10 | W | OTA firmware chunk |
| `BOOT_STAT` | 0x11 | R | OTA boot status |
| `LOG_SUM` | 0x17 | R | Log area summary |
| `ROBUST_MEM_HEALTH` | 0x1A | R | SRAM health struct |

### Internal flash memory map

| Address | Contents |
|---|---|
| `0x08000000` | Bootloader code |
| `0x08030000` | Firmware metadata (`firmware_metadata_t`) |
| `0x08031000` | App slot 1 (active) |
| `0x08100000` | Bank 2 base — App slot 2 (OTA target) / log buffer |

### Compile-time configuration (`NASCERR_Config.h`)

| Flag | Default | Effect |
|---|---|---|
| `MOCK_COMMAND` | 0 | Inject a hardcoded command at startup (no I2C master needed) |
| `MOCK_EXPERIMENT` | 0 | Skip real SRAM writes during experiments |
| `ENABLE_LOGS` | 1 | Enable `NASCERR_LOG` system |
| `TEST_MODE` | 1 | 1 = Earth mode (relaxed timing); 0 = Space mode |
| `ENABLE_SABOTAGE_FLASH` | 0 | Probabilistic bit-flip injection in external flash |
| `CURRENT_LOG_LEVEL` | `LOG_INFO` | Maximum log verbosity emitted |
| `ENABLE_SERIAL_SYS` | 0 | Enable serial (UART) log print-out |
