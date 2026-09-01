# wolfHAL Example Board Definitions

The board definitions in this directory are **examples** for use with
wolfHAL's tests and sample applications. They are configured for specific
development boards and are not intended for production use. Users should
create their own board support packages tailored to their hardware.

Each subdirectory contains a board support package (BSP) for a specific
development board. A BSP provides everything needed to build wolfHAL for a
given target: startup code, peripheral initialization, linker script, and
build configuration.

## Supported Boards

| Board | Platform | CPU | Directory |
|-------|----------|-----|-----------|
| Microchip PIC32CZ CA Curiosity Ultra | PIC32CZ | Cortex-M7 | `pic32cz_curiosity_ultra/` |
| ST NUCLEO-C031C6 | STM32C0 | Cortex-M0+ | `stm32c031_nucleo/` |
| ST NUCLEO-F091RC | STM32F0 | Cortex-M0 | `stm32f091rc_nucleo/` |
| ST NUCLEO-F302R8 | STM32F3 | Cortex-M4 | `stm32f302r8_nucleo/` |
| WeAct BlackPill STM32F411 | STM32F4 | Cortex-M4 | `stm32f411_blackpill/` |
| ST NUCLEO-H563ZI | STM32H5 | Cortex-M33 | `stm32h563zi_nucleo/` |
| ST NUCLEO-WB05KZ | STM32WB0 | Cortex-M0+ | `stm32wb05kz_nucleo/` |
| ST NUCLEO-WB55RG | STM32WB | Cortex-M4 | `stm32wb55xx_nucleo/` |
| ST NUCLEO-L152RE | STM32L1 | Cortex-M3 | `stm32l152re_nucleo/` |
| ST NUCLEO-N657X0-Q | STM32N6 | Cortex-M55 | `stm32n657a0_nucleo/` |
| ST NUCLEO-U5A5ZJ-Q | STM32U5 | Cortex-M33 | `stm32u5a5zj_nucleo/` |
| ST NUCLEO-WBA55CG | STM32WBA | Cortex-M33 | `stm32wba55cg_nucleo/` |

## Board Directory Contents

Each board directory contains:

- **`board.mk`** - Build configuration: toolchain, CPU flags, platform
  drivers, and linker script. Included by application Makefiles via
  `include $(BOARD_DIR)/board.mk`.
- **`wolfHAL_board.h`** - Board-level declarations: `extern` globals for vtable-dispatched
  peripherals, `WHAL_CFG_<PLAT>_<X>_DEV` initializer macros consumed by each
  driver TU to define its `whal_<Plat>_<X>_Dev` singleton, `BOARD_<PERIPH>_DEV`
  macros that resolve to `WHAL_INTERNAL_DEV`, `&g_whal<X>`, or a cast pointer
  at one of those singletons (depending on how each peripheral is wired), pin
  definitions, and `Board_Init()`/`Board_Deinit()` prototypes.
- **`wolfHAL_board.c`** - Peripheral instantiation for vtable-dispatched drivers and
  `Board_Init()` implementation (power, clock, GPIO, UART, flash, timer).
- **`linker.ld`** - Linker script defining memory regions (flash, RAM).
- Any additional board-specific source files (e.g. interrupt vector table,
  architecture-specific startup code).

## board.mk Convention

Board `board.mk` files use a self-referencing pattern so that they can be
included from any directory:

```makefile
_BOARD_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
```

`_BOARD_DIR` points to the board's own directory, while the application
Makefile sets `BOARD_DIR` which may point elsewhere (e.g. a private board
overlay). This enables private repositories to extend a board by including
the base `board.mk` and adding additional sources.

### What `BOARD_SOURCE` includes

Board `board.mk` populates `BOARD_SOURCE` with all of the sources
required to build the wolfHAL tests and sample applications for that board:

- Board files: `wolfHAL_board.c` and any additional board-specific source files
- Platform / SoC drivers: e.g. `pic32cz_*.c`, `stm32wb_*.c`
- Architecture support: `systick.c` and any related startup / vector code
- Core wolfHAL modules and common sources: generic dispatch sources such as
  `gpio.c`, `uart.c`, and other files under `src/*.c` (clock and power are
  board-level — header-only or chip-specific, with no generic dispatch source)

In your own projects you may either reuse these defaults by including the
board `board.mk` as-is, or define your own `BOARD_SOURCE` in your
application Makefile to select a different set of modules.

## Adding a New Board

1. Create a new directory: `boards/<vendor>_<board>/`
2. Add `board.mk` following the `_BOARD_DIR` pattern above
3. Implement `wolfHAL_board.h`, `wolfHAL_board.c`, and `linker.ld`
4. Set `PLATFORM`, `TESTS`, toolchain variables, `CFLAGS`, and `BOARD_SOURCE`
5. Build with `make BOARD=<your_board>`
