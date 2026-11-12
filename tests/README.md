# wolfHAL Test Suite

## Hardware Tests

### Building

```
make BOARD=<board>
```

Each board defines its supported tests in `boards/<board>/board.mk`. The output binary
is placed in `build/<board>/`.

### Selecting Tests

By default, all supported tests for the specified board will be built. Override
the `TESTS` variable to build a subset:

```
make BOARD=stm32wb55xx_nucleo TESTS=gpio
make BOARD=stm32wb55xx_nucleo TESTS="gpio uart"
```

### Test Structure

Tests are organized by device type:

```
tests/
  main.c          # Test runner entry point
  test.h          # Test macros (WHAL_ASSERT_EQ, WHAL_TEST, etc.)
  Makefile
  gpio/
    test_gpio.c          # Generic API tests
    test_<platform>_gpio.c  # Platform-specific tests
  clock/
    test_<platform>_clock.c # Clock has no generic API — only platform tests
  ...
```

Each device directory contains:

- `test_<device>.c` — Generic tests that exercise the wolfHAL API.
- `test_<platform>_<device>.c` — Platform-specific tests. These are
  automatically detected and compiled when building for the matching board.

Clock is a board-level driver with no generic API, so the `clock/` directory
contains only platform-specific tests.

Board support (device instances, linker scripts, etc.) lives in the top-level
`boards/` directory. See [boards/README.md](../boards/README.md) for details.

### Peripheral Devices

External peripheral drivers (SPI-NOR flash, SD cards, IMUs, etc.) are opt-in.
Enable them using the `PERIPHERALS` variable when building:

```
make BOARD=stm32wb55xx_nucleo PERIPHERALS="spi_nor_w25q64"
make BOARD=stm32wb55xx_nucleo PERIPHERALS="bmi270" TESTS="bmi270"
make BOARD=stm32wb55xx_nucleo PERIPHERALS="spi_nor_w25q64 bmi270"
```

Peripheral devices are automatically tested by their matching test suite (e.g.,
`flash` tests iterate all entries in `g_peripheralFlash[]`, `bmi270` tests
use `g_peripheralSensor[]`).

## Core Tests

Host-side unit tests (bitops, dispatch, endian, reg, timeout) live in `core/`
and build with the native compiler. Driver tests against an emulated
register block or a fake flash live with their type
(`spimem/test_cadence_ospi_spimem.c`, `spimem/test_versal2_ospi_dma.c`,
`flash/test_spimem_nor_flash.c`) and are built into the same `core/`
binary:

```
cd core && make && make run
```
