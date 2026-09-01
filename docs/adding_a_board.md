# Adding a New Board

This guide covers adding a new board configuration to wolfHAL.

## Overview

A board ties a platform to concrete hardware by defining peripheral instances,
pin assignments, clock settings, and startup code. Each board lives in its own
directory under `boards/` named `<platform>_<board_name>/`.

## Required Files

### wolfHAL_board.h

`wolfHAL_board.h` is where the board describes each peripheral to the rest of the
project. It contains four kinds of declaration:

1. `extern` globals for vtable-dispatched drivers (`g_whalUart`,
   `g_whalSpi`, `g_whalI2c`, ...) defined over in `wolfHAL_board.c`.
2. `WHAL_CFG_<PLAT>_<X>_DEV` initializer macros that each single-instance
   driver `.c` uses to define its `whal_<Plat>_<X>_Dev` singleton. The
   driver header `extern`-declares the singleton; the `.c` `#include`s
   `wolfHAL_board.h` and writes `const whal_<X> whal_<Plat>_<X>_Dev = WHAL_CFG_<PLAT>_<X>_DEV;`.
3. `BOARD_<PERIPH>_DEV` macros that the application and tests use to
   reach each peripheral — these resolve to either `WHAL_INTERNAL_DEV`
   (driver ignores it) or to a cast pointer at the singleton
   (e.g. `((whal_Flash *)&whal_<Plat>_Flash_Dev)`).
4. Board constants (pin indices, flash test addresses, etc.) and the
   `Board_Init` / `Board_Deinit` / `Board_WaitMs` prototypes.

```c
/* wolfHAL_board.h */
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/platform/vendor/myplatform.h>

extern whal_Uart    g_whalUart;
extern whal_Timeout g_whalTimeout;

#define BOARD_LED_PIN 0

/* BOARD_<DEVICE>_DEV: how this board reaches each device.
 * WHAL_INTERNAL_DEV for single-instance drivers (driver ignores the
 * pointer); &g_whal<X> for drivers still using vtable dispatch; or a cast
 * pointer at a driver-owned singleton when single-instance must coexist
 * with another driver of the same generic type. */
#define BOARD_GPIO_DEV     WHAL_INTERNAL_DEV
#define BOARD_UART_DEV     (&g_whalUart)
#define BOARD_WATCHDOG_DEV WHAL_INTERNAL_DEV
#define BOARD_RNG_DEV      WHAL_INTERNAL_DEV

/* Initializer for the GPIO single-instance device. The driver header extern-declares
 * whal_Myplatform_Gpio_Dev; the driver .c writes
 *     const whal_Gpio whal_Myplatform_Gpio_Dev = WHAL_CFG_MYPLATFORM_GPIO_DEV;
 * after #include "wolfHAL_board.h". */
#define WHAL_CFG_MYPLATFORM_GPIO_DEV { \
    .base = WHAL_MYPLATFORM_GPIO_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_Myplatform_Gpio_Cfg){ \
        /* ...pin table... */ \
    }, \
}

/* Initializer for the RNG singleton (same pattern). */
#define WHAL_CFG_MYPLATFORM_RNG_DEV { \
    .base = WHAL_MYPLATFORM_RNG_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_Myplatform_Rng_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
void Board_WaitMs(size_t ms);
```

### wolfHAL_board.c

Defines the `extern` device instances declared in `wolfHAL_board.h` (the
vtable-dispatched ones) and implements `Board_Init()` / `Board_Deinit()`.
Single-instance singletons are defined in their driver `.c` files from the
`WHAL_CFG_<PLAT>_<X>_DEV` initializers in `wolfHAL_board.h`; `wolfHAL_board.c` does not
need to declare or initialize them.

`Board_Init()` is responsible for initializing all peripherals in
dependency order. For example, the clock controller must be initialized
before peripherals that depend on it, and a power supply controller (if
present) may need to come before the clock. It should return
`WHAL_SUCCESS` on success or an error code on failure.

`Board_Deinit()` tears down peripherals in reverse order.

```c
#include "wolfHAL_board.h"
#include <wolfHAL/platform/vendor/device.h>
#include "wolfHAL_peripheral.h"

whal_Uart g_whalUart = {
    .base = WHAL_MYPLATFORM_UART1_BASE,
    /* .driver: direct API mapping */
    .cfg = &(whal_Myplatform_Uart_Cfg) {
        .brr = WHAL_MYPLATFORM_UART_BRR(SYSCLK_HZ, 115200),
        .timeout = &g_whalTimeout,
    },
};

static const whal_Myplatform_Clock_PeriphClk g_periphClks[] = {
    {WHAL_MYPLATFORM_GPIOA_GATE},
    {WHAL_MYPLATFORM_UART1_GATE},
};
#define PERIPH_CLK_COUNT \
    (sizeof(g_periphClks) / sizeof(g_periphClks[0]))

whal_Error Board_Init(void)
{
    whal_Error err;
    size_t i;

    err = whal_Myplatform_Clock_EnableOsc(
        &(whal_Myplatform_Clock_OscCfg){WHAL_MYPLATFORM_CLOCK_OSC0_CFG});
    if (err)
        return err;
    err = whal_Myplatform_Clock_SetSysClock(
        WHAL_MYPLATFORM_CLOCK_SYSCLK_SRC_OSC0);
    if (err)
        return err;

    for (i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Myplatform_Clock_EnablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    /* Initialize devices. Pass the corresponding BOARD_<DEVICE>_DEV
     * macro — it will be WHAL_INTERNAL_DEV for single-instance drivers or
     * &g_whal<X> for vtable-dispatched drivers, whichever this board
     * declared in wolfHAL_board.h. */
    err = whal_Gpio_Init(BOARD_GPIO_DEV);
    if (err)
        return err;

    err = whal_Uart_Init(BOARD_UART_DEV);
    if (err)
        return err;

    err = whal_Timer_Init(BOARD_TIMER_DEV);
    if (err)
        return err;

    err = whal_Timer_Start(BOARD_TIMER_DEV);
    if (err)
        return err;

    err = Peripheral_Init();
    if (err)
        return err;

    return WHAL_SUCCESS;
}
```

#### Coexisting drivers of the same type

When two drivers of the same generic type share a board (the typical case
is on-chip flash plus an external SPI-NOR flash), the platform driver's
single-instance device carries `.driver` alongside `.base` and `.cfg` so generic API
calls (`whal_Flash_Read`, etc.) can vtable-dispatch through it.
`BOARD_FLASH_DEV` casts the singleton's address to the non-const generic
type so the dispatcher signature accepts it:

```c
/* wolfHAL_board.h */
#define WHAL_CFG_MYPLATFORM_FLASH_DEV { \
    .driver = WHAL_MYPLATFORM_FLASH_DRIVER, \
    .base   = WHAL_MYPLATFORM_FLASH_BASE, \
    .cfg    = (void *)&(const whal_Myplatform_Flash_Cfg){ /* ... */ }, \
}

#define BOARD_FLASH_DEV ((whal_Flash *)&whal_Myplatform_Flash_Dev)

/* src/flash/myplatform_flash.c — driver TU owns the singleton. */
#include "wolfHAL_board.h"
const whal_Flash whal_Myplatform_Flash_Dev = WHAL_CFG_MYPLATFORM_FLASH_DEV;
```

The external SPI-NOR side does not need this dance — it is already a
peripheral driver with its own `g_whal<X>` and vtable, and lives in
`boards/peripheral/`.

The watchdog is intentionally excluded from `Board_Init()` and `Board_Deinit()`.
Once started, most watchdog peripherals cannot be stopped — if `Board_Init()`
starts the watchdog, any delay before the application begins refreshing it will
cause an unexpected reset. The application or test should call
`whal_Watchdog_Init(BOARD_WATCHDOG_DEV)` directly when it is ready to begin
refreshing. The board still declares the watchdog device (whether through
a `WHAL_CFG_<PLAT>_<IWDG|WWDG>_DEV` initializer that the driver `.c`
consumes or as `g_whalWatchdog` in `wolfHAL_board.c`, depending on the driver),
points `BOARD_WATCHDOG_DEV` at it, and enables any required clocks
(e.g., WWDG APB clock, IWDG LSI oscillator) in `Board_Init()` so the
watchdog is ready to be started.

```c
whal_Error Board_Deinit(void)
{
    whal_Error err;

    err = Peripheral_Deinit();
    if (err)
        return err;

    whal_Timer_Stop(BOARD_TIMER_DEV);
    whal_Timer_Deinit(BOARD_TIMER_DEV);
    whal_Uart_Deinit(BOARD_UART_DEV);
    whal_Gpio_Deinit(BOARD_GPIO_DEV);

    for (size_t i = PERIPH_CLK_COUNT; i-- > 0; ) {
        err = whal_Myplatform_Clock_DisablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    return WHAL_SUCCESS;
}
```

### board.mk

Defines the toolchain, compiler flags, and source file list:

```makefile
_BOARD_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

PLATFORM = myplatform
TESTS = gpio clock uart flash timer

GCC     = arm-none-eabi-gcc
LD      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS  = -mcpu=cortex-m4 -mthumb -Os -Wall -MMD $(INCLUDE) -I$(_BOARD_DIR) \
          -DWHAL_CFG_MYPLATFORM_GPIO_DIRECT_API_MAPPING \
          -DWHAL_CFG_MYPLATFORM_UART_DIRECT_API_MAPPING \
          -DWHAL_CFG_MYPLATFORM_UART_SINGLE_INSTANCE
LDFLAGS = -mcpu=cortex-m4 -mthumb -nostdlib -lgcc

LINKER_SCRIPT = $(_BOARD_DIR)/linker.ld

INCLUDE += -I$(_BOARD_DIR) -I$(WHAL_DIR)/boards/peripheral

BOARD_SOURCE  = $(_BOARD_DIR)/wolfHAL_board.c
BOARD_SOURCE += $(_BOARD_DIR)/ivt.c
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*.c)
# Dispatch sources for mapped types are excluded; keep dispatch sources
# for types that may have peripheral drivers (flash, block, sensor).
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/timer.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/flash.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/block.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/myplatform_*.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/systick.c)

# Peripheral devices
include $(WHAL_DIR)/boards/peripheral/board.mk
```

Two families of build-time knobs commonly appear in `board.mk`:

- `WHAL_CFG_<DRIVER>_DIRECT_API_MAPPING` renames the chip-specific
  driver functions to the generic API names, eliminating the vtable
  dispatch wrapper. Safe whenever a generic-API type has at most one
  driver linked into the build.
- `WHAL_CFG_<PLAT>_<DRV>_SINGLE_INSTANCE` makes a conditionally
  single-instance driver read its `.base` and `.cfg` from the
  corresponding `whal_<Plat>_<Drv>_Dev` singleton (defined in the driver
  `.c` from the `WHAL_CFG_<PLAT>_<DRV>_DEV` initializer in `wolfHAL_board.h`)
  instead of dereferencing the handle argument. The flag is per-driver
  per-platform; default builds keep the pointer-based path. Other
  drivers are unconditionally single-instance and need no flag — their
  driver bodies always read from the singleton.

## Peripheral Devices

Boards support optional external peripheral devices (e.g., SPI-NOR flash, SD
cards) through the peripheral system in `boards/peripheral/`. To enable this:

1. Include `wolfHAL_peripheral.h` in `wolfHAL_board.c` and add the peripheral include path
   (`-I$(WHAL_DIR)/boards/peripheral`) in `board.mk`.

2. Include `boards/peripheral/board.mk` at the end of the board's
   `board.mk`. This conditionally compiles peripheral drivers based on
   build-time flags (e.g., `PERIPHERAL_SPI_NOR_W25Q64=1`).

3. Call `Peripheral_Init()` at the end of `Board_Init()` and
   `Peripheral_Deinit()` at the top of `Board_Deinit()`. These functions
   iterate the peripheral registry arrays and initialize/deinitialize all
   enabled peripheral devices.

`Peripheral_Init()` and `Peripheral_Deinit()` are safe to call even when no
peripherals are enabled — the registry arrays will be empty and the functions
return immediately.

See [Adding a Peripheral](adding_a_peripheral.md) for details on how to add
new peripheral devices to the registry.

### linker.ld

Linker script defining the memory layout for your board's MCU. Must define
FLASH and RAM regions, place `.isr_vector` at the start of FLASH, and set up
`.text`, `.data`, and `.bss` sections.

### ivt.c (ARM targets)

Interrupt vector table and `Reset_Handler`. The reset handler copies `.data`
from FLASH to RAM, zeroes `.bss`, and calls `main()`. This file is specific to
ARM Cortex-M targets. Other architectures will need their own startup code.
