---
name: generate-driver-template
description: Scaffold a new wolfHAL driver. For vtable device types (spi, i2c, uart, rng, gpio, dma, eth, …) it generates the wolfHAL/<type>/<name>_<type>.h header and src/<type>/<name>_<type>.c source with the direct-API-mapping #defines, stubbed driver functions, and the driver vtable at the bottom of the .c. For board-level device types (clock, power, …) it generates a single header-only wolfHAL/<type>/<name>_<type>.h with a _BASE macro and stubbed static-inline helpers.
argument-hint: <device_type> <device_name> [single|multi]
---

# Generate a wolfHAL driver template

Scaffold a new platform driver so the user only has to fill in register definitions and
function bodies. The output matches wolfHAL conventions exactly. There are **two modes**,
chosen automatically from the device type in Step 0:

- **Mode A — vtable driver** (spi, i2c, uart, rng, gpio, dma, eth, flash, watchdog, …):
  emits a `.h` + `.c` pair with a direct-API-mapping `#define` block, every vtable function
  stubbed out, and the driver vtable definition at the bottom of the `.c`.
- **Mode B — board-level header-only driver** (clock, power, …): emits a single `.h` with a
  fixed `_BASE` macro and stubbed `static inline` helpers. No `.c`, no vtable, no device handle.

This skill **only scaffolds one driver**. It does not touch `wolfHAL_board.h`, the platform header,
the Makefile, or CI. For a full chip-family port use `port-stm32-platform`.

## Inputs

1. `$1` **device_type** — the device class / folder under `wolfHAL/` and `src/`, e.g. `spi`,
   `i2c`, `uart`, `rng`, `gpio`, `dma`, `eth`, `clock`, `power`.
2. `$2` **device_name** — the platform/driver prefix that goes in the filename and symbol
   names, e.g. `stm32f4`, `stm32h5`, `pic32cz`. The driver files become
   `<device_name>_<device_type>.{h,c}`.
3. `$3` **instance model** — `single` or `multi`. **Mode A only**; ignored for Mode B
   (board-level drivers have no device instance). If a Mode A type is given without it, ask.
   - `single` — a single-instance peripheral (one per chip: RNG, GPIO, flash, watchdog, eth).
     The `.c` defines the `whal_<Plat>_<Type>_Dev` device instance unconditionally and every body
     reads `base`/`cfg` from it. Reference: `src/rng/stm32wb_rng.c`.
   - `multi` — a multi-instance peripheral (UART, SPI, I2C, DMA). The `.c` carries the optional
     `WHAL_CFG_<PLAT>_<TYPE>_SINGLE_INSTANCE` `#ifdef` machinery: each body bifurcates between a
     fixed-device-instance path and the default pointer-based path. Reference: `src/uart/stm32wb_uart.c`,
     `src/spi/stm32f4_spi.c`.

If `$1` or `$2` is missing, ask before generating.

## Step 0 — Classify the device type (Mode A vs Mode B)

Look for the generic API header `wolfHAL/<device_type>/<device_type>.h`:

- **It exists and defines a `whal_<Type>Driver` vtable typedef + a `whal_<Type>` device
  struct → Mode A** (vtable driver). Proceed to Mode A.
- **It does not exist** (e.g. there is no `wolfHAL/clock/clock.h` or `wolfHAL/power/power.h`),
  or it exists but defines only register/helper content with no vtable → **Mode B**
  (board-level header-only). Proceed to Mode B.

**crypto is the one type this skill does not scaffold** — it uses per-algorithm device structs
and per-algorithm direct-API mapping, not a single device vtable and not the board-level shape.
Stop and point the user at `wolfHAL/crypto/stm32wb_aes.h` / `stm32wba_hash.h` and the
`port-stm32-platform` skill's Crypto notes.

## Naming derivation (both modes)

From the two name inputs derive these tokens (apply consistently):

| Token | Rule | Example (`stm32f4`, `spi`) |
|---|---|---|
| `<type>`  | device_type, lowercase                 | `spi` |
| `<Type>`  | device_type, first letter capitalized (for Mode A, confirm against `whal_<Type>` / `whal_<Type>Driver` in the generic header) | `Spi` (`I2c` for i2c) |
| `<TYPE>`  | device_type, UPPERCASE                  | `SPI` |
| `<plat>`  | device_name, lowercase                  | `stm32f4` |
| `<Plat>`  | device_name, first letter capitalized   | `Stm32f4` |
| `<PLAT>`  | device_name, UPPERCASE                  | `STM32F4` |

> **Use the device-type token exactly as given — never substitute a chip-specific peripheral
> name.** For `clock` the file is `<plat>_clock.h` with symbols `whal_<Plat>_Clock_*` and base
> `WHAL_<PLAT>_CLOCK_BASE`; for `power`, `<plat>_power.h` / `whal_<Plat>_Power_*` /
> `WHAL_<PLAT>_POWER_BASE`. Some existing in-tree headers are named after the chip's peripheral
> block (`rcc`, `pwr`, `supc`); this skill standardizes on the generic device-type name instead.
> Do **not** emit `rcc`/`pwr`/`supc` in the generated filename, guard, base macro, or symbols.

Resulting symbol names:
- Functions: `whal_<Plat>_<Type>_<Func>` (e.g. `whal_Stm32f4_Spi_Init`)
- Config struct (Mode A): `whal_<Plat>_<Type>_Cfg`
- Driver vtable instance (Mode A): `whal_<Plat>_<Type>_Driver`
- Device instance (Mode A): `whal_<Plat>_<Type>_Dev`
- Base macro (Mode B): `WHAL_<PLAT>_<TYPE>_BASE`
- Header guard: `WHAL_<PLAT>_<TYPE>_H`
- Config macros (Mode A): `WHAL_CFG_<PLAT>_<TYPE>_DIRECT_API_MAPPING`, `WHAL_CFG_<PLAT>_<TYPE>_SINGLE_INSTANCE`, `WHAL_CFG_<PLAT>_<TYPE>_DEV`

> Mode A note on `<Type>` casing: do NOT guess — read it from the generic header. A few folders
> use a leaf token that differs from the folder name (e.g. the `watchdog` folder holds
> `iwdg`/`wwdg` drivers whose generic vtable is `whal_WatchdogDriver`). When the leaf token
> differs, take `<type>` from the driver filename the user wants and `<Type>`/`<TYPE>`/the generic
> struct from the actual `whal_<Type>` / `whal_<Type>Driver` typedef in `wolfHAL/<folder>/<folder>.h`.

---

# Mode A — vtable driver

## A1 — Read the generic API to discover the vtable

Read `wolfHAL/<device_type>/<device_type>.h`. Extract, verbatim:

1. The generic device struct name — `whal_<Type>` (e.g. `whal_Spi`). Note whether its `cfg`
   field is `void *` or `const void *` — the body casts must match (a `const void *cfg`
   requires a `const whal_<Plat>_<Type>_Cfg *` cast to stay `-Werror`-clean; see GPIO).
2. The vtable typedef — `whal_<Type>Driver` — and **every function-pointer member**: its name
   (`Init`, `Deinit`, `StartCom`, `SendRecv`, …) and its **full parameter list** including the
   exact parameter variable names (e.g. `whal_Spi *spiDev`, `const void *tx, size_t txLen`).
3. Any per-session config / message structs referenced in the signatures (e.g. `whal_Spi_ComCfg`)
   — you do not redefine these, but you reuse them verbatim in the stub signatures.

The set of vtable members is the exact set of functions you stub and the exact set of `.Member`
initializers in the vtable definition. Mirror their signatures precisely.

## A2 — Generate the header `wolfHAL/<type>/<plat>_<type>.h`

Start with the standard GPL file header (copy the 20-line block verbatim from any existing file,
updating the first comment line to `<plat>_<type>.h`). Then:

```c
#ifndef WHAL_<PLAT>_<TYPE>_H
#define WHAL_<PLAT>_<TYPE>_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/<type>/<type>.h>
#include <wolfHAL/timeout.h>

/*
 * @file <plat>_<type>.h
 * @brief <Plat> <Type> driver configuration.
 */

/*
 * @brief <Type> device configuration.
 */
typedef struct whal_<Plat>_<Type>_Cfg {
    /* TODO: platform clock, timeout, and any per-device fields */
    whal_Timeout *timeout;
} whal_<Plat>_<Type>_Cfg;

<DEVICE_INSTANCE_EXTERN_BLOCK>

#ifndef WHAL_CFG_<PLAT>_<TYPE>_DIRECT_API_MAPPING
/*
 * @brief Driver instance for <Plat> <Type> peripheral.
 */
extern const whal_<Type>Driver whal_<Plat>_<Type>_Driver;

<ONE_DOXYGEN_DECL_PER_VTABLE_MEMBER>
#endif /* !WHAL_CFG_<PLAT>_<TYPE>_DIRECT_API_MAPPING */

#endif /* WHAL_<PLAT>_<TYPE>_H */
```

If the device type does not poll hardware (e.g. GPIO), drop the `<wolfHAL/timeout.h>` include and
the `timeout` field — use a peripheral-appropriate placeholder instead.

`<DEVICE_INSTANCE_EXTERN_BLOCK>`:
- **single**: unconditional —
  ```c
  /*
   * @brief Fixed device instance. Defined in the driver TU
   * from the WHAL_CFG_<PLAT>_<TYPE>_DEV initializer in wolfHAL_board.h.
   */
  extern const whal_<Type> whal_<Plat>_<Type>_Dev;
  ```
- **multi**: guarded on the single-instance flag —
  ```c
  /*
   * @brief Fixed device instance. Defined in the driver TU
   * from the WHAL_CFG_<PLAT>_<TYPE>_DEV initializer in wolfHAL_board.h.
   */
  #if defined(WHAL_CFG_<PLAT>_<TYPE>_SINGLE_INSTANCE)
  extern const whal_<Type> whal_<Plat>_<Type>_Dev;
  #endif
  ```

`<ONE_DOXYGEN_DECL_PER_VTABLE_MEMBER>`: for every vtable member, emit a doxygen block and the
prototype `whal_Error whal_<Plat>_<Type>_<Member>(<same params as the generic vtable slot>);`.

## A3 — Generate the source `src/<type>/<plat>_<type>.c`

Standard GPL file header (first line `<plat>_<type>.c`), then:

```c
#include <stdint.h>
<BOARD_H_INCLUDE>
#include <wolfHAL/<type>/<plat>_<type>.h>
#include <wolfHAL/<type>/<type>.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/*
 * <Plat> <Type> Register Definitions
 *
 * TODO: add register offsets, bit positions, and masks from the TRM.
 */

#ifdef WHAL_CFG_<PLAT>_<TYPE>_DIRECT_API_MAPPING
<ONE_DEFINE_PER_VTABLE_MEMBER>
#endif /* WHAL_CFG_<PLAT>_<TYPE>_DIRECT_API_MAPPING */

<DEVICE_INSTANCE_DEFINITION>

<ONE_STUBBED_FUNCTION_PER_VTABLE_MEMBER>

#ifndef WHAL_CFG_<PLAT>_<TYPE>_DIRECT_API_MAPPING
const whal_<Type>Driver whal_<Plat>_<Type>_Driver = {
<ONE_INITIALIZER_PER_VTABLE_MEMBER>
};
#endif /* !WHAL_CFG_<PLAT>_<TYPE>_DIRECT_API_MAPPING */
```

`<BOARD_H_INCLUDE>`:
- **single**: `#include "wolfHAL_board.h"  /* provides WHAL_CFG_<PLAT>_<TYPE>_DEV initializer */`
- **multi**:
  ```c
  #ifdef WHAL_CFG_<PLAT>_<TYPE>_SINGLE_INSTANCE
  #include "wolfHAL_board.h"  /* provides whal_<Plat>_<Type>_Dev device instance (possibly via platform alias macro) */
  #endif
  ```

`<ONE_DEFINE_PER_VTABLE_MEMBER>`: `#define whal_<Plat>_<Type>_<Member> whal_<Type>_<Member>`
(align the columns, one per member).

`<DEVICE_INSTANCE_DEFINITION>`:
- **single**: unconditional —
  `const whal_<Type> whal_<Plat>_<Type>_Dev = WHAL_CFG_<PLAT>_<TYPE>_DEV;`
- **multi**:
  ```c
  #ifdef WHAL_CFG_<PLAT>_<TYPE>_SINGLE_INSTANCE
  const whal_<Type> whal_<Plat>_<Type>_Dev = WHAL_CFG_<PLAT>_<TYPE>_DEV;
  #endif
  ```

`<ONE_INITIALIZER_PER_VTABLE_MEMBER>`: `    .<Member> = whal_<Plat>_<Type>_<Member>,`

### Stub function bodies

Every stub does **argument validation → TODO comment → `return WHAL_SUCCESS`**. The first
parameter is the device pointer; name it exactly as the generic vtable does (`spiDev`, `rngDev`,
`uartDev`, `i2cDev`, …). Use the device pointer name `<dev>` below. Match the `cfg` cast's
constness to the generic struct's `cfg` field.

**single** — read base/cfg from the fixed device instance, `(void)` the unused device pointer:

```c
whal_Error whal_<Plat>_<Type>_<Member>(<params>)
{
    size_t base = whal_<Plat>_<Type>_Dev.base;
    whal_<Plat>_<Type>_Cfg *cfg =
        (whal_<Plat>_<Type>_Cfg *)whal_<Plat>_<Type>_Dev.cfg;
    (void)<dev>;
    (void)base;
    (void)cfg;

    /* validate the remaining args, e.g.:
     * if (!<dataPtr> && <len>) return WHAL_EINVAL; */

    /* TODO: implement */
    return WHAL_SUCCESS;
}
```

**multi** — bifurcate on the single-instance flag; the SI branch reads the fixed device instance and `(void)`s
the pointer, the default branch validates the pointer and reads base/cfg from it:

```c
whal_Error whal_<Plat>_<Type>_<Member>(<params>)
{
#ifdef WHAL_CFG_<PLAT>_<TYPE>_SINGLE_INSTANCE
    size_t base = whal_<Plat>_<Type>_Dev.base;
    whal_<Plat>_<Type>_Cfg *cfg =
        (whal_<Plat>_<Type>_Cfg *)whal_<Plat>_<Type>_Dev.cfg;
    (void)<dev>;
#else
    size_t base;
    whal_<Plat>_<Type>_Cfg *cfg;

    if (!<dev> || !<dev>->cfg)
        return WHAL_EINVAL;

    base = <dev>->base;
    cfg = (whal_<Plat>_<Type>_Cfg *)<dev>->cfg;
#endif
    (void)base;
    (void)cfg;

    /* validate the remaining args here */

    /* TODO: implement */
    return WHAL_SUCCESS;
}
```

Notes for the bodies:
- For `Init`/`Deinit` and any member whose only parameter is the device pointer, drop the
  trailing `/* validate the remaining args */` line — there is nothing else to check.
- Keep `(void)base;` / `(void)cfg;` so the stub compiles with `-Werror` before the bodies are
  filled in. Remove them as soon as `base`/`cfg` get used.
- Add a `whal_Timeout`-based wait (`WHAL_TIMEOUT_*` or `whal_Reg_ReadPoll`) wherever the real
  implementation will poll hardware — never a bare `while`. Leave a TODO noting it.

---

# Mode B — board-level header-only driver

For board-level types (clock, power, and similar with no generic vtable). These are driven
imperatively from `Board_Init`: there is **no device struct, no vtable, no generic dispatch API,
no direct-API mapping, and no `.c` file**. Everything is `static inline` in one header, and the
peripheral's fixed base address lives at the top of that header. References:
`wolfHAL/clock/stm32wb_rcc.h`, `wolfHAL/power/stm32l1_pwr.h`.

The instance-model input ($3) does not apply here — ignore it.

**Do not invent register layouts, struct fields, or enum values.** Leave every register define,
descriptor struct, and enum as a TODO **comment** — do not emit concrete `typedef enum {...}` /
`typedef struct {...}` declarations with made-up members. So the header still compiles, keep the
emitted helper stubs parameterless (take `void`); the user adds their own structs/enums and wires
them into the helper signatures.

## B1 — Generate the header `wolfHAL/<type>/<plat>_<type>.h` (the only file)

Standard GPL file header (first line `<plat>_<type>.h`), then:

```c
#ifndef WHAL_<PLAT>_<TYPE>_H
#define WHAL_<PLAT>_<TYPE>_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

/*
 * @file <plat>_<type>.h
 * @brief <Plat> <Type> board-level driver.
 *
 * Boards drive the <type> peripheral imperatively from Board_Init by calling
 * the helpers below in order. <Type> is a board-level driver: there is no
 * whal_<Type> device struct, no generic API, no vtable, and no direct-API
 * mapping. The peripheral lives at a fixed address (WHAL_<PLAT>_<TYPE>_BASE)
 * and every helper is a `static inline` taking no device pointer.
 */

#define WHAL_<PLAT>_<TYPE>_BASE   0x00000000  /* TODO: peripheral base address from the datasheet */

/*
 * <Plat> <Type> Register Definitions
 *
 * TODO: add register offsets, bit positions, and masks from the TRM, e.g.:
 *   #define WHAL_<PLAT>_<TYPE>_CR_REG     0x00
 *   #define WHAL_<PLAT>_<TYPE>_CR_EN_Pos  0
 *   #define WHAL_<PLAT>_<TYPE>_CR_EN_Msk  (1UL << WHAL_<PLAT>_<TYPE>_CR_EN_Pos)
 */

/*
 * TODO: define the descriptor structs / enums the helpers take — e.g. a
 * peripheral-clock gate descriptor (clock) or a voltage-range enum (power).
 * Leave them as TODOs here; do not invent fields or values.
 */

<ONE_OR_MORE_STUBBED_INLINE_HELPERS>

#endif /* WHAL_<PLAT>_<TYPE>_H */
```

`<ONE_OR_MORE_STUBBED_INLINE_HELPERS>`: emit a representative set of imperative helper stubs the
user will rename, re-parameterize, and extend to match the peripheral's real operations. Each
takes **`void`** (so the header compiles before any structs/enums are defined), references
`WHAL_<PLAT>_<TYPE>_BASE`, has a TODO body, and returns `whal_Error`:

```c
/*
 * @brief <One line: what this helper does>.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_<Plat>_<Type>_Enable(void)
{
    /* TODO: implement — write WHAL_<PLAT>_<TYPE>_BASE registers via
     * whal_Reg_Update / whal_Reg_Get. Poll any ready bit before returning.
     * Clock bring-up runs before the system timer, so a bare do/while on the
     * ready bit is acceptable there; elsewhere prefer whal_Reg_ReadPoll with a
     * whal_Timeout. */
    return WHAL_SUCCESS;
}

/*
 * @brief <One line: what this helper does>.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_<Plat>_<Type>_Disable(void)
{
    /* TODO: implement */
    return WHAL_SUCCESS;
}
```

Name the stubs after the type's conventional operations (still parameterless — add the real
params, structs, and enums as TODOs the user fills in):
- **clock**: `EnableOsc`/`DisableOsc`, `EnablePll`/`DisablePll`, `SetSysClock`,
  `EnablePeriphClk`/`DisablePeriphClk`. Model on `wolfHAL/clock/stm32wb_rcc.h` — but keep the
  `Clock` token, not `Rcc`, and leave the descriptor struct / source enum as TODOs.
- **power**: a `SetVosRange`-style helper. Model on `wolfHAL/power/stm32l1_pwr.h` — keep the
  `Power` token, not `Pwr`, and leave the voltage-range enum as a TODO.
- **anything else**: keep the generic `Enable`/`Disable` pair above and tell the user to rename.

Drop `<wolfHAL/timeout.h>` unless a `whal_Timeout`-related TODO references it (keep the body
comments' mention of it regardless).

---

## After generating

Print a short summary tailored to the mode:

**Mode A:**
- The two file paths created.
- The vtable members stubbed (so the user sees the surface they must implement).
- What's intentionally left as TODO: register definitions, the `Cfg` fields, and each body.
- That wiring this driver into a board still needs the platform-header base-address `#define`,
  the `WHAL_CFG_<PLAT>_<TYPE>_DEV` initializer in `wolfHAL_board.h`, and the Makefile/CI entries — point
  to `port-stm32-platform`.

**Mode B:**
- The one header path created (note: no `.c`, nothing to add to the Makefile — it's header-only).
- The helper(s) stubbed, and that they should be renamed/extended to the peripheral's real ops.
- What's left as TODO: the `_BASE` address, register definitions, descriptor structs, and bodies.
- That boards include this header and call the helpers from `Board_Init`; the `_BASE` macro
  stays in this header (it is not re-defined in the platform header). See `port-stm32-platform`'s
  clock/power notes.

Do not invent register offsets, base addresses, or peripheral details — those come from the TRM
and datasheet and are the user's to fill in.
