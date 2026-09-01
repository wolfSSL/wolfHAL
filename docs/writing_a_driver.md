# Writing a Driver

This guide covers how to implement wolfHAL drivers.

## Generating the Driver Boilerplate

Before writing a driver by hand, you can generate the boilerplate with the
`generate-driver-template` Claude Code skill, then fill in the register
definitions and function bodies following the rest of this guide:

```
/generate-driver-template <device_type> <device_name> [single|multi]
```

The skill reads the device type's generic vtable (or recognizes board-level
types like clock/power) and emits idiomatic, compiling stubs:

- **vtable device types** (spi, i2c, uart, rng, gpio, dma, eth, …) — the
  `wolfHAL/<type>/<name>_<type>.h` + `src/<type>/<name>_<type>.c` pair with the
  direct-API-mapping `#define` block, every vtable function stubbed
  (argument-validate → `TODO` → `return WHAL_SUCCESS`), and the driver vtable
  at the bottom of the `.c`. Pass `single` or `multi` to choose the instance
  model (see "Single-instance" / "multi-instance" conventions below).
- **board-level types** (clock, power, …) — a single header-only
  `wolfHAL/<type>/<name>_<type>.h` with the fixed `_BASE` macro and stubbed
  `static inline` helpers; no `.c`, no vtable, no device handle.

It leaves register layouts, config-struct fields, descriptor structs, and enums
as `TODO`s — those come from the TRM and are yours to fill in. The skill
scaffolds a single driver; for an entire chip-family port (platform header,
reused-driver aliases, and a first board) use the `port-stm32-platform` skill
instead.

For example, to scaffold a multi-instance SPI driver for a new `stm32g4`
platform:

```
/generate-driver-template spi stm32g4 multi
```

## Driver Categories

wolfHAL has three categories of drivers:

**Platform drivers** operate directly on SoC hardware registers at fixed memory
addresses. They are tied to a specific microcontroller family and use the
`whal_Reg_*` helpers to read and write peripheral registers. Examples:
`stm32wb_uart`, `stm32wb_gpio`, `stm32wb_dma`.

**Peripheral drivers** communicate with external chips over a bus (SPI, I2C,
MDIO). They call platform drivers (e.g., `whal_Spi_SendRecv`) to reach the
hardware and are portable across any SoC that provides the required bus.
Examples: `spi_nor_flash` (SPI flash), `sdhc_spi_block` (SD card over SPI),
`bmi270_sensor` (IMU over I2C), `lan8742a_eth_phy` (Ethernet PHY over MDIO).

Platform and peripheral drivers share the same vtable interface — the
application calls `whal_Flash_Read()` whether the flash is on-chip (platform
driver) or external SPI NOR (peripheral driver).

## Common Patterns

Platform and peripheral drivers follow the same structure:

1. A **driver vtable** — a struct of function pointers that the driver must
   populate.
2. A **device struct** — contains a base address, a pointer to the driver
   vtable, and a pointer to driver-specific configuration.
3. A **generic dispatch layer** — validates inputs and calls through the vtable.

To write a driver for a device type, you implement the functions defined in that
type's vtable and expose them as a const driver instance.

### File Layout and Naming Conventions

For platform and peripheral drivers implementing device type `uart` on platform `myplatform`:

- `wolfHAL/uart/myplatform_uart.h` — configuration types and driver extern
- `src/uart/myplatform_uart.c` — driver implementation and vtable definition

### Defining and exposing the driver Vtable

Every vtable includes `Init` and `Deinit`. The remaining functions are specific
to the device type. All functions receive a pointer to the device instance as
their first argument and return `whal_Error`.

```c
/* myplatform_uart.c */
const whal_UartDriver whal_Myplatform_Uart_Driver = {
    .Init   = whal_Myplatform_Uart_Init,
    .Deinit = whal_Myplatform_Uart_Deinit,
    /* device-specific operations */
};

/* myplatform_uart.h */
extern const whal_UartDriver whal_Myplatform_Uart_Driver;

```

### Direct API Mapping

Every driver should include a rename-at-definition block that lets the build
system map chip-specific function names to the top-level API. Place this
`#ifdef` block after your register defines and before the first function
definition:

```c
#ifdef WHAL_CFG_MYPLATFORM_UART_DIRECT_API_MAPPING
#define whal_Myplatform_Uart_Init   whal_Uart_Init
#define whal_Myplatform_Uart_Deinit whal_Uart_Deinit
/* ... one #define per mapped function ... */
#endif
```

When the flag is defined, the preprocessor renames each function to its
generic API name at the definition site. The driver source uses chip-specific
names everywhere — the macros handle the rest. The flag name is
`WHAL_CFG_<DRIVER>_DIRECT_API_MAPPING` where `<DRIVER>` is the driver's full
prefix in upper case, e.g., `WHAL_CFG_STM32WB_UART_DIRECT_API_MAPPING` for a
platform driver or `WHAL_CFG_BMI270_SENSOR_DIRECT_API_MAPPING` for a
peripheral driver.

Wrap the vtable in `#ifndef` since it is unused when mapping is active:

```c
#ifndef WHAL_CFG_MYPLATFORM_UART_DIRECT_API_MAPPING
const whal_UartDriver whal_Myplatform_Uart_Driver = {
    .Init   = whal_Myplatform_Uart_Init,
    .Deinit = whal_Myplatform_Uart_Deinit,
};
#endif
```

In the chip header, wrap the mapped function prototypes and extern vtable
declaration in the same `#ifndef` guard. Types and configuration structs
stay unconditional. Extension functions (`Ext_*`) that have no generic
equivalent are never mapped and stay unconditional.

### Conventions for single-instance devices
When there's only a single IP block for a device on a chip then this is
considered a single-instance device. For example: the STM32WB platform only has
a single RNG device.

Single-instance device drivers must define the device struct within the driver
.c file like so:

```c
/* wolfHAL/uart/myplatform_uart.h */
extern const whal_Uart whal_Myplatform_Uart_Dev;

/* src/uart/myplatform_uart.c */
#include "wolfHAL_board.h"  /* provides WHAL_CFG_MYPLATFORM_UART_DEV initializer */
const whal_Uart whal_Myplatform_Uart_Dev = WHAL_CFG_MYPLATFORM_UART_DEV;

whal_Error whal_Myplatform_Uart_Init(whal_Uart *uartDev)
{
    const whal_Myplatform_Uart_Cfg *cfg =
        (const whal_Myplatform_Uart_Cfg *)whal_Myplatform_Uart_Dev.cfg;
    size_t base = whal_Myplatform_Uart_Dev.base;
    (void)uartDev;
    /* ... */
}
```

### Conventions for multi-instance devices
When there's multiple IP blocks for a device on a chip then this is considered a
multi-instance device. For example: the STM32WB platform has multiple UART
devices, each with their own register map offset.

Since the user could use multiple instances of the device the user must define
the device struct within the wolfHAL_board.c and pass it in as a function argument. The
driver must determine this at runtime like so:

```c
/* src/uart/myplatform_uart.c */
whal_Error whal_Myplatform_Uart_Init(whal_Uart *uartDev)
{
    const whal_Myplatform_Uart_Cfg *cfg =
        (const whal_Myplatform_Uart_Cfg *)uartDev->cfg;
    size_t base = uartDev->base;
    /* ... */
}
```

#### Supporting a single-instance use on a multi-instance device driver
In some cases a user's application only needs a single instance from a
multi-instance device driver. To support this use case the driver should provide
a compile-time configuration option that makes the multi-instance driver behave
identical to a single-instance driver. Here is an example of how to achieve
this:

```c
/* wolfHAL/uart/myplatform_uart.h */
#ifdef WHAL_CFG_MYPLATFORM_UART_SINGLE_INSTANCE
extern const whal_Uart whal_Myplatform_Uart_Dev;
#endif

/* src/uart/myplatform_uart.c */
#ifdef WHAL_CFG_MYPLATFORM_UART_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides WHAL_CFG_MYPLATFORM_UART_DEV initializer */
const whal_Uart whal_Myplatform_Uart_Dev = WHAL_CFG_MYPLATFORM_UART_DEV;
#endif

whal_Error whal_Myplatform_Uart_Init(whal_Uart *uartDev)
{
    const whal_Myplatform_Uart_Cfg *cfg;
    size_t base;
#ifdef WHAL_CFG_MYPLATFORM_UART_SINGLE_INSTANCE
    cfg = (const whal_Myplatform_Uart_Cfg *)whal_Myplatform_Uart_Dev.cfg;
    base = whal_Myplatform_Uart_Dev.base;
    (void)uartDev;
#else
    cfg = (const whal_Myplatform_Uart_Cfg *)uartDev->cfg;
    base = uartDev->base;
#endif
    /* ... */
}
```

### No Cross-Driver Calls

Platform drivers should only touch their own registers. The board handles all
cross-peripheral setup, clock enables, power-rail sequencing, flash wait
states, before calling Init.

There are two exceptions:

- **Peripheral drivers** call their underlying bus driver (SPI, I2C, MDIO) to
  communicate with the external chip.
- **DMA-backed platform drivers** (e.g., `stm32wb_uart_dma`) call the DMA
  driver to set up and start transfers. The DMA device is passed through the
  driver's configuration struct.

### Register Access

wolfHAL provides register access helpers in `wolfHAL/reg.h`:

- `whal_Reg_Write()` — write a full register value
- `whal_Reg_Read()` — read a full register value
- `whal_Reg_Update()` — read-modify-write a specific field within a register
- `whal_Reg_Get()` — read and extract a masked field from a register

Use `Write` and `Read` for whole-register access. Use `Update` and `Get` when
you need to modify or read individual fields within a register without
disturbing other bits.

### Timeouts

Most drivers poll hardware status registers in busy-wait loops (e.g., waiting
for a DMA transfer to complete, a flash erase to finish, or an AES computation
to produce a result). wolfHAL provides an optional timeout mechanism to bound
these waits and prevent infinite hangs if hardware misbehaves.

#### Timeout Struct

The board creates a `whal_Timeout` instance with a tick source and a timeout
duration. Drivers receive a pointer to it through their configuration struct:

```c
typedef struct {
    uint32_t timeoutTicks;     /* max ticks before timeout */
    uint32_t startTick;        /* snapshot, set by START */
    uint32_t (*GetTick)(void); /* board-provided tick source */
} whal_Timeout;
```

The tick units are determined by the board's `GetTick` implementation. A 1 kHz
SysTick gives millisecond ticks; a 1 MHz timer gives microsecond ticks. Drivers
do not need to know the tick rate.

The tick fields are 32-bit by default; define `WHAL_CFG_64BIT_TICK` to widen
them to 64-bit (see "64-bit Ticks" below).

#### whal_Reg_ReadPoll

For the common case of polling a register bit, use `whal_Reg_ReadPoll` from
`wolfHAL/reg.h`:

```c
whal_Error whal_Reg_ReadPoll(size_t base, size_t offset,
                              size_t mask, size_t value,
                              whal_Timeout *timeout);
```

This polls until `(reg & mask) == value` or the timeout expires. Pass the mask
as the value to wait for bits to be set, or `0` to wait for bits to be clear:

```c
/* Wait for TXE flag to be set */
err = whal_Reg_ReadPoll(base, SPI_SR_REG, SPI_SR_TXE_Msk,
                        SPI_SR_TXE_Msk, cfg->timeout);

/* Wait for BSY flag to be clear */
err = whal_Reg_ReadPoll(base, SPI_SR_REG, SPI_SR_BSY_Msk,
                        0, cfg->timeout);
```

A NULL timeout pointer means unbounded wait (poll forever).

#### Cleanup on Timeout

When a timeout occurs during an operation that has enabled a hardware mode
(e.g., flash programming mode, AES enable), the driver must still clean up
before returning. Use a `goto cleanup` pattern:

```c
whal_Error err = WHAL_SUCCESS;

whal_Reg_Update(base, CR_REG, PG_Msk, 1);   /* enable programming mode */

for (...) {
    err = whal_Reg_ReadPoll(base, SR_REG, BSY_Msk, 0, cfg->timeout);
    if (err)
        goto cleanup;
}

cleanup:
    whal_Reg_Update(base, CR_REG, PG_Msk, 0);   /* always disable */
    return err;
```

Never return directly from inside a polling loop if the peripheral is in a
mode that requires cleanup.

#### Compile-Time Disable

Define `WHAL_CFG_NO_TIMEOUT` to remove all timeout logic from the binary.
When defined, `WHAL_TIMEOUT_START` becomes a no-op and `WHAL_TIMEOUT_EXPIRED`
always evaluates to `0`, so polling loops run until the hardware condition is
met with no overhead.

#### 64-bit Ticks

By default the tick fields (`timeoutTicks`, `startTick`, and the `GetTick`
return value) are 32-bit, which wraps in about 49 days at millisecond
resolution. Define `WHAL_CFG_64BIT_TICK` to widen them to `uint64_t` for
systems that need a longer monotonic range. The elapsed-time comparison in
`WHAL_TIMEOUT_EXPIRED` uses unsigned wraparound arithmetic that adjusts to the
configured width, so both settings handle a tick-counter wrap correctly.

When the macro is defined, the board's tick source must match the width: its
`g_tick` counter and `GetTick` callback (typically `Board_GetTick`) must be
`uint64_t`, since `whal_Timeout.GetTick` becomes `uint64_t (*)(void)`. The
`WHAL_TICK_MAX` macro in `wolfHAL/timeout.h` evaluates to the maximum tick
value at the configured width (`UINT32_MAX` or `UINT64_MAX`) for wrap-handling
logic.

A 64-bit `GetTick` must return a coherent snapshot. On a 32-bit MCU a 64-bit
read is two loads, so a tick interrupt landing between them returns a torn value
and `WHAL_TIMEOUT_EXPIRED` reports a spurious timeout. Read with interrupts
masked, or retry until the halves agree.

#### Adding Timeout to a Config Struct

Driver config structs that use polling should include an optional timeout
pointer:

```c
typedef struct {
    /* ... other config fields ... */
    whal_Timeout *timeout;
} whal_Myplatform_Foo_Cfg;
```

The timeout is optional — if the board does not set it (NULL), all waits are
unbounded.

### Reusing a Driver Across Platforms

Many MCU families share identical peripheral IP blocks. For example, the
STM32WB and STM32H5 have register-compatible GPIO and UART peripherals.
Rather than duplicating driver code, create thin alias files for the new
platform that re-export the existing driver under platform-specific names.

#### Header

The alias header `typedef`s the config structs and `#define`s the driver
instance, functions, and any enum constants. The driver and function aliases
are wrapped in `#ifndef` so they are omitted when direct API mapping is active
for the alias platform:

```c
/* stm32h5_gpio.c */
#ifndef WHAL_STM32H5_GPIO_H
#define WHAL_STM32H5_GPIO_H

#include <wolfHAL/gpio/stm32wb_gpio.h>

typedef whal_Stm32wb_Gpio_Cfg    whal_Stm32h5_Gpio_Cfg;
typedef whal_Stm32wb_Gpio_PinCfg whal_Stm32h5_Gpio_PinCfg;

#ifndef WHAL_CFG_STM32H5_GPIO_DIRECT_API_MAPPING
#define whal_Stm32h5_Gpio_Driver whal_Stm32wb_Gpio_Driver
#define whal_Stm32h5_Gpio_Init   whal_Stm32wb_Gpio_Init
#define whal_Stm32h5_Gpio_Deinit whal_Stm32wb_Gpio_Deinit
#define whal_Stm32h5_Gpio_Get    whal_Stm32wb_Gpio_Get
#define whal_Stm32h5_Gpio_Set    whal_Stm32wb_Gpio_Set
#endif

/* Re-export enum constants under the new platform name */
#define WHAL_STM32H5_GPIO_MODE_OUT WHAL_STM32WB_GPIO_MODE_OUT
/* ... */

#endif
```

Use `typedef` for types (gives proper type-checking and debugger visibility)
and `#define` for the driver instance and functions (which are values, not
types).


#### Source

The source file includes the original implementation directly:

```c
/* stm32h5_gpio.c */
#include "stm32wb_gpio.c"
```

#### When to alias vs. write a new driver

Alias when the register layout is identical, same offsets, same bit positions,
same behavior. If even one register differs (different bit position, extra
field, different reset value that affects behavior), write a new driver. A
partial alias that papers over register differences with workarounds is worse
than a clean separate implementation.

### Platform Device Macros

Add base address and driver macros to your platform header
(`wolfHAL/platform/<vendor>/<device>.h`) so that board configs can instantiate
devices without knowing the register addresses or driver symbols:

```c
/* myplatform.h */
#define WHAL_MYPLATFORM_UART_BASE   0x40000000
#define WHAL_MYPLATFORM_UART_DRIVER &whal_Myplatform_Uart_Driver
```

The board uses these in device struct initializers:

```c
/* wolfHAL_board.c */

#include <wolfHAL/platform/myvendor/myplatform.h>
whal_Uart g_whalUart = {
    .base = WHAL_MYPLATFORM_UART_BASE,
    .driver = WHAL_MYPLATFORM_UART_DRIVER,
    .cfg = &uartCfg,
};
```
---

## Clock

Clock is a **board-level driver** (see Driver Categories). There is no
generic `clock.h`, no `whal_Clock` handle, no `whal_Clock_Init`/`Deinit`/
`Enable`/`Disable` API, no `whal_ClockDriver` vtable. Each chip clock
driver exposes imperative chip-specific helpers that boards call
directly from `Board_Init` in the right order. The driver header owns
the chip's fixed clock-controller `_BASE` macro and the helpers take no
device pointer parameter.

### API contract

The chip clock driver header may declare ONLY functions whose names match
one of these prefix patterns:

- `whal_<Chip>_<Subsys>_Enable<Node>(...)` — turn a clock node on (and wait
  ready, if applicable)
- `whal_<Chip>_<Subsys>_Disable<Node>(...)` — turn a clock node off
- `whal_<Chip>_<Subsys>_Set<Node>(...)` — change a clock node's
  selection/value (e.g. `SetSysClock`, peripheral-mux selects, divider/range
  selects)

Where `<Subsys>` is the chip's clock-controller name (e.g. `Rcc` on STM32,
`Clock` on PIC32CZ). NOT allowed in the public header: `Init`, `Deinit`,
`Configure`, `BringUp`, `GetFreq`, or any other operation. Internal helpers go
in the `.c` file as `static`.

Configuration and enabling are bundled into a single `Enable*` call where
the order is fixed (e.g. `EnablePll` writes the dividers/source then turns
the PLL on and waits for ready). Boards do not call separate `Configure`
operations.

The chip header may declare types, enums, and `_CFG` field-initializer
macros freely — those are not constrained.

### Typical helpers

A chip with a fairly common clock tree exposes:

```c
#define WHAL_<CHIP>_<SUBSYS>_BASE  0x<addr>   /* at top of header */

whal_Error whal_<Chip>_<Subsys>_EnableOsc(const whal_<Chip>_<Subsys>_OscCfg *);
whal_Error whal_<Chip>_<Subsys>_DisableOsc(const whal_<Chip>_<Subsys>_OscCfg *);
whal_Error whal_<Chip>_<Subsys>_EnablePll(const whal_<Chip>_<Subsys>_PllCfg *);
whal_Error whal_<Chip>_<Subsys>_DisablePll(void);
whal_Error whal_<Chip>_<Subsys>_SetSysClock(whal_<Chip>_<Subsys>_SysClockSrc);
whal_Error whal_<Chip>_<Subsys>_EnablePeriphClk(const whal_<Chip>_<Subsys>_PeriphClk *);
whal_Error whal_<Chip>_<Subsys>_DisablePeriphClk(const whal_<Chip>_<Subsys>_PeriphClk *);
```

Adapt the set to what the chip actually has. A chip without a PLL drops
`EnablePll`/`DisablePll`. A chip with extra muxes adds `Set*` helpers. A
chip with a different topology (e.g., generic-clock generators routing to
peripheral channels) adds operations like `EnableGclkGen` and keeps to the same
naming convention.

### Reference implementation

Clock drivers are header-only — every `whal_<Chip>_<Subsys>_*` helper is
defined as a `static inline` in the chip's clock header, and there is no
matching `.c` file under `src/clock/`. `wolfHAL/clock/stm32wb_rcc.h` is
the canonical reference shape; new chip clock drivers should be modeled
on it. `wolfHAL/clock/pic32cz_clock.h` shows how the same convention
covers a fundamentally different topology (oscillators + GCLK generators
+ peripheral channels).

### Board responsibilities

The board calls helpers in the right order in `Board_Init` — typically
oscillator(s) on, PLL configure-and-enable, sysclk source switch,
peripheral clock enables. The board also handles flash wait states and
voltage scaling around the sysclk transition. There is no walker; the
ordering is explicit at the call site.

---

## GPIO

Header: `wolfHAL/gpio/gpio.h`

The GPIO driver configures and controls general-purpose I/O pins. The
configuration describes a table of pins, and the API operates on pins by their
index in that table (not raw hardware pin/port numbers).

### Init

Configure all pins described in the device's configuration. For each pin this
typically involves:

- Setting the pin mode (input, output, alternate function, analog)
- Configuring output type (push-pull or open-drain), speed, and pull
  resistors as applicable
- Setting the alternate function mux if the pin is in alternate function mode
  (e.g., for UART TX/RX or SPI signals)

The board must enable GPIO port clocks before calling Init.

If any pin configuration fails, Init should stop and return an error.

### Deinit

Reset GPIO pin configurations as needed. The board is responsible for
disabling GPIO port clocks after Deinit.

### Get

Read the current state of a pin. The `pin` parameter is an index into the
configured pin table, not a raw hardware pin number. The driver should look up
the actual port and pin from the configuration.

For output pins, read the output data register (the value being driven). For
input pins, read the input data register (the value being sampled from the
pad). Store the result (0 or 1) in the output pointer.

### Set

Drive a pin to the given value (0 or 1). The `pin` parameter is an index into
the configured pin table. The driver writes to the output data register for the
corresponding port and pin.

---

## UART

Header: `wolfHAL/uart/uart.h`

The UART driver provides basic serial transmit and receive. All operations are
blocking — Send does not return until all bytes have been transmitted, and Recv
does not return until all requested bytes have been received.

### Init

Configure and enable the UART peripheral:

- Write the baud rate register value from the configuration. The board
  pre-computes this value from the clock frequency and desired baud rate
  (e.g., `BRR = clockFreq / baud`)
- Configure word length, stop bits, and parity as needed
- Enable the transmitter and receiver
- Enable the UART peripheral

The board must enable the peripheral clock before calling Init.

On platforms with synchronization requirements (e.g., Microchip SERCOM), the
driver must poll synchronization busy flags after writing to certain registers
before proceeding.

### Deinit

Disable the UART peripheral:

- Disable the transmitter and receiver
- Clear the baud rate register

### Send

Transmit `dataSz` bytes from the provided buffer. For each byte:

1. Poll the transmit-ready flag (TX empty / data register empty) until the
   hardware is ready to accept a byte
2. Write the byte to the transmit data register

After sending all bytes, poll the transmission-complete flag to ensure the last
byte has fully shifted out before returning.

### Recv

Receive `dataSz` bytes into the provided buffer. For each byte:

1. Poll the receive-ready flag (RX not empty / receive complete) until a byte
   is available
2. Read the byte from the receive data register and store it in the buffer

### SendAsync

Start a non-blocking transmit. Returns immediately after initiating the
transfer. The buffer must remain valid until the transfer completes. The
driver signals completion through a platform-specific mechanism.

Drivers that do not support async should set SendAsync to NULL in the vtable.
The dispatch layer returns WHAL_ENOTSUP when the caller tries to use any
NULL vtable entry (or when the driver pointer itself is NULL). When direct
API mapping is active, polled drivers provide stub implementations that
return WHAL_ENOTSUP directly.

### RecvAsync

Start a non-blocking receive. Returns immediately after initiating the
transfer. The buffer must remain valid until the transfer completes.

The async variants are optional — a driver vtable only needs to populate
them if the platform supports non-blocking transfers. Polled-only drivers
leave these NULL (the dispatch layer returns WHAL_ENOTSUP) or provide
stubs returning WHAL_ENOTSUP (direct API mapping).

---

## IRQ

Header: `wolfHAL/irq/irq.h`

The IRQ driver controls an interrupt controller. It provides a
platform-independent way to enable and disable individual interrupt lines.

### Init

Initialize the interrupt controller.

### Deinit

Shut down the interrupt controller.

### Enable

Enable an interrupt line. The `irqCfg` parameter is platform-specific and
can contain settings such as priority. Pass NULL for defaults.

### Disable

Disable an interrupt line.

---

## SPI

Header: `wolfHAL/spi/spi.h`

The SPI driver provides serial peripheral interface communication. A
communication session is bracketed by `StartCom` / `EndCom`, which configure
the peripheral for a specific mode and speed. All transfers within a session
use the same settings, allowing the bus to be shared between devices with
different configurations by starting a new session for each device.

### Init

Perform one-time SPI peripheral configuration that remains fixed for the
lifetime of the device. Do not configure mode, baud rate, or data size
here — these are applied per-session via `StartCom`.

The board must enable the peripheral clock before calling Init. The
configuration struct should include the peripheral clock frequency so the
driver can compute baud rate prescalers.

### Deinit

Disable the SPI peripheral.

### StartCom

Begin a communication session. Configures the peripheral from the
platform-independent `whal_Spi_ComCfg` struct:

- `freq` — bus frequency in Hz
- `mode` — SPI mode (CPOL/CPHA)
- `wordSz` — word size in bits (e.g. 8)
- `dataLines` — number of data lines (1 for standard SPI)

The driver should disable the peripheral, apply the new settings (mode, baud
rate prescaler, data size, FIFO threshold), and re-enable it. Return
`WHAL_EINVAL` if a requested setting is not supported by the hardware.

### EndCom

End the current communication session by disabling the peripheral.

### SendRecv

Perform a full-duplex SPI transfer. This is the only transfer function — there
are no separate Send or Recv operations.

The driver clocks `max(txLen, rxLen)` bytes:

- When `tx` is NULL or exhausted, send `0xFF` for remaining clocks
- When `rx` is NULL or exhausted, discard received bytes
- Every TX byte produces an RX byte; always drain the RX FIFO to prevent
  overflow

After the loop, wait for the bus to go idle before returning.

---

## I2C

Header: `wolfHAL/i2c/i2c.h`

The I2C driver provides inter-integrated circuit bus communication. A
communication session is bracketed by `StartCom` / `EndCom`, which configure
the peripheral for a specific target address and bus frequency. The message-based
`Transfer` API gives full control over bus conditions (START, STOP, direction).

Convenience helpers `whal_I2c_WriteReg` and `whal_I2c_ReadReg` are provided
as inline functions for the common register read/write patterns.

### Init

Perform one-time I2C peripheral configuration (noise filters, enable). Do not
configure timing or address here — these are applied per-session via `StartCom`.

The board must enable the peripheral clock before calling Init.

### Deinit

Disable the I2C peripheral.

### StartCom

Begin a communication session. Configures the peripheral from the
platform-independent `whal_I2c_ComCfg` struct:

- `freq` — bus frequency in Hz (100000, 400000, 1000000)
- `addr` — target device address (7-bit or 10-bit)
- `addrSz` — address size in bits (7 or 10)

The driver should compute timing parameters from `freq` and the peripheral
clock, then write the target address into the appropriate hardware register.

### EndCom

End the current communication session by clearing the target address.

### Transfer

Execute a sequence of I2C messages. Each message has a data buffer, size, and
flags that control bus conditions:

- `WHAL_I2C_MSG_WRITE` (0) — master write
- `WHAL_I2C_MSG_READ` — master read
- `WHAL_I2C_MSG_START` — generate START (or repeated START)
- `WHAL_I2C_MSG_STOP` — generate STOP after this message

The driver processes messages sequentially. When the current message has no
STOP and the next message has no START, use hardware RELOAD to continue the
same transfer without re-addressing (for multi-part writes). When the next
message has START, use TC (transfer complete) so the hardware can generate a
repeated START for direction changes.

For messages larger than 255 bytes, the driver must split into chunks using
the hardware RELOAD mechanism internally.

---

## Sensor

Header: `wolfHAL/sensor/sensor.h`

The sensor driver provides a bus-agnostic API for reading sensor data. Each
sensor driver implements the vtable and uses the appropriate bus (I2C, SPI,
etc.) internally. The `Read` function fills a driver-defined data struct
passed as a void pointer.

### Init

Initialize the sensor hardware. This may involve loading configuration data,
verifying device identity, and configuring operating parameters. The driver
should call `StartCom` / `EndCom` on its bus device to bracket I2C/SPI access.

### Deinit

Shut down the sensor (e.g., soft reset). Should bracket bus access with
`StartCom` / `EndCom`.

### Read

Read sensor data into a driver-defined struct. The driver fetches a new sample
from the hardware and fills the struct. The struct type is defined by each
sensor driver (e.g., `whal_Bmi270_Data` with `accelX/Y/Z`, `gyroX/Y/Z`).

Each `Read` call should bracket its bus access with `StartCom` / `EndCom` so
the bus is released between reads and other devices can use it.

---

## Flash

Header: `wolfHAL/flash/flash.h`

The flash driver provides access to on-chip or external flash memory. Flash
has specific constraints around alignment, erase-before-write, and region
protection that the driver must handle.

### Init

Initialize the flash controller. This may involve clearing error flags or
releasing hardware mutex locks. The board must enable the flash interface clock
before calling Init.

### Deinit

Release flash controller resources.

### Lock

Write-protect a flash region to prevent modification. On some platforms this is
a global lock (the `addr` and `len` parameters are ignored and the entire flash
is locked). On others it may protect specific regions. After locking, Write and
Erase operations on the protected region should fail until Unlock is called.

The implementation varies significantly by platform — some use an unlock key
sequence (where Lock simply sets a lock bit), while others have dedicated
write-protect registers for individual regions.

### Unlock

Remove write protection to allow Write and Erase operations. On platforms that
use a key sequence, this typically involves writing two specific magic values to
a key register in the correct order. An incorrect sequence may trigger a bus
fault (this is a hardware security feature).

### Read

Read data from flash. On most platforms, flash is memory-mapped, so this is a
straightforward memory copy from the flash address into the provided buffer. No
special flash controller interaction is needed.

Some platforms may require acquiring a mutex or performing cache maintenance
before reading.

### Write

Program data into flash starting at the given address. The caller must ensure
the region is unlocked and erased before writing (flash can only transition bits
from 1 to 0; erasing sets all bits to 1).

Key constraints:
- **Alignment**: writes must be aligned to the platform's programming unit
  (e.g., 8-byte double-word on STM32, 8 or 32 bytes on PIC32CZ). The driver
  should validate alignment and return an error if misaligned.
- **Programming unit**: the driver writes data in hardware-defined chunks. Some
  platforms support multiple write sizes (e.g., single double-word vs. quad
  double-word) and the driver can optimize by using larger writes when
  alignment permits.
- **Busy polling**: after each write, poll the flash controller's busy flag
  until the operation completes.
- **Error flags**: clear any pending error flags before starting, and check for
  new errors after each operation.

### Erase

Erase a flash region. Flash erase operates at sector/page granularity
(typically 4 KB). The driver should:

- Calculate the page range covering the requested address and size
- Erase each page individually, polling for completion between pages
- Validate that the region is unlocked before erasing

The `addr` does not need to be page-aligned — the driver should erase all pages
that overlap with the requested range. Peripheral flash drivers (e.g., SPI-NOR)
may enforce stricter alignment requirements where the underlying hardware
requires aligned erase addresses.

---

## Block

Header: `wolfHAL/block/block.h`

The block driver provides access to block-addressable storage devices such as
SD cards and eMMC. Unlike flash, block devices are addressed by block number
rather than byte address, and all operations work in units of fixed-size blocks
(e.g. 512 bytes).

Block drivers are peripheral drivers — they call their underlying bus driver
(SPI, SDIO) to communicate with the storage device.

### Init

Initialize the storage device. This includes any device-specific initialization
sequence required to bring the device into a usable state. The board must have
already initialized the bus driver and enabled the relevant clocks and GPIOs
before calling Init.

### Deinit

Release the storage device.

### Read

Read one or more blocks from the device into a buffer. The driver should handle
both single-block and multi-block reads based on the block count.

### Write

Write one or more blocks from a buffer to the device. The driver should handle
both single-block and multi-block writes, and wait for the device to finish
programming before returning.

### Erase

Erase a range of blocks on the device.

---

## Timer

Header: `wolfHAL/timer/timer.h`

The timer driver provides periodic tick or counter functionality. The most
common implementation is the ARM Cortex-M SysTick timer, which provides a
simple periodic interrupt for system timekeeping.

### Init

Configure the timer hardware but do **not** start it. This includes:

- Setting the reload value (determines the tick interval:
  interval = reload / clock_frequency)
- Selecting the clock source (e.g., CPU clock vs. external reference)
- Enabling or disabling the tick interrupt

The timer should not begin counting until Start is called.

### Deinit

Stop the timer and release resources.

### Start

Start the timer counting. The timer begins decrementing from the configured
reload value, generating interrupts (if enabled) each time it reaches zero and
reloading automatically.

### Stop

Stop the timer without resetting it. The counter holds its current value.

### Reset

Reset the timer counter back to its initialized state.

---

## PWM

Header: `wolfHAL/pwm/pwm.h`

The PWM driver generates a pulse-width-modulated output. Each channel's waveform
(frequency, duty, polarity) is described by a `whal_Pwm_ChannelCfg` passed to
Start, which applies it and begins output; Stop halts the channel. Channels are
addressed by a zero-based index, and a driver returns `WHAL_ENOTSUP` for any
channel its hardware does not implement (a single-output peripheral accepts only
channel 0).

The waveform's `periodCycles` and `pulseCycles` are counted in ticks of the
driver's (prescaled) timer clock, not seconds; the board translates real time
into ticks. On hardware that shares one counter across channels, every channel
of an instance must use the same `periodCycles`.

### Init

Configure the instance-static PWM settings (clock source, prescaler, waveform
mode) with the output disabled. The board must enable the peripheral clock
before calling Init.

### Deinit

Disable the PWM output and release resources.

### Start

Apply a `whal_Pwm_ChannelCfg` to a channel and begin output. The generic
dispatcher rejects a null waveform, a zero period, and
`pulseCycles > periodCycles` with `WHAL_EINVAL`; the driver returns
`WHAL_ENOTSUP` for a channel or waveform its hardware cannot represent. Start is
a repeatable toggle with Stop and does not require a re-Init.

### Stop

Halt output on a channel, parking the pin. A subsequent Start reprograms and
resumes the waveform.

---

## Display

Header: `wolfHAL/display/display.h`

The display driver provides a bus-agnostic API for pixel-addressable panels.
Each driver implements the vtable and talks to its panel over the appropriate
bus (SPI, parallel, etc.) internally. The pixel format, the byte layout of the
data buffer, and the meaning of a region are defined by each driver rather than
by the generic API.

### Init

Bring the panel up: configure the bus session, start any refresh / COM-inversion
source the panel needs, and clear pixel memory to a known state. A driver that
acquires resources here (e.g. a VCOM PWM) should release them on any Init error
path so a failed Init leaves nothing running.

### Deinit

Shut the panel down, mirroring Init's bring-up in reverse (e.g. blank the panel
before halting COM inversion, then release the bus).

### Update

Push pixel data to the `w` by `h` region whose top-left corner is (`x`, `y`).
The generic dispatcher rejects structural violations (null pointers, a zero-area
region, a null or zero-length data buffer) with `WHAL_EINVAL`; the driver
returns `WHAL_ENOTSUP` for a region its hardware cannot address (e.g. a panel
that can only write whole lines requires a full-width region) and validates
`dataSz` against its own pixel format.

---

## RNG

Header: `wolfHAL/rng/rng.h`

The RNG driver provides access to a hardware random number generator. The
hardware typically uses an analog entropy source to produce true random numbers.

### Init

Initialize the RNG hardware. The board must enable the peripheral clock (and
any additional clock sources the RNG's entropy source requires) before calling
Init.

### Deinit

Shut down the RNG hardware.

### Generate

Fill the provided buffer with random data. The driver should:

1. Enable the RNG peripheral
2. Loop until the buffer is filled:
   - Check for hardware errors (seed errors, clock errors). If the entropy
     source has failed, disable the RNG and return `WHAL_EHARDWARE`
   - Poll the data-ready flag until a new random word is available
   - Read the random word (typically 32 bits) and extract as many bytes as
     needed into the output buffer
3. Disable the RNG peripheral before returning

The RNG is enabled only for the duration of the Generate call to minimize power
consumption and avoid unnecessary entropy source wear.

---

## Crypto

Header: `wolfHAL/crypto/crypto.h`

The crypto subsystem provides access to hardware cryptographic accelerators
(ciphers, hashes, MACs, public-key math). Unlike other device types, crypto
uses a two-level device model: a **hardware device** (`whal_Crypto`) for
peripheral lifecycle, and **per-algorithm device structs** (`whal_AesGcm`,
`whal_Sha256`, etc.) that carry typed vtables with direct function
arguments.

Public-key math accelerators (PKA) are an exception: lifecycle still goes
through `whal_Crypto`, but the math operations themselves are exposed as
**direct functions on big-endian operand byte arrays** — no per-algorithm
device struct, no vtable. See the *Math Primitives* subsection below.

### Architecture

`whal_Crypto` is a platform driver representing a crypto hardware peripheral.
It provides only `Init` and `Deinit` for hardware lifecycle (enable/disable
the peripheral, clear error flags). Each algorithm gets its own typed device
struct that references the underlying `whal_Crypto` and a per-algorithm
driver vtable. There is no `opId`, no `void*` casting, and no args structs
in the dispatch path.

### Hardware Device

```c
struct whal_Crypto {
    const size_t base;
    const whal_CryptoDriver *driver;
    const void *cfg;
};

typedef struct {
    whal_Error (*Init)(whal_Crypto *dev);
    whal_Error (*Deinit)(whal_Crypto *dev);
} whal_CryptoDriver;
```

The board must enable the peripheral clock before calling `whal_Crypto_Init`.
`whal_Crypto_Deinit` disables the crypto accelerator peripheral.

### Per-Algorithm Device Structs

Each algorithm has its own device struct with a typed vtable. The struct
shape varies by algorithm category:

**Block ciphers (ECB, CBC, CTR)** — no streaming state needed:

```c
struct whal_AesCbc {
    whal_Crypto *crypto;
    const whal_AesCbcDriver *driver;
};
```

**AEAD modes (GCM, CCM) and HMAC** — streaming state for multi-phase ops:

```c
struct whal_AesGcm {
    whal_Crypto *crypto;
    const whal_AesGcmDriver *driver;
    void *state;
};
```

**Hash algorithms (SHA-1, SHA-224, SHA-256)** — no streaming state (the
hardware retains intermediate context internally):

```c
struct whal_Sha256 {
    whal_Crypto *crypto;
    const whal_Sha256Driver *driver;
};
```

The `.crypto` field points to the underlying hardware device. The `.driver`
field points to the per-algorithm vtable. The `.state` field, when present,
points to a driver-defined struct that the driver uses to carry data between
streaming phases (e.g., accumulated AAD/data sizes for GCM's final GHASH, or
the HMAC key for the outer hash pass).

### Per-Algorithm Vtables

Each algorithm defines a vtable with typed function pointers. The available
operations depend on the algorithm category:

**Block ciphers** have `Oneshot`, `Start`, and `Process`:

```c
typedef struct {
    whal_Error (*Oneshot)(whal_AesCbc *dev, whal_Crypto_Dir dir,
                          const void *key, size_t keySz,
                          const void *iv,
                          const void *in, void *out, size_t sz);
    whal_Error (*Start)(whal_AesCbc *dev, whal_Crypto_Dir dir,
                        const void *key, size_t keySz,
                        const void *iv);
    whal_Error (*Process)(whal_AesCbc *dev,
                          const void *in, void *out, size_t sz);
} whal_AesCbcDriver;
```

**AEAD modes** add `Finalize` for tag generation:

```c
typedef struct {
    whal_Error (*Oneshot)(whal_AesGcm *dev, whal_Crypto_Dir dir,
                          const void *key, size_t keySz,
                          const void *iv, size_t ivSz,
                          const void *aad, size_t aadSz,
                          const void *in, void *out, size_t sz,
                          void *tag, size_t tagSz);
    whal_Error (*Start)(whal_AesGcm *dev, whal_Crypto_Dir dir,
                        const void *key, size_t keySz,
                        const void *iv, size_t ivSz,
                        const void *aad, size_t aadSz);
    whal_Error (*Process)(whal_AesGcm *dev,
                          const void *in, void *out, size_t sz);
    whal_Error (*Finalize)(whal_AesGcm *dev,
                           void *tag, size_t tagSz);
} whal_AesGcmDriver;
```

**Hash algorithms** have `Oneshot`, `Start`, `Process`, and `Finalize`:

```c
typedef struct {
    whal_Error (*Oneshot)(whal_Sha256 *dev,
                          const void *in, size_t inSz,
                          void *digest, size_t digestSz);
    whal_Error (*Start)(whal_Sha256 *dev);
    whal_Error (*Process)(whal_Sha256 *dev, const void *in, size_t inSz);
    whal_Error (*Finalize)(whal_Sha256 *dev, void *digest, size_t digestSz);
} whal_Sha256Driver;
```

**HMAC** adds key parameters to `Oneshot` and `Start`:

```c
typedef struct {
    whal_Error (*Oneshot)(whal_HmacSha256 *dev,
                          const void *key, size_t keySz,
                          const void *in, size_t inSz,
                          void *digest, size_t digestSz);
    whal_Error (*Start)(whal_HmacSha256 *dev,
                        const void *key, size_t keySz);
    whal_Error (*Process)(whal_HmacSha256 *dev,
                          const void *in, size_t inSz);
    whal_Error (*Finalize)(whal_HmacSha256 *dev,
                           void *digest, size_t digestSz);
} whal_HmacSha256Driver;
```

**Oneshot-only algorithms** (e.g., GMAC) have only `Oneshot`:

```c
typedef struct {
    whal_Error (*Oneshot)(whal_AesGmac *dev,
                          const void *key, size_t keySz,
                          const void *iv, size_t ivSz,
                          const void *aad, size_t aadSz,
                          void *tag, size_t tagSz);
} whal_AesGmacDriver;
```

### Operations

- **Oneshot** — Performs the entire operation in a single call. Handles key
  loading, data processing, and finalization internally.
- **Start** — Begin a streaming session. Load the key (and IV/AAD for modes
  that need them). For AEAD modes, this covers the init and header phases.
- **Process** — Feed data through an active session. May be called multiple
  times. For block ciphers this processes blocks of ciphertext/plaintext.
  For hash/HMAC this feeds message data.
- **Finalize** — End a streaming session and produce the final output (tag
  for AEAD, digest for hash/HMAC). Only present on algorithms that need a
  finalization step. Block ciphers (ECB, CBC, CTR) do not have Finalize
  because each Process call produces complete output.

Unsupported parameter combinations (e.g. AES-192 on hardware that only
supports 128/256) return `WHAL_ENOTSUP`.

### The .state Field

AEAD (GCM, CCM) and HMAC device structs include a `void *state` field that
points to a driver-defined struct. The driver uses this to carry data between
`Start`, `Process`, and `Finalize` calls. For example:

- **AES-GCM**: e.g. `whal_Stm32wb_AesGcm_State` tracks accumulated AAD and
  data sizes for the final-phase GHASH computation.
- **AES-CCM**: e.g. `whal_Stm32wb_AesCcm_State` tracks accumulated AAD and
  data sizes for tag construction.
- **HMAC**: driver-specific state retains the key pointer for the outer hash
  pass (e.g., `whal_Stm32wba_HmacSha256_State`).

Block ciphers and plain hash algorithms do not need `.state` because the
hardware retains all necessary context internally.

### API Usage

Applications reach each algorithm through its `BOARD_<ALGO>_DEV` macro
(`BOARD_AES_GCM_DEV`, `BOARD_SHA256_DEV`, etc.). Boards point those at
`WHAL_INTERNAL_DEV` for the common single-instance case or at a
`&g_whalAesGcm` pointer if they have kept the device in `wolfHAL_board.c`.

One-shot:

```c
whal_AesGcm_Oneshot(BOARD_AES_GCM_DEV, WHAL_CRYPTO_ENCRYPT,
                    key, 32, iv, 12, aad, aadSz,
                    pt, ct, ptSz, tag, 16);
```

Streaming (AEAD):

```c
whal_AesGcm_Start(BOARD_AES_GCM_DEV, WHAL_CRYPTO_ENCRYPT,
                  key, 32, iv, 12, aad, aadSz);
whal_AesGcm_Process(BOARD_AES_GCM_DEV, chunk1, out1, chunk1Sz);
whal_AesGcm_Process(BOARD_AES_GCM_DEV, chunk2, out2, chunk2Sz);
whal_AesGcm_Finalize(BOARD_AES_GCM_DEV, tag, 16);
```

Streaming (hash):

```c
whal_Sha256_Start(BOARD_SHA256_DEV);
whal_Sha256_Process(BOARD_SHA256_DEV, chunk1, chunk1Sz);
whal_Sha256_Process(BOARD_SHA256_DEV, chunk2, chunk2Sz);
whal_Sha256_Finalize(BOARD_SHA256_DEV, digest, 32);
```

Streaming (block cipher):

```c
whal_AesCbc_Start(BOARD_AES_CBC_DEV, WHAL_CRYPTO_ENCRYPT, key, 32, iv);
whal_AesCbc_Process(BOARD_AES_CBC_DEV, block1, out1, 16);
whal_AesCbc_Process(BOARD_AES_CBC_DEV, block2, out2, 16);
```

### Math Primitives

Header: `wolfHAL/crypto/pka.h`

Public-key accelerators (PKA, BIGINT, etc.) expose **stateless math
primitives** over multi-precision integers rather than algorithm-typed
streaming sessions. Each call is one-shot: load operands, run one hardware
opcode, read the result. wolfHAL surfaces these as direct functions on
big-endian byte arrays:

```c
whal_Error whal_Pka_ModExp(const uint8_t *A, size_t ASz,
                           const uint8_t *e, size_t eSz,
                           const uint8_t *N, size_t NSz,
                                 uint8_t *result, size_t resultSz);

whal_Error whal_Pka_ModInv   (...);
whal_Error whal_Pka_ModReduce(...);
whal_Error whal_Pka_IntMul   (...);
whal_Error whal_Pka_IntSub   (...);
whal_Error whal_Pka_RsaCrtExp(...);
```

There is no `whal_Pka` device struct, no per-primitive vtable, and no
device handle passed to the call. The platform driver owns a single
`whal_Crypto` for the PKA peripheral's lifecycle (`Init`/`Deinit`,
enable/disable, error flag clearing); the math primitives reach the
peripheral through that internal device.

**Why no algorithm-typed device?** The primitives are single-call,
share no state between invocations, and the same hardware opcode set is
the only useful "algorithm" a PKA exposes. A vtable per primitive would
just be six 1-entry tables.

**RAM hygiene.** A PKA driver should zero every operand slot it writes
on the success path of each primitive. The peripheral RAM is small and
shared across opcodes — leaving operand or result bytes in place can
corrupt the next call (operands shorter than the configured op length
read stale data from the unwritten high words) and leaks sensitive
material (private exponents, primes, signature results) until the next
op overwrites those slots.

**Direct API mapping.** The platform-prefixed primitives
(`whal_Stm32wb_Pka_ModExp` etc.) are aliased to the generic
`whal_Pka_*` names via `WHAL_CFG_<PLAT>_PKA_DIRECT_API_MAPPING`, the
same pattern as other crypto algorithms. The macros live in the driver
`.c` (not the header) so the binary exports only the generic names.

**Reference:** `wolfHAL/crypto/stm32wb_pka.h` and
`src/crypto/stm32wb_pka.c`.

### Writing a Crypto Driver

A single platform driver file typically implements multiple algorithms that
share the same hardware peripheral. For example, `stm32wb_aes.c` implements
AES-ECB, AES-CBC, AES-CTR, AES-GCM, AES-GMAC, and AES-CCM. Functions for
all operations of a single algorithm should be grouped together, not spread
by operation type.

The driver must:

1. Implement the `whal_CryptoDriver` vtable (`Init`/`Deinit`) for the
   hardware peripheral.
2. Implement per-algorithm vtables for each supported algorithm.
3. Define any streaming state structs needed for AEAD/HMAC.

#### Hardware Init/Deinit

These go through the `whal_CryptoDriver` vtable on `whal_Crypto`. They
handle peripheral enable/disable, clearing error flags, etc.

```c
whal_Error whal_Myplatform_Aes_Init(whal_Crypto *dev)
{
    /* Enable peripheral, clear flags */
}

whal_Error whal_Myplatform_Aes_Deinit(whal_Crypto *dev)
{
    /* Disable peripheral */
}
```

#### Per-Algorithm Functions

Each algorithm function receives the per-algorithm device struct. To access
hardware registers, dereference `dev->crypto->base`. To access the config,
cast `dev->crypto->cfg`.

```c
whal_Error whal_Myplatform_AesGcm_Oneshot(whal_AesGcm *dev,
                                          whal_Crypto_Dir dir,
                                          const void *key, size_t keySz,
                                          const void *iv, size_t ivSz,
                                          const void *aad, size_t aadSz,
                                          const void *in, void *out, size_t sz,
                                          void *tag, size_t tagSz)
{
    size_t base = dev->crypto->base;
    whal_Myplatform_Aes_Cfg *cfg =
        (whal_Myplatform_Aes_Cfg *)dev->crypto->cfg;
    /* Load key, IV, process AAD, encrypt/decrypt, compute tag */
}
```

For streaming state, the driver reads and writes `dev->state`:

```c
whal_Error whal_Myplatform_AesGcm_Start(whal_AesGcm *dev, ...)
{
    whal_Myplatform_AesGcm_State *st = (whal_Myplatform_AesGcm_State *)dev->state;
    st->aadSz = aadSz;
    st->dataSz = 0;
    /* Load key, IV, process AAD header */
}

whal_Error whal_Myplatform_AesGcm_Process(whal_AesGcm *dev,
                                          const void *in, void *out, size_t sz)
{
    whal_Myplatform_AesGcm_State *st = (whal_Myplatform_AesGcm_State *)dev->state;
    st->dataSz += sz;
    /* Process payload blocks */
}

whal_Error whal_Myplatform_AesGcm_Finalize(whal_AesGcm *dev,
                                           void *tag, size_t tagSz)
{
    whal_Myplatform_AesGcm_State *st = (whal_Myplatform_AesGcm_State *)dev->state;
    /* Use st->aadSz and st->dataSz for final GHASH, produce tag */
}
```

#### Vtable Definitions

Define one vtable per algorithm, plus one `whal_CryptoDriver` for
Init/Deinit. Wrap each in `#ifndef` for the corresponding direct API
mapping flag:

```c
#ifndef WHAL_CFG_MYPLATFORM_AES_INIT_DIRECT_API_MAPPING
const whal_CryptoDriver whal_Myplatform_Aes_CryptoDriver = {
    .Init   = whal_Myplatform_Aes_Init,
    .Deinit = whal_Myplatform_Aes_Deinit,
};
#endif

#ifndef WHAL_CFG_MYPLATFORM_AES_GCM_DIRECT_API_MAPPING
const whal_AesGcmDriver whal_Myplatform_Aes_GcmDriver = {
    .Oneshot  = whal_Myplatform_AesGcm_Oneshot,
    .Start    = whal_Myplatform_AesGcm_Start,
    .Process  = whal_Myplatform_AesGcm_Process,
    .Finalize = whal_Myplatform_AesGcm_Finalize,
};
#endif
```

### Direct API Mapping

Crypto uses per-algorithm direct API mapping flags. Each algorithm can be
independently mapped. The flags follow the pattern
`WHAL_CFG_<DRIVER>_<ALGO>_DIRECT_API_MAPPING`:

```c
/* In the driver header */
#ifdef WHAL_CFG_MYPLATFORM_AES_GCM_DIRECT_API_MAPPING
#define whal_Myplatform_AesGcm_Oneshot  whal_AesGcm_Oneshot
#define whal_Myplatform_AesGcm_Start    whal_AesGcm_Start
#define whal_Myplatform_AesGcm_Process  whal_AesGcm_Process
#define whal_Myplatform_AesGcm_Finalize whal_AesGcm_Finalize
#endif

#ifdef WHAL_CFG_MYPLATFORM_AES_INIT_DIRECT_API_MAPPING
#define whal_Myplatform_Aes_Init        whal_Crypto_Init
#define whal_Myplatform_Aes_Deinit      whal_Crypto_Deinit
#endif
```

The Init/Deinit mapping maps to `whal_Crypto_Init`/`whal_Crypto_Deinit`
(the hardware device API). Each algorithm mapping maps to that algorithm's
top-level API (e.g., `whal_AesGcm_Oneshot`).

### Adding a New Algorithm

To add support for a new algorithm (e.g., ChaCha20-Poly1305):

1. Add the per-algorithm device struct, vtable typedef, and API function
   declarations to `wolfHAL/crypto/crypto.h`. Include a `.state` field if
   the algorithm needs streaming state.
2. Add the dispatch functions to `src/crypto/crypto.c`.
3. Implement the vtable functions in the platform driver (e.g.,
   `src/crypto/myplatform_aes.c`).
4. Add direct API mapping guards in the driver header.
5. Add `WHAL_CFG_<ALGO>` config guards around the new API functions.

For stateless math primitives (PKA-style), follow the *Math Primitives*
pattern instead: declare the generic functions in `wolfHAL/crypto/pka.h`
(no dispatch layer needed — they're called directly), implement them in
the platform driver, and alias the platform-prefixed names to the
generic ones via the driver's direct-API-mapping flag.

### Board Integration

The board instantiates one `whal_Crypto` device per hardware peripheral, plus
one per-algorithm device for each algorithm it uses. The per-algorithm device
structs reference the `whal_Crypto` instance, the appropriate driver vtable,
and (for AEAD/HMAC) a state struct:

```c
/* Hardware device */
whal_Crypto g_whalCrypto = {
    .base = WHAL_MYPLATFORM_AES1_BASE,
    .driver = &whal_Myplatform_Aes_CryptoDriver,
    .cfg = &(whal_Myplatform_Aes_Cfg) { .timeout = &g_whalTimeout },
};

/* Block cipher — no state */
whal_AesCbc g_whalAesCbc = {
    .crypto = &g_whalCrypto,
    .driver = &whal_Myplatform_Aes_CbcDriver,
};

/* AEAD — with state for streaming */
static whal_Myplatform_AesGcm_State g_aesGcmState;
whal_AesGcm g_whalAesGcm = {
    .crypto = &g_whalCrypto,
    .driver = &whal_Myplatform_Aes_GcmDriver,
    .state = &g_aesGcmState,
};

/* Hash — no state (hardware retains context) */
whal_Sha256 g_whalSha256 = {
    .crypto = &g_whalHash,
    .driver = &whal_Myplatform_Hash_Sha256Driver,
};

/* HMAC — with driver-specific state */
static whal_Myplatform_HmacSha256_State g_hmacState;
whal_HmacSha256 g_whalHmacSha256 = {
    .crypto = &g_whalHash,
    .driver = &whal_Myplatform_Hash_HmacSha256Driver,
    .state = &g_hmacState,
};
```

When direct API mapping is active for an algorithm, the `.driver` field is
omitted from that per-algorithm device. When Init/Deinit mapping is active,
the `.driver` field is omitted from the `whal_Crypto` device.

Crypto drivers that are single-instance (most on-MCU AES/hash blocks
qualify, since the chip exposes one of each) follow the pattern in the
"Single-instance drivers" section above: the driver header
`extern`-declares each singleton (the `whal_Crypto` peripheral and each
per-algorithm `whal_<Plat>_<Algo>_Dev`), the driver `.c` defines them
from `WHAL_CFG_<PLAT>_<ALGO>_DEV` initializers in `wolfHAL_board.h`, and any
streaming state is a `static` variable in the driver `.c` whose address
the initializer plumbs into the `.state` field. The per-algorithm
initializer's `.crypto` is the cast address of the `whal_Crypto`
singleton. The board's `BOARD_<ALGO>_DEV` macro is then
`WHAL_INTERNAL_DEV`. See `boards/stm32wb55xx_nucleo/wolfHAL_board.h` for a
worked example.

### Reference Implementations

- **AES (ECB/CBC/CTR/GCM/GMAC/CCM)**: `wolfHAL/crypto/stm32wb_aes.h` and
  `src/crypto/stm32wb_aes.c`
- **Hash/HMAC (SHA-1/SHA-224/SHA-256, HMAC variants)**:
  `wolfHAL/crypto/stm32wba_hash.h` and `src/crypto/stm32wba_hash.c`
- **PKA math primitives (ModExp, ModInv, ModReduce, IntMul, IntSub,
  RsaCrtExp)**: `wolfHAL/crypto/stm32wb_pka.h` and
  `src/crypto/stm32wb_pka.c`

---

## Power

Power is a **board-level driver** (see Driver Categories). There is no
generic `power.h`, no `whal_Power` handle, no `whal_Power_Init`/`Deinit`/
`Enable`/`Disable` API, no `whal_PowerDriver` vtable. Each chip power
driver exposes imperative chip-specific helpers that boards call
directly from `Board_Init` (typically before clock setup, to bring up
regulators that downstream peripherals depend on). The driver header
owns the chip's fixed `_BASE` macro and the helpers take no device
pointer parameter.

### API contract

The chip power driver header declares chip-specific helpers named
`whal_<Chip>_<Subsys>_<Operation>(...)`, where `<Subsys>` is the chip's
power-controller name (e.g. `Pwr` on STM32L1, `Supc` on PIC32CZ). The set
of operations is whatever the chip actually exposes — there is no fixed
list. Examples:

- `whal_Stm32l1_Pwr_SetVosRange(range, timeout)` — voltage scaling range
  select with ready-bit poll
- `whal_Pic32cz_Supc_EnableSupply(const whal_Pic32cz_Supc_Supply *)`
  / `DisableSupply(...)` — toggle a regulator output identified by a
  descriptor (register mask + position)

NOT allowed in the public header: `Init`, `Deinit`, or any abstracted
"generic power" operation. Internal helpers go in the `.c` file as
`static`.

The chip header may declare types, enums, and descriptor-initializer
macros freely.

### Reference implementations

`wolfHAL/power/stm32l1_pwr.h` (single voltage-scaling helper) and
`wolfHAL/power/pic32cz_supc.h` (regulator enable/disable by descriptor)
are the canonical examples for the two common shapes.

### Board responsibilities

The board calls power helpers from `Board_Init` in the right order —
typically before clock configuration, since some clock nodes (e.g., a
PLL's analog regulator) require their supply to be enabled first. There
is no separate Init/Deinit step; helpers are imperative and only do what
they're called to do.

---

## Ethernet

Header: `wolfHAL/eth/eth.h`

The Ethernet driver controls a MAC (Media Access Controller) with an integrated
DMA engine. It manages descriptor rings in RAM for transmit and receive, handles
MDIO bus access for PHY communication, and configures the MAC for the negotiated
link speed and duplex.

### Init

Initialize the MAC, DMA, and MTL. Set up TX and RX descriptor rings in RAM,
program the MAC address, and configure DMA bus mode and burst lengths. Does NOT
enable TX/RX — call Start for that. The descriptor and buffer memory must be
pre-allocated by the board and passed via the config struct. Validate all config
fields (descriptor counts > 0, buffer pointers non-NULL, buffer sizes > 0).

### Deinit

Perform a DMA software reset to clear all state.

### Start

Configure the MAC speed and duplex to match the PHY, enable MAC TX/RX, start
the DMA TX and RX engines, and kick the RX DMA by writing the tail pointer.
Speed and duplex are passed as parameters — the board reads these from the PHY
driver before calling Start.

### Stop

Stop DMA TX and RX engines, then disable MAC TX and RX.

### Send

Transmit a single Ethernet frame. Find the next available TX descriptor (OWN=0),
copy the frame into the pre-allocated TX buffer, fill in the descriptor fields
(buffer address, length, OWN=1, FD, LD), and write the DMA tail pointer to kick
transmission. Return WHAL_ENOTREADY if no descriptor is available. Validate that
the frame length does not exceed the TX buffer size.

### Recv

Receive a single Ethernet frame by polling. Check the next RX descriptor — if
OWN=0, DMA has written a frame. Read the packet length from the descriptor, copy
the frame data to the caller's buffer, re-arm the descriptor (set OWN=1), and
update the tail pointer. Return WHAL_ENOTREADY if no frame is available, or
WHAL_EHARDWARE if the error summary bit is set.

### MdioRead

Read a 16-bit PHY register via the MDIO management bus. Write the PHY address,
register address, and read command to the MDIO address register, poll for
completion, then read the data register. Use a timeout to avoid infinite hang
if the PHY is not responding.

### MdioWrite

Write a 16-bit value to a PHY register via MDIO. Write the data first, then
issue the write command and poll for completion with a timeout.

---

## EthPhy

Header: `wolfHAL/eth_phy/eth_phy.h`

The Ethernet PHY driver handles link negotiation and status for an external PHY
chip connected to a MAC via the MDIO bus. The PHY device struct holds a pointer
to its parent MAC (for MDIO access) and the PHY address on the bus. Different
PHY chips (e.g., LAN8742A, DP83848) have different vendor-specific registers
but share the same API.

### Init

Reset the PHY via software reset (BCR bit 15), wait for the reset bit to
self-clear, then enable autonegotiation. Does not block waiting for link — the
board or application polls GetLinkState separately.

### Deinit

Power down the PHY or release resources. May be a no-op on simple PHYs.

### GetLinkState

Read the current link status, negotiated speed, and duplex mode. The IEEE 802.3
BSR register (reg 1) link bit is latching-low — read it twice and use the second
result for current status. Speed and duplex are read from a vendor-specific
status register (e.g., register 0x1F on LAN8742A). Return speed as 10 or 100,
and duplex as WHAL_ETH_DUPLEX_HALF or WHAL_ETH_DUPLEX_FULL.

---

## DMA

Header: `wolfHAL/dma/dma.h`

The DMA driver controls a DMA controller. A single device instance represents
one controller, and individual channels are identified by index. Channel
configuration is platform-specific and passed as an opaque pointer.

DMA is a service peripheral — peripheral drivers (UART, SPI) consume it
internally. The application never calls the DMA API directly. Peripheral
drivers receive a `whal_Dma` pointer and channel number through their
configuration struct and use them to set up transfers.

### Init

Initialize the DMA controller. Clear any pending interrupt flags and reset
controller state. The board must enable the DMA controller clock before calling
Init.

### Deinit

Shut down the DMA controller.

### Configure

Configure a DMA channel for transfers. The `chCfg` parameter is a
platform-specific struct containing:

- Transfer direction (memory-to-peripheral, peripheral-to-memory, etc.)
- Source and destination addresses
- Transfer width (8, 16, 32 bit)
- Buffer address and length
- Burst size (if supported)
- Peripheral request mapping (e.g., DMAMUX request ID)

The DMA driver does not store callbacks. Instead, the board defines ISR
entries in the vector table and calls the driver's IRQ handler (e.g.,
`whal_Stm32wb_Dma_IRQHandler()`), passing a callback and context pointer.
The IRQ handler checks and clears the interrupt flags, then invokes the
callback. Peripheral drivers expose their completion callbacks for the
board to wire up (e.g., `whal_Stm32wb_UartDma_TxCallback`).

Configure sets up all channel registers but does not start the transfer.
Call Start to begin. A channel can be reconfigured between transfers (e.g.,
to change the buffer address and length) by calling Configure again.

### Start

Start a previously configured DMA channel. This enables the channel,
beginning the transfer. The channel must have been configured via Configure
before calling Start.

### Stop

Stop a DMA channel. This aborts any in-progress transfer and disables the
channel. The peripheral driver should call Stop in its cleanup path or when
a transfer needs to be cancelled.

---

## Watchdog

Header: `wolfHAL/watchdog/watchdog.h`

The watchdog driver provides access to hardware watchdog timers that reset the
system if not periodically refreshed. On most hardware, the watchdog cannot be
stopped once started — only a system reset clears it.

### Init

Configure and start the watchdog. This typically involves:

- Setting the prescaler and reload/counter values from the configuration
- Enabling the watchdog peripheral

On some hardware (e.g., STM32 IWDG), the watchdog must be started before its
configuration registers can be written. The driver should handle this ordering
internally.

The board must enable any required clocks before calling Init. For example, the
STM32 WWDG requires an APB clock, while the IWDG requires the LSI oscillator.

The watchdog is NOT initialized in `Board_Init` — the test or application
controls when it starts, since once started it cannot be stopped.

### Deinit

Shut down the watchdog. On hardware where the watchdog cannot be stopped (e.g.,
STM32 IWDG), this function is a no-op.

### Refresh

Reload the watchdog counter to prevent a reset. Must be called periodically
within the configured timeout window. The exact mechanism is hardware-specific
(e.g., writing a reload key to IWDG_KR, or writing the counter value back to
WWDG_CR).

For window watchdogs, the refresh must occur within the valid window — refreshing
too early or too late triggers a reset. The driver does not enforce window timing;
it writes the reload value unconditionally and relies on the hardware to enforce
the window.
