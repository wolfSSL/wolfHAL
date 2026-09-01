# Adding a Peripheral

> **Scope.** This document describes the `boards/peripheral/` registry — a
> convenience mechanism used inside this repository so that tests and
> examples can dynamically opt a peripheral (SPI-NOR flash, SD card, IMU,
> etc.) into any compatible board via a build flag. It exists to make
> cross-board testing of peripheral drivers cheap.
>
> **In an actual application project**, you would not use this registry.
> You would simply define the peripheral instances you need directly in
> your `wolfHAL_board.c` (the same way you define `g_whalUart`, `g_whalSpi`, etc.)
> and call them directly from your application. The registry adds a layer
> of indirection that's only worth its cost when you're trying to
> mix-and-match peripherals across many boards from a single test binary.

This guide covers how to add an external peripheral device to the wolfHAL
peripheral registry. Peripherals are bus-attached devices (e.g., SPI-NOR
flash, SD cards) that live in `boards/peripheral/` and are opt-in at build
time.

## Overview

A peripheral consists of three parts:

1. A **device configuration file** that instantiates the device with
   board-specific parameters (SPI bus, CS pin, clock speed, etc.)
2. An **entry in the peripheral registry** (`wolfHAL_peripheral.c`) so that board init
   and tests can discover the device
3. A **board.mk entry** to conditionally compile the peripheral and its
   driver source

## File Layout

Peripherals are organized by device type under `boards/peripheral/`:

```
boards/peripheral/
  wolfHAL_peripheral.h          # Registry structs and extern arrays
  wolfHAL_peripheral.c          # Registry arrays (g_peripheralBlock[], g_peripheralFlash[], g_peripheralSensor[])
  board.mk          # Conditional build rules
  block/
    sdhc_spi_sdcard32gb.h
    sdhc_spi_sdcard32gb.c
  flash/
    spi_nor_w25q64.h
    spi_nor_w25q64.c
  sensor/imu/
    bmi270.h
    bmi270.c
```

## Step 1: Create the Device Configuration

Create a header and source file for your device under the appropriate type
directory.

### Header

Declare the global device instance:

```c
#ifndef BOARD_SPI_NOR_W25Q64_H
#define BOARD_SPI_NOR_W25Q64_H

#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/flash/spi_nor_flash.h>

extern whal_Flash g_whalSpiNorW25q64;

#endif
```

### Source

Define the device instance with its configuration. The device reaches its
underlying bus (SPI here) and any other host-side peripherals through the
board's `BOARD_<PERIPH>_DEV` macros so the same peripheral source works
regardless of how each board has wired those drivers. Other board values
(`SPI_CS_PIN`, `g_whalTimeout`) come from `wolfHAL_board.h` directly:

```c
#include "spi_nor_w25q64.h"
#include <wolfHAL/flash/spi_nor_flash.h>
#include "wolfHAL_board.h"

#define W25Q64_PAGE_SZ  256
#define W25Q64_CAPACITY (8 * 1024 * 1024)

static whal_Spi_ComCfg g_w25q64ComCfg = {
    .freq = 25000000,
    .mode = WHAL_SPI_MODE_0,
    .wordSz = 8,
    .dataLines = 1,
};

whal_Flash g_whalSpiNorW25q64 = {
    .driver = &whal_SpiNor_Driver,
    .cfg = &(whal_SpiNor_Cfg) {
        .spiDev = BOARD_SPI_DEV,
        .spiComCfg = &g_w25q64ComCfg,
        .gpioDev = BOARD_GPIO_DEV,
        .csPin = SPI_CS_PIN,
        .timeout = &g_whalTimeout,
        .pageSz = W25Q64_PAGE_SZ,
        .capacity = W25Q64_CAPACITY,
    },
};
```

## Step 2: Register in wolfHAL_peripheral.c

Add a conditional include and an entry in the appropriate registry array.

In `wolfHAL_peripheral.c`:

```c
#ifdef PERIPHERAL_SPI_NOR_W25Q64
#include "flash/spi_nor_w25q64.h"
#endif
```

And add an entry to the matching array (before the sentinel):

```c
whal_PeripheralFlash_Cfg g_peripheralFlash[] = {
#ifdef PERIPHERAL_SPI_NOR_W25Q64
    {
        .name = "spi_nor_w25q64",
        .dev = &g_whalSpiNorW25q64,
        .sectorSz = 4096,
    },
#endif
    {0}, /* sentinel */
};
```

The registry structs are defined in `wolfHAL_peripheral.h`:

- `whal_PeripheralBlock_Cfg` for block devices (`g_peripheralBlock[]`)
- `whal_PeripheralFlash_Cfg` for flash devices (`g_peripheralFlash[]`)
- `whal_PeripheralSensor_Cfg` for sensor devices (`g_peripheralSensor[]`)

Each array is terminated by a zero sentinel so that board init and test code
can iterate without knowing the count.

## Step 3: Add Build Rules

In `boards/peripheral/board.mk`, add a conditional block for your
peripheral. The block checks whether the peripheral name appears in the
`PERIPHERALS` variable and adds the define, config source, and driver source:

```makefile
ifneq ($(filter mydevice,$(PERIPHERALS)),)
CFLAGS += -DPERIPHERAL_MYDEVICE
BOARD_SOURCE += $(_PERIPHERAL_DIR)/type/mydevice.c
BOARD_SOURCE += $(WHAL_DIR)/src/type/mydevice_driver.c
endif
```

This compiles both the peripheral configuration and the underlying driver
source when the peripheral is enabled.

## Building

Enable peripherals using the `PERIPHERALS` variable:

```
make BOARD=stm32wb55xx_nucleo PERIPHERALS="spi_nor_w25q64"
```

Multiple peripherals can be enabled simultaneously:

```
make BOARD=stm32wb55xx_nucleo PERIPHERALS="spi_nor_w25q64 bmi270"
```

## Testing

Peripheral devices are automatically picked up by their matching test suite.
Flash peripherals are tested by the `flash` test, and block peripherals by the
`block` test. See [Adding a Test](adding_a_test.md) for details on the test
framework.

## Naming Convention

Instance files are named `<driver>_<instance>`:

- `sdhc_spi_sdcard32gb` — sdhc_spi driver, 32GB SD card
- `spi_nor_w25q64` — spi_nor driver, W25Q64 chip
- `bmi270` — bmi270 driver (single instance, no qualifier needed)

Summary:

- Directory: `boards/peripheral/<type>/` (e.g., `flash/`, `block/`, `sensor/imu/`)
- Files: `<driver>.h` and `<driver>.c` for single-instance peripherals; otherwise `<driver>_<instance>.h` and `<driver>_<instance>.c`
- Flag: `PERIPHERAL_<NAME>` (e.g., `PERIPHERAL_SPI_NOR_W25Q64`)
- PERIPHERALS variable: lowercase name (e.g., `spi_nor_w25q64`, `bmi270`)
- Global instance: `g_whal<Name>` (e.g., `g_whalSpiNorW25q64`, `g_whalBmi270`)
