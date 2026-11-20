# Changelog

All notable changes to wolfHAL are documented in this file.

## [1.0.0] - 2026-11-18

First tagged release of wolfHAL.

### Platforms

- **ST STM32:** C0, F0, F3, F4, H5, L1, N6, U5, WB, WB0, WBA
- **Microchip:** PIC32CZ
- **AMD:** Versal Gen 2
- **Arm Cortex-M:** M0, M0+, M3, M4, M7, M33, M55

### Device APIs

- Block, Clock, Crypto, Display, DMA, Ethernet, Ethernet PHY, Flash, GPIO,
  I2C, IPC, IRQ, Power, PWM, RNG, Sensor, SPI, SPI memory, Timer, UART,
  Watchdog
- Generic vtable dispatch per device type, with optional direct API mapping
  for single-instance drivers

### Drivers

- **Clock:** STM32 RCC for all supported STM32 families, PIC32CZ clock
- **Power:** STM32L1 PWR, PIC32CZ SUPC
- **GPIO:** STM32 (all supported families), PIC32CZ
- **UART:** STM32 (all supported families, DMA variants on WB and WBA),
  PIC32CZ, Arm PL011
- **SPI:** STM32 (all supported families)
- **I2C:** STM32 F0, F3, L1, N6, U5, WB, WB0, WBA
- **Flash:** STM32 on-chip flash for C0, F0, F3, F4, H5, L1, U5, WB, WB0,
  WBA; PIC32CZ on-chip flash; SPI NOR; SPI memory NOR
- **SPI memory:** Cadence OSPI
- **DMA:** STM32 DMA/GPDMA for N6, U5, WB, WB0, WBA; Versal Gen 2 OSPI DMA
- **RNG:** STM32 H5, N6, U5, WB, WB0, WBA
- **AES:** ECB, CBC, CTR, GCM, GMAC, CCM on STM32 N6, U5, WB, WBA
- **Hash:** SHA-1, SHA-224, SHA-256 and HMAC on STM32 N6, U5, WBA
- **PKA:** STM32 WB, WB0
- **Watchdog:** STM32 IWDG and WWDG
- **Ethernet:** STM32 H5 and N6 MAC, LAN8742A PHY
- **Timer:** Arm SysTick
- **IRQ:** Arm Cortex-M4 NVIC
- **PWM:** STM32WB LPTIM
- **Block:** SD card over SPI
- **Display:** Sharp memory display
- **Sensor:** Bosch BMI270 IMU

### Boards

- NUCLEO-C031C6, NUCLEO-F091RC, NUCLEO-F302R8, NUCLEO-H563ZI,
  NUCLEO-L152RE, NUCLEO-N657X0-Q, NUCLEO-U5A5ZJ-Q, NUCLEO-WB05KZ,
  NUCLEO-WB55RG, NUCLEO-WBA55CG
- WeAct BlackPill STM32F411
- Microchip PIC32CZ CA Curiosity Ultra
- AMD VEK385 (Versal Gen 2), also runs on QEMU `amd-versal2-virt`
- External peripherals under `boards/peripheral`: Winbond W25Q64 SPI NOR,
  32 GB SD card over SPI, Sharp LS013B7DH03 display, BMI270 IMU

### Other

- Version macros in `wolfHAL/version.h`
