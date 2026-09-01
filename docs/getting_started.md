# Getting Started

This guide walks through integrating wolfHAL into a bare-metal embedded project.

## Project Layout

A typical project using wolfHAL looks like this:

```
my_project/
  wolfHAL/                          wolfHAL repository (submodule, copy, etc.)
  boards/
    <board_name>/
      wolfHAL_board.h               Per-peripheral DEV macros and config initializers
      wolfHAL_board.c               Pointer-based device globals and Board_Init
      ivt.c                         Interrupt vector table and Reset_Handler
      linker.ld                     Linker script for your MCU
      board.mk                      Toolchain, source list, and feature flags
  src/
    main.c                          Application entry point
    ...                             Additional application sources
  Makefile
```

Your project provides the board-level glue (device instances, pin assignments,
clock config, startup code) and wolfHAL provides the driver implementations
and API.

## Adding wolfHAL to Your Project

wolfHAL is a source-level library with no external dependencies beyond a C
compiler and standard headers (`stdint.h`, `stddef.h`). To use it:

1. Add the wolfHAL repository root to your include path (e.g., `-I/path/to/wolfHAL`).
2. Compile the generic dispatch sources for the device types you need
   (`src/gpio/gpio.c`, `src/uart/uart.c`, `src/flash/flash.c`, …), **except**
   for types where you've enabled direct API mapping — see below.
3. Compile the platform-specific driver sources for your target
   (`src/gpio/<platform>_gpio.c`, `src/uart/<platform>_uart.c`, …).

You only need the modules and drivers your project actually uses.

## The Device Model

This section describes how wolfHAL represents a device. The driver categories,
the device struct, the generic API and driver vtable, and the
configuration knobs a board picks per device.

### The device struct
Most device types share the same three fields in their struct
- `.base` — the base address of the register map.
- `.driver` — the function-pointer table.
- `.cfg` — driver-specific configuration settings (pins, baud rate, etc.).
```c
struct whal_Uart {
    const size_t base;             /* register block address */
    const whal_UartDriver *driver; /* driver vtable */
    void *cfg;                     /* driver-specific config */
};
```

### The device API and driver vtable
Each device type defines an API to access the underlying driver implementation

```c
whal_Error whal_Uart_Init(whal_Uart *uartDev);
whal_Error whal_Uart_Deinit(whal_Uart *uartDev);
whal_Error whal_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz);
whal_Error whal_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz);
whal_Error whal_Uart_SendAsync(whal_Uart *uartDev, const void *data, size_t dataSz);
whal_Error whal_Uart_RecvAsync(whal_Uart *uartDev, void *data, size_t dataSz);

typedef struct {
    /* Initialize the UART hardware. */
    whal_Error (*Init)(whal_Uart *uartDev);
    /* Deinitialize the UART hardware. */
    whal_Error (*Deinit)(whal_Uart *uartDev);
    /* Transmit a buffer. */
    whal_Error (*Send)(whal_Uart *uartDev, const void *data, size_t dataSz);
    /* Receive into a buffer. */
    whal_Error (*Recv)(whal_Uart *uartDev, void *data, size_t dataSz);
    /* Start an asynchronous transmit. NULL if not supported. */
    whal_Error (*SendAsync)(whal_Uart *uartDev, const void *data, size_t dataSz);
    /* Start an asynchronous receive. NULL if not supported. */
    whal_Error (*RecvAsync)(whal_Uart *uartDev, void *data, size_t dataSz);
} whal_UartDriver;
```


### Single-instance vs Multi-instance devices

**Multi-instance devices.** The driver reads its `.base` and `.cfg`
from the device handle the caller passes in:

```c
whal_Error whal_Stm32wb_Uart_Send(whal_Uart *dev, ...)
{
    size_t base = dev->base;
    whal_Stm32wb_Uart_Cfg *cfg = (whal_Stm32wb_Uart_Cfg *)dev->cfg;
    /* ... */
}
```

The board declares the device as a global in `wolfHAL_board.c`. The
caller passes it through the API.

**Single-instance.** The driver reads `.base` and `.cfg` from a named
single-instance device it owns. The handle parameter still exists (the
function sits behind a generic vtable signature) but the body ignores it:

```c
/* stm32wb_rng.c */
const whal_Rng whal_Stm32wb_Rng_Dev = WHAL_CFG_STM32WB_RNG_DEV;

whal_Error whal_Stm32wb_Rng_Generate(whal_Rng *dev, ...)
{
    size_t base = whal_Stm32wb_Rng_Dev.base;
    (void)dev;
    /* ... */
}
```

```c
/* wolfHAL_board.h */
#define WHAL_CFG_STM32WB_RNG_DEV { \
    .base = WHAL_STM32WB_RNG_BASE, \
    .cfg = &(whal_Stm32wb_Rng_Cfg) {...} \ 
} 

```

The single-instance device struct is *defined* in the driver `.c`,
initialized from a `WHAL_CFG_<PLAT>_<X>_DEV` macro the board supplies
in `wolfHAL_board.h`. The driver `#include`s `wolfHAL_board.h` to pull in the
initializer. Callers pass `WHAL_INTERNAL_DEV` (defined as `((void *)0)`)
at the call site to make the intent explicit.

#### Configuring a multi-instance driver to be a single-instance driver

Some boards only wire one instance of a multi-instance capable driver.
In that case the driver can be compiled in its single-instance form by
defining `WHAL_CFG_<PLAT>_<X>_SINGLE_INSTANCE` in `board.mk`. The board
then supplies a `WHAL_CFG_<PLAT>_<X>_DEV` initializer in `wolfHAL_board.h` and
points `BOARD_<X>_DEV` at `WHAL_INTERNAL_DEV`, exactly as it would for
any unconditional single-instance driver.

For example, to compile STM32WB UART as single-instance:

```make
# board.mk
CFLAGS += -DWHAL_CFG_STM32WB_UART_SINGLE_INSTANCE
```

```c
/* wolfHAL_board.h */
#define WHAL_CFG_STM32WB_UART_DEV { \
    .base = WHAL_STM32WB55_UART1_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_Stm32wb_Uart_Cfg){ \
        .brr     = WHAL_STM32WB_UART_BRR(64000000, 115200), \
        .timeout = &g_whalTimeout, \
    }, \
}

#define BOARD_UART_DEV WHAL_INTERNAL_DEV
```

### Vtable Dispatch vs Direct API Mapping

**Vtable dispatch.** The generic dispatch source `src/<type>/<type>.c` defines
the top-level API functions which call the underlying driver function pointer.
Vtable dispatch is necessary when you have more than one driver in a particular
device type. I.E. using stm32wb_flash and spi_nor_flash in the same application.
```c
/* uart.c */
inline whal_Error whal_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz)
{
    return uartDev->driver->Send(uartDev, data, dataSz);
}

/* stm32wb_uart.c */
whal_Error whal_Stm32wb_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz)
{
    ...
}

const whal_UartDriver whal_Stm32wb_Uart_Driver = {
    ...
    .Send = whal_Stm32wb_Uart_Send,
    ...
};

/* wolfHAL_board.c — vtable dispatch */
whal_Flash g_whalFlash = {
    .base   = WHAL_STM32WB55_FLASH_BASE,
    .driver = &whal_Stm32wb_Uart_Driver,
    .cfg    = &flashCfg,
};
```

**Direct API mapping** (`WHAL_CFG_<DRIVER>_DIRECT_API_MAPPING`). If it's the
case that your application only uses a single driver from a specific device type
then you can define `WHAL_CFG_<DRIVER>_DIRECT_API_MAPPING` to map the top level
API to directly to the underlying driver function, bypassing the vtable.

```make
# board.mk
CFLAGS += -DWHAL_CFG_STM32WB_UART_SINGLE_INSTANCE
```

```c
/* wolfHAL_board.c — direct API mapping */
whal_Uart g_whalUart = {
    .base = WHAL_STM32WB55_UART1_BASE,
    /* .driver: direct API mapping */
    .cfg  = &uartCfg,
};
```

One constraint come with direct API mapping:

**Only one driver of that type per build.** Both the dispatch source
and the mapped driver source provide the same top-level symbols, so
the dispatch source must be excluded from `board.mk`. And a board
that needs to host multiple drivers of the same type (e.g. on-chip
flash *and* SPI NOR flash) can't use mapping for that type — it has
to keep vtable dispatch so both drivers can link.

## Board Files and Initialization

The board is responsible for configuring and initializing devices. This is akin
to a BSP layer. In the wolfHAL project we provide reference board examples in
`boards/`. These are only to be used within this repo for the examples and
tests.

The typical board init sequence:

1. Pre-clock setup, I.E. enable regulators, set flash wait states.
2. Bring up the clock tree (oscillators, optional PLL, sysclk source)
3. Enable device clocks
4. Call the driver init functions
5. Start timers

The chip's clock driver exposes `Enable*`/`Disable*`/`Set*` helpers that
the board calls in order. There is no generic `whal_Clock_Init` walker —
clock-tree shape varies too much across vendors to abstract.

```c
/* wolfHAL_board.c */

static const whal_Myplatform_Clock_PeriphClk g_periphClks[] = {
    {WHAL_MYPLATFORM_GPIOB_GATE},
    {WHAL_MYPLATFORM_UART1_GATE},
};
#define PERIPH_CLK_COUNT (sizeof(g_periphClks) / sizeof(g_periphClks[0]))

whal_Error Board_Init(void)
{
    whal_Error err;

    /* Bring up clocks — chip-specific helpers, called in order. No device
     * pointer; each helper reads the chip's fixed base from its own header. */
    err = whal_Myplatform_Clock_EnableOsc(
        &(whal_Myplatform_Clock_OscCfg){WHAL_MYPLATFORM_OSC0_CFG});
    if (err) return err;
    err = whal_Myplatform_Clock_SetSysClock(WHAL_MYPLATFORM_SYSCLK_SRC_OSC0);
    if (err) return err;

    /* Enable peripheral clocks. */
    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Myplatform_Clock_EnablePeriphClk(&g_periphClks[i]);
        if (err) return err;
    }

    /* Initialize peripherals through BOARD_<X>_DEV. */
    err = whal_Gpio_Init(BOARD_GPIO_DEV);       if (err) return err;
    err = whal_Uart_Init(BOARD_UART_DEV);       if (err) return err;
    err = whal_Timer_Init(BOARD_TIMER_DEV);     if (err) return err;
    err = whal_Timer_Start(BOARD_TIMER_DEV);    if (err) return err;

    return WHAL_SUCCESS;
}
```

See the board examples in `boards/` for complete sequences.

## Using the API

```c
/* Include the wolfHAL and board header */
#include <wolfHAL/wolfHAL.h>
#include "wolfHAL_board.h"

void main(void)
{
    /* Initialize your devices */
    if (Board_Init() != WHAL_SUCCESS)
        while (1);

    while (1) {
        /* Use the API */
        whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 1);
        whal_Uart_Send(BOARD_UART_DEV, "Hello!\r\n", 8);
        Board_WaitMs(1000);

        whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 0);
        Board_WaitMs(1000);
    }
}
```

## Next Steps
- See [Writing a Driver](writing_a_driver.md) for how to add support for
  a new platform.
- See [Adding a Board](adding_a_board.md) for how to wire up a new
  hardware target.
