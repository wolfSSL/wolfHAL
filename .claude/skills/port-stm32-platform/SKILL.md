---
name: port-stm32-platform
description: Port a new STM32 chip family to wolfHAL. Creates the platform header, scaffolds new drivers, adds alias header + stub .c for reused drivers, and optionally wires up a first board.
argument-hint: <platform-short-name> [chip-name] [board-name]
---

# Port a new STM32 chip family to wolfHAL

You are helping the user port a new STM32 chip family (e.g., `stm32g4`, `stm32u5`, `stm32l5`) to wolfHAL. A "platform" in wolfHAL is a family of register-compatible chips that share drivers (e.g., `stm32wba` covers wba52/wba54/wba55). A specific chip (e.g., `stm32wba55cg`) gets its own platform header with concrete base addresses and clock macros. A board (e.g., `stm32wba55cg_nucleo`) ties a chip to a physical dev board.

Arguments passed after the skill name identify the scope:
- `$1`: platform short name (required, e.g., `stm32g4`)
- `$2`: specific chip (optional, e.g., `stm32g431cb`)
- `$3`: board name (optional, e.g., `stm32g431_nucleo`)

If any are missing, ask the user before starting the relevant phase.

## Prerequisites — gather before starting

Ask the user to provide (or confirm paths to):

1. **Reference Manual (TRM) PDF** for the chip family — e.g., `stm32g4_trm.pdf`. Place at repo root.
2. **Datasheet PDF** for the specific chip — e.g., `stm32g431cb.pdf`. Place at repo root.
3. **Board user guide PDF** if adding a board — e.g., `nucleo_g431.pdf`. Place at repo root.

Do NOT proceed with register layout work without the TRM — guessing from similar chips is the #1 source of silent bugs. STM32 family differences (bit positions, register offsets, new CONDRST/FIFO/voltage-scaling fields) are subtle and costly to debug later.

## Phase 1 — Discovery (read-only, no code changes)

Goal: figure out what is register-compatible with an existing wolfHAL driver vs what needs a new driver.

For each device type below, use `pdftotext -layout -f <start> -l <end> <trm.pdf>` on the TRM to extract the register map page, then compare against existing wolfHAL drivers in `src/<type>/`:

| Device type | Existing drivers to diff against | What to check in TRM |
|---|---|---|
| Clock/RCC   | `stm32wb_rcc.c`, `stm32wba_rcc.c`, `stm32h5_rcc.c`, `stm32f4_rcc.c` | PLL config register (PLLCFGR split vs unified), voltage scaling (PWR_VOSR), AHB/APB prescaler fields, HSI/HSE/LSI/LSE, peripheral clock enable register offsets (AHBxENR, APBxENR) |
| GPIO        | `stm32wb_gpio.c`, `stm32f4_gpio.c` | MODER/OTYPER/OSPEEDR/PUPDR/AFRL/AFRH — usually compatible across STM32 |
| UART        | `stm32wb_uart.c`, `stm32wba_uart.c`, `stm32wba_uart_dma.c`, `stm32h5_uart.c`, `stm32c0_uart.c`, `stm32f4_uart.c` | BRR calc (16x vs 8x oversample), FIFO enable (CR1.FIFOEN bit 29), ISR/ICR flags, TDR/RDR offsets |
| Flash       | `stm32wb_flash.c`, `stm32wba_flash.c`, `stm32h5_flash.c`, `stm32c0_flash.c`, `stm32f4_flash.c` | NSKEYR/KEYR unlock sequence, SR bit positions (BSY, EOP, errors), CR bit positions (PG, PER, STRT, LOCK, PNB), page size |
| RNG         | `stm32wb_rng.c`, `stm32wba_rng.c` | CR bits (RNGEN, CED, CONDRST), SR bits (DRDY, errors), clock source selection register (CCIPRx RNGSEL) |
| I2C         | `stm32wb_i2c.c` | CR1/CR2/TIMINGR — usually compatible on modern STM32 (V2 I2C) |
| SPI         | `stm32wb_spi.c`, `stm32h5_spi.c`, `stm32f4_spi.c` | CR1/CR2/SR layout (V1 SPI differs from V2 SPI used on newer chips) |
| AES         | `stm32wb_aes.c`, `stm32wba_aes.c` | CR (KEYSIZE, MODE, CHMOD, DATATYPE, NPBLB, GCMPH), KEYR, IVR, DIN/DOUT |
| HASH        | `stm32wba_hash.c` | CR (ALGO, MODE, DATATYPE, LKEY, INIT), STR (NBLW, DCAL), SR (BUSY), DIN, digest registers (HRx). Check ALGO bit positions — some chips split ALGO across non-contiguous bits |
| DMA         | `stm32wb_dma.c` (classic DMA+DMAMUX), `stm32wba_gpdma.c` (GPDMA) | Is it DMA+DMAMUX (older) or GPDMA (newer)? Not compatible. GPDMA appears on U5/WBA/H5/C0/G0 variants. |
| Timer       | `systick.c` (shared) | SysTick is architectural (Cortex-M), works unchanged |
| IRQ         | `cortex_m4_nvic.c`, `cortex_m33_nvic.c` | NVIC is architectural per core (M4 vs M33) |
| Watchdog    | `stm32wb_iwdg.c`, `stm32wb_wwdg.c`, `stm32wba_iwdg.c`, `stm32wba_wwdg.c` | IWDG/WWDG are very stable across STM32 |

For each device type, decide **reuse existing driver (with alias)** OR **write a new driver**. Write the decision to a checklist and show the user before starting Phase 2.

The clock driver is almost always new per family. GPIO, I2C, and IWDG/WWDG are almost always reusable. UART, flash, RNG, DMA require careful diffing.

**Clock is an architectural exception (1): header-only inline.** Clock drivers are implemented entirely as `static inline` functions in `wolfHAL/clock/<platform>_rcc.h` — there is no `.c` file under `src/clock/`. Register defines live in the header alongside the function bodies. Reference: `wolfHAL/clock/stm32wb_rcc.h`.

**Clock is an architectural exception (2): no vtable, no device handle.** Unlike most driver types, the clock subsystem has no `whal_ClockDriver` vtable, no generic `whal_Clock_Init`/`Enable`/`Disable` API, and no `whal_Clock` handle at all. The chip's clock-controller `_BASE` macro lives at the top of the clock driver header (the platform header just includes the clock header and doesn't re-define `_BASE`). Chip clock drivers expose imperative `Enable*`/`Disable*`/`Set*` helpers (e.g. `whal_<Platform>_Rcc_EnableOsc`, `EnablePll`, `SetSysClock`, `EnablePeriphClk`) that take no device pointer; boards call them directly. The platform header does NOT define a `WHAL_<PLATFORM>_RCC_DRIVER` macro. Power follows the same shape — no handle, helpers take no device pointer, `_BASE` macro at the top of the power driver header.

**Most other drivers are single-instance**, reading their cfg/base from a `whal_<Platform>_<Driver>_Dev` singleton that the driver `.c` defines from a `WHAL_CFG_<PLATFORM>_<DRIVER>_DEV` initializer macro in `wolfHAL_board.h`. The driver header `extern`-declares the singleton. Two flavors:
- **Unconditional single-instance** for true singletons (one per chip): RNG, GPIO, NVIC, SysTick, Watchdog, Crypto, Hash, Ethernet, Flash. Driver body always reads from the singleton; NULL checks dropped. Examples: `src/rng/stm32wb_rng.c`, `src/eth/stm32n6_eth.c`.
- **Conditional single-instance** gated on `WHAL_CFG_<PLATFORM>_<TYPE>_SINGLE_INSTANCE` for multi-instance peripherals (UART, SPI, I2C, DMA). Default builds keep the pointer-based path; boards opt in per device. Example: `src/uart/stm32wb_uart.c`.

**Flash with peripheral coexistence (e.g. on-chip + SPI NOR W25Q64)**: the driver-owned singleton (defined in the driver `.c` from `WHAL_CFG_<PLATFORM>_FLASH_DEV` in `wolfHAL_board.h`) carries `.driver`, `.base`, and `.cfg`. `BOARD_FLASH_DEV` casts its address (`((whal_Flash *)&whal_<Plat>_Flash_Dev)`) so generic `whal_Flash_*` calls can vtable-dispatch through it. There is no separate `g_whalFlash` stub. Reference: `boards/stm32wb55xx_nucleo/wolfHAL_board.h` + `src/flash/stm32wb_flash.c`.

**Crypto is also an architectural exception.** Crypto uses per-algorithm device structs (`whal_AesGcm`, `whal_Sha256`, etc.) instead of a single `whal_Crypto` with generic dispatch. Each algo struct has `.crypto` (pointer to the hardware device), `.driver` (per-algo vtable with Oneshot/Start/Process/Finalize), and `.state` (driver-managed streaming state). `whal_Crypto` itself is a platform driver with just Init/Deinit for hardware lifecycle. Direct API mapping is per-algorithm (e.g. `WHAL_CFG_STM32WB_AES_GCM_DIRECT_API_MAPPING`), not per-device-type. See `docs/writing_a_driver.md` "Crypto" section. Reference: `wolfHAL/crypto/stm32wb_aes.h` and `wolfHAL/crypto/stm32wba_hash.h`.

## Phase 2 — Platform header

Create `wolfHAL/platform/st/<chip>.h` (e.g., `stm32g431cb.h`). Reference: `stm32wba55cg.h` (GPDMA chip) or `stm32wb55xx.h` (DMA+DMAMUX chip).

Conventions:
1. Every driver include uses the **new platform's prefix** — for reused drivers that means the alias header created in Phase 3, not the original family's header. The clock driver header may be included here (it carries its own `_BASE`); the power driver header is pulled in directly where needed since not every board uses it.
2. Every base-address `#define` (e.g., `WHAL_<PLATFORM>_USART1_BASE`, `WHAL_<PLATFORM>_AES_BASE`) lands here. Board.h's `WHAL_CFG_*_DEV` initializers (Phase 4) pull these. Exception: the RCC/PWR `_BASE` macros stay in the clock/power driver header itself.
3. Vtable-pointer `#define`s like `WHAL_<PLATFORM>_<TYPE>_DRIVER` are needed for any driver type whose `_DRIVER` symbol callers may want to reference — typically flash (multiple flash drivers can coexist via SPI NOR peripheral) and the crypto peripheral-level `_CryptoDriver` (e.g. `WHAL_<PLATFORM>_AES_DRIVER &whal_<Plat>_Aes_CryptoDriver`, `WHAL_<PLATFORM>_HASH_DRIVER`, `WHAL_<PLATFORM>_CRYP_DRIVER`). Per-mode crypto vtables (`_AesEcbDriver`, `_Sha1Driver`, etc.) don't get `_DRIVER` macros — there's no single name to pin them to.
4. Defines clock enable macros and, when applicable, DMA request mapping macros.

Skeleton:

```c
#include <wolfHAL/platform/arm/cortex_m4.h>  /* or cortex_m33.h / cortex_m7.h */

/* Every include uses the new platform's prefix — aliased or newly written. */
#include <wolfHAL/clock/<platform>_rcc.h>
#include <wolfHAL/gpio/<platform>_gpio.h>
#include <wolfHAL/uart/<platform>_uart.h>
/* ...one include per device type... */

/* Base addresses — referenced by wolfHAL_board.h's WHAL_CFG_*_DEV initializers. */
#define WHAL_<PLATFORM>_USART1_BASE  0x<addr>
#define WHAL_<PLATFORM>_AES_BASE     0x<addr>
#define WHAL_<PLATFORM>_FLASH_BASE   0x<addr>

/* Vtable-pointer macros — flash (vtable-dispatched due to SPI NOR
 * coexistence) and the crypto peripheral-level _CryptoDriver. */
#define WHAL_<PLATFORM>_FLASH_DRIVER &whal_<Platform>_Flash_Driver
#define WHAL_<PLATFORM>_AES_DRIVER   &whal_<Platform>_Aes_CryptoDriver
#define WHAL_<PLATFORM>_HASH_DRIVER  &whal_<Platform>_Hash_CryptoDriver

#define WHAL_<PLATFORM>_USART1_CLOCK \
    .regOffset = 0x<offset>,         \
    .enableMask = (1UL << <bit>),    \
    .enablePos  = <bit>
```

## Phase 3 — Drivers

### For each device type marked "write new"

1. Read `docs/writing_a_driver.md` for the driver pattern.
2. Copy the closest existing driver as a starting point.
3. Update register offsets, bit positions, and sequences per the TRM. **Cross-check register-map diagrams against the textual bit descriptions** — they sometimes disagree, and the textual description is authoritative.
4. Match the existing naming: `whal_<Platform><Type>_<Func>` for functions, `whal_<Platform><Type>_Driver` for the vtable, `whal_<Platform><Type>_Cfg` for the config struct. **Exception: the clock driver has no vtable and no `_Driver` symbol** — its public API is the imperative `Enable*`/`Disable*`/`Set*` helpers. See `docs/writing_a_driver.md` "Clock".
5. Place files at `wolfHAL/<type>/<platform>_<type>.h` and `src/<type>/<platform>_<type>.c`.
6. Do not add cross-driver calls from inside a driver — clock enables, power sequencing, flash wait states, pin muxing are the board's responsibility. Boards toggle peripheral clocks via the chip clock driver's `whal_<Platform>_Rcc_EnablePeriphClk(&gateDescriptor)` (or chip-equivalent name); it takes no device pointer.

### For each device type marked "reuse existing driver" — create an alias header + stub .c

Reference pattern in-tree: `wolfHAL/gpio/stm32f4_gpio.h` (alias header) and `src/gpio/stm32f4_gpio.c` (stub). Every symbol the caller sees must carry the new platform's prefix; aliasing is done at preprocess time via `typedef` and `#define`, so no code is duplicated and no GCC alias attribute is needed.

**Alias header** — `wolfHAL/<type>/<newplatform>_<type>.h`:

```c
#ifndef WHAL_<NEWPLATFORM>_<TYPE>_H
#define WHAL_<NEWPLATFORM>_<TYPE>_H

/*
 * @file <newplatform>_<type>.h
 * @brief <NewPlatform> <Type> driver (alias for <OrigPlatform> <Type>).
 *
 * The <NewPlatform> <Type> peripheral is register-compatible with the
 * <OrigPlatform> <Type>. This header re-exports the <OrigPlatform> driver
 * types and symbols under <NewPlatform>-specific names. The underlying
 * implementation is shared.
 */

#include <wolfHAL/<type>/<origplatform>_<type>.h>

/* Type aliases — one typedef per exposed type from the original header */
typedef whal_<OrigPlatform><Type>_Cfg    whal_<NewPlatform><Type>_Cfg;
typedef whal_<OrigPlatform><Type>_PinCfg whal_<NewPlatform><Type>_PinCfg;
/* ...repeat for every exposed type... */

/* Singleton alias — REQUIRED for any single-instance driver. wolfHAL_board.h declares
 * the singleton using the new platform's name; this macro renames it at
 * preprocess time so the leaf driver's references resolve correctly. Place
 * unconditionally (not under any DIRECT_API_MAPPING guard). */
#define whal_<NewPlatform><Type>_Dev whal_<OrigPlatform><Type>_Dev

/* Driver and function aliases — one #define per exposed function/driver */
#define whal_<NewPlatform><Type>_Driver whal_<OrigPlatform><Type>_Driver
#define whal_<NewPlatform><Type>_Init   whal_<OrigPlatform><Type>_Init
#define whal_<NewPlatform><Type>_Deinit whal_<OrigPlatform><Type>_Deinit
/* ...repeat for every exposed function... */

/* Macro / enum-value aliases — one #define per user-facing macro */
#define WHAL_<NEWPLATFORM>_<TYPE>_<CONST1> WHAL_<ORIGPLATFORM>_<TYPE>_<CONST1>
/* ...repeat for every mode selector, port letter, pin-packing macro, etc... */

#endif /* WHAL_<NEWPLATFORM>_<TYPE>_H */
```

**Stub .c** — `src/<type>/<newplatform>_<type>.c`:

```c
#include "<origplatform>_<type>.c"
```

That is the entire file. Examples in-tree: `src/gpio/stm32f4_gpio.c`, `src/gpio/stm32wba_gpio.c`, `src/i2c/stm32wba_i2c.c`, `src/uart/stm32wba_uart.c`, `src/watchdog/stm32wba_iwdg.c` — each is one line. The stub exists so the board's Makefile wildcard (`src/*/<newplatform>_*.c`) compiles the original implementation under the new-prefix filename. The original `.c` is NOT added to the Makefile separately; the `#include` pulls it into this translation unit exactly once.

**Direct API mapping macros stay in the leaf driver, not in the alias .c.** Most leaf drivers contain a guarded block that renames the platform-specific function names directly to the generic `whal_<Type>_*` API. The flag is `WHAL_CFG_<PLATFORM>_<TYPE>_DIRECT_API_MAPPING` (note the order: platform-then-type):

```c
/* in src/<type>/<origplatform>_<type>.c — the LEAF */
#if defined(WHAL_CFG_<ORIGPLATFORM>_<TYPE>_DIRECT_API_MAPPING) || \
    defined(WHAL_CFG_<NEWPLATFORM>_<TYPE>_DIRECT_API_MAPPING)
#define whal_<OrigPlatform><Type>_Init   whal_<Type>_Init
/* ...one #define per driver entry point... */
#endif

/* ...driver implementation... */

#if !defined(WHAL_CFG_<ORIGPLATFORM>_<TYPE>_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_<NEWPLATFORM>_<TYPE>_DIRECT_API_MAPPING)
const whal_<Type>Driver whal_<OrigPlatform><Type>_Driver = { ... };
#endif
```

Add the new platform's `WHAL_CFG_<NEWPLATFORM>_<TYPE>_DIRECT_API_MAPPING` macro to **both** guards in the leaf driver. Do **not** translate the macro inside the alias `.c` stub like `#ifdef <NEW> #define <ORIG> #endif` — every leaf has to learn about every alias platform anyway, and putting the recognition in two places (alias shim + leaf driver) creates drift. Keep the alias `.c` as a single `#include` line.

**For conditional single-instance drivers (UART, SPI, I2C, DMA)**, the leaf driver also has a separate `WHAL_CFG_<PLATFORM>_<TYPE>_SINGLE_INSTANCE` gate that bifurcates each function body — SI branch reads from `whal_<Platform>_<Type>_Dev`, else branch keeps the pointer-based code. The new platform's `_SINGLE_INSTANCE` macro must be added to that disjunction in the leaf too. See `src/uart/stm32wb_uart.c` for the canonical form.

**Alias the leaf directly — never daisy-chain.** If `<origplatform>` itself aliases another driver, alias `<newplatform>` to the **leaf** (the one with the actual implementation), not to the intermediate alias. Two-hop chains (`<new>` → `<intermediate>` → `<leaf>`) make every macro recognition twice as painful and obscure the dependency graph. Trace the include chain in the existing `.h` and `.c` files until you find the file with real driver code, and point your new alias at that.

**Test alias** — when a driver is reused via alias AND the original driver already has a platform-specific test file (`tests/<type>/test_<origplatform>_<type>.c`), create a matching test alias at `tests/<type>/test_<newplatform>_<type>.c`:

```c
#include "test_<origplatform>_<type>.c"
```

This is the same one-line pattern as the driver stub. The build system auto-discovers `test_$(PLATFORM)_$(t).c` files and defines `WHAL_TEST_ENABLE_<TYPE>_PLATFORM`, which gates the `whal_Test_<Type>_Platform()` call in `tests/main.c`. Examples in-tree: `tests/gpio/test_stm32c0_gpio.c`, `tests/gpio/test_stm32f0_gpio.c`.

### Common driver pitfalls (from prior ports)
- **Flash**: check if already unlocked before writing keys — double-unlock hard-faults. Bit positions (LOCK, STRT, PNB) differ between families; do not copy-paste.
- **RNG**: CONDRST + per-config register sequence goes in Init, not per Generate call. Select RNG clock source via `RCC_CCIPR`/`CCIPR2` before Init — default is often LSE, which requires LSE running.
- **Clock**: on chips with voltage scaling, transition to VOS Range 1 BEFORE raising SYSCLK above ~16 MHz; skipping this hard-faults at the first PLL switch.
- **DMA + USART**: enable `CR1.FIFOEN` when available — DMA→USART without FIFO has produced silent byte drops.
- **Polling loops**: every hardware-flag wait must go through `whal_Timeout` — bare `while` loops violate a repo-wide preference.
- **One-time hardware setup** (clock sources, trimming, feature selects): belongs in Init, not in per-operation functions.

## Phase 4 — Board (optional, only if $3 provided)

Create `boards/<board_name>/` with:

### `wolfHAL_board.h`

Three concerns: extern declarations for pointer-based globals that still live in `wolfHAL_board.c`, `WHAL_CFG_<PLAT>_<X>_DEV` initializer macros for each single-instance driver (the driver header `extern`-declares the singleton; the driver `.c` defines it from this initializer after `#include "wolfHAL_board.h"`), and the `BOARD_<PERIPH>_DEV` macro block that lets tests/apps portably reach each peripheral. Follow `docs/adding_a_board.md`.

Skeleton:

```c
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/platform/st/<chip>.h>

/* Pointer-based globals — defined in wolfHAL_board.c, declared here for reachability. */
extern whal_Uart g_whalUart;

extern whal_Timeout g_whalTimeout;
extern volatile uint32_t g_tick;

/* BOARD_*_DEV macros — how this board reaches each peripheral.
 * WHAL_INTERNAL_DEV for single-instance drivers (driver ignores the pointer);
 * (&g_whal<X>) for pointer-based; or a cast pointer at a driver-owned singleton
 * when single-instance must coexist with another driver of the same generic
 * type (typically flash + SPI NOR). */
#define BOARD_GPIO_DEV       WHAL_INTERNAL_DEV
#define BOARD_UART_DEV       (&g_whalUart)
#define BOARD_FLASH_DEV      ((whal_Flash *)&whal_<NewPlatform>_Flash_Dev)
#define BOARD_RNG_DEV        WHAL_INTERNAL_DEV
#define BOARD_WATCHDOG_DEV   WHAL_INTERNAL_DEV
/* ...one per peripheral the board exposes... */

/* Initializers for single-instance singletons. The driver header
 * extern-declares whal_<NewPlatform>_<X>_Dev; the driver .c writes
 *   const whal_<X> whal_<NewPlatform>_<X>_Dev = WHAL_CFG_<NEWPLATFORM>_<X>_DEV;
 * after #include "wolfHAL_board.h". The board uses its own platform's prefix; if
 * the driver is an alias of another platform's, the alias header bridges
 * the singleton name with a #define. */
#define WHAL_CFG_<NEWPLATFORM>_GPIO_DEV { \
    .base = WHAL_<PLATFORM>_GPIO_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_<NewPlatform>_Gpio_Cfg){ \
        .pinCfg = (const whal_<NewPlatform>_Gpio_PinCfg[PIN_COUNT]){ \
            [LED_PIN] = WHAL_<PLATFORM>_GPIO_PIN(...), \
            /* ...one entry per board pin... */ \
        }, \
        .pinCount = PIN_COUNT, \
    }, \
}

#define WHAL_CFG_<NEWPLATFORM>_RNG_DEV { \
    .base = WHAL_<PLATFORM>_RNG_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_<NewPlatform>_Rng_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

/* Watchdog: define initializers for both IWDG and WWDG unconditionally —
 * each alias .c that includes the leaf always references its singleton. */
#define WHAL_CFG_<NEWPLATFORM>_IWDG_DEV { ... }
#define WHAL_CFG_<NEWPLATFORM>_WWDG_DEV { ... }

/* SysTick is platform-agnostic — initializer drops the platform segment. */
#define WHAL_CFG_SYSTICK_DEV { \
    .base = WHAL_CORTEX_M4_SYSTICK_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_SysTick_Cfg){ \
        .cyclesPerTick = <hz> / 1000, \
        .clkSrc = WHAL_SYSTICK_CLKSRC_SYSCLK, \
        .tickInt = WHAL_SYSTICK_TICKINT_ENABLED, \
    }, \
}
```

**Names follow the board's own platform.** `WHAL_CFG_STM32G4_IWDG_DEV` in a g4-board's wolfHAL_board.h, with `whal_Stm32g4_Iwdg_Dev` as the singleton name. If the IWDG driver is an alias of wb's, the alias header bridges the singleton name (`#define whal_Stm32g4_Iwdg_Dev whal_Stm32wb_Iwdg_Dev`); the board supplies the initializer under the upstream platform's `WHAL_CFG_STM32WB_IWDG_DEV` name (because the leaf driver `.c` references that). Allowed cross-platform names: `whal_Nvic_Dev`, `whal_SysTick_Dev`, `whal_Lan8742a_Dev` (these are genuinely platform-agnostic).

**State for AEAD streaming** (AES GCM/CCM, HMAC) is a `static` variable in the driver `.c`. The `WHAL_CFG_<PLAT>_<ALGO>_DEV` initializer in wolfHAL_board.h takes its address via the `.state` field.

### `wolfHAL_board.c`

Define remaining pointer-based globals (`g_whalUart` for the still-vtable-dispatched peripherals, `g_whalTimeout`, the SysTick handler) and implement `Board_Init`/`Board_Deinit`/`Board_WaitMs`. Single-instance singletons (flash included) live in their driver `.c` files, not here — `wolfHAL_board.c` does not declare or initialize them.

Init/Deinit call sites use `BOARD_<PERIPH>_DEV`, NOT the raw globals — this lets the board switch a peripheral between single-instance and pointer-based by flipping one macro:

```c
err = whal_Gpio_Init(BOARD_GPIO_DEV);
err = whal_Uart_Init(BOARD_UART_DEV);
err = whal_Flash_Init(BOARD_FLASH_DEV);
err = whal_Timer_Init(BOARD_TIMER_DEV);
```

Clock-tree bring-up is imperative — `whal_<Platform>_Rcc_EnableOsc(...)`, `EnablePll(...)`, `SetSysClock(...)`, `EnablePeriphClk(...)` take no device pointer (each reads the chip's fixed RCC base from the driver header).

Implement `Board_Init` in dependency order: PWR → bring up clock tree imperatively (oscillator on, PLL configure+enable, sysclk switch) → enable peripheral clocks → GPIO → UART → Timer → the rest. Keep the watchdog out of `Board_Init` (the app starts it when ready to refresh) per `docs/adding_a_board.md`. Guard DMA-specific setup under `#ifdef BOARD_DMA`, matching `boards/stm32wba55cg_nucleo/wolfHAL_board.c`.

### GPIO pin conflict check
After writing the `pinCfg` array in `wolfHAL_board.c`, scan every entry pair and verify no two entries share the same physical port+pin. This is a common mistake when a pin serves double duty (e.g., PA5 used as both an LED and SPI1_SCK). If a conflict is found, consult the chip's alternate-function table in the datasheet and remap the conflicting peripheral to an alternate pin on a different port.

### `board.mk`
Model on `boards/stm32wba55cg_nucleo/board.mk`:
- `PLATFORM = <platform>` — matches the prefix used in `src/*/<platform>_*.c`
- `TESTS ?= clock gpio flash timer rng uart spi i2c` plus any per-algorithm crypto tests (`aes_ecb aes_cbc aes_gcm sha256 ...`) — trim to what the board supports. `clock` only has `_PLATFORM` variants (no generic clock API), and `crypto` is the same — `WHAL_TEST_ENABLE_CLOCK` / `_CRYPTO` only gate the `_PLATFORM` suites. There are no `dma` or `irq` test suites.
- `CFLAGS` — `-mcpu=cortex-m4`/`cortex-m33` to match the core, `-DPLATFORM_<UPPER>` for platform ifdefs
- `BOARD_SOURCE` wildcards over `$(WHAL_DIR)/src/*/<platform>_*.c` — picks up both native drivers and the alias stub `.c` files created in Phase 3. **Do NOT also wildcard over the original family's prefix**, or the original `.c` will compile twice (once directly, once through the stub's include) and you'll get duplicate-symbol errors at link time.

### `linker.ld`
Copy from a similar board and update the `MEMORY` block's FLASH/RAM origins and lengths from the chip's datasheet.

### `ivt.c`
Copy from a similar board with the same core (M4/M33). Update the vector table — vectors from position 16 onward are device-specific and listed in the TRM's interrupt mapping table. At minimum: SysTick plus any peripheral IRQs the tests exercise (USART1_IRQHandler, GPDMAx_ChannelY_IRQHandler, etc.).

### boards/README.md

Add a row to the **Supported Boards** table in `boards/README.md` with the board name, platform, CPU core, and directory. Keep the table sorted alphabetically by platform name.

### GitHub CI

Add the new board to `.github/workflows/boards.yml` by appending it to the `board` matrix list. This ensures the board builds are verified on every PR and push to main. If the board supports peripheral devices (BMI270, SPI-NOR, etc.), also add entries to `.github/workflows/peripheral-tests.yml`. If it supports watchdog, add entries to `.github/workflows/watchdog-tests.yml`.

## Phase 5 — Build and validate

1. `make BOARD=<board_name>` from the repo root. Fix errors in order. Typical failures:
   - Missing symbol for an aliased driver → the alias header is missing a `#define` for that function/type, or the stub `.c` isn't present.
   - `'whal_Stm32<orig>_<Type>_Dev' undeclared` in a single-instance leaf driver → the alias header for the new platform is missing the `#define whal_Stm32<new>_<Type>_Dev whal_Stm32<orig>_<Type>_Dev` line, OR the wolfHAL_board.h didn't provide `WHAL_CFG_STM32<ORIG>_<TYPE>_DEV` (the leaf `.c` defines the singleton from that name).
   - Duplicate symbol → the Makefile is picking up both `<newplatform>_*.c` and the original `<origplatform>_*.c`; restrict the wildcard to the new prefix only.
   - Implicit declaration warnings for `whal_Reg_*` / `whal_SetBits` in a stub include → you probably wrote `#include <wolfHAL/...>` with angle brackets in the stub instead of `#include "<origplatform>_<type>.c"` with quotes. The stub must use quoted include so the preprocessor finds the sibling `.c` in the same directory.
2. Flash the binary (user runs this) and run the test suite. Each suite prints `PASS`, `FAIL`, or `SKIP`.
3. If UART output is garbled or missing bytes and the chip uses DMA, enable `CR1.FIFOEN` in the UART init before chasing further — DMA→USART without FIFO has caused byte drops on WBA.

## Output to the user

At each phase transition, summarize in under 10 lines:
- What was done
- Which existing drivers were reused (and note the alias header + stub `.c` pair created for each) vs newly written
- What the user should verify or flash

Wait for the user to confirm before moving to the next phase, unless they asked for an all-in-one run upfront.
