#ifndef WHAL_PIC32CZ_H
#define WHAL_PIC32CZ_H

#include <wolfHAL/clock/pic32cz_clock.h>
#include <wolfHAL/supply/pic32cz_supc.h>
#include <wolfHAL/gpio/pic32cz_gpio.h>
#include <wolfHAL/uart/pic32cz_uart.h>
#include <wolfHAL/flash/pic32cz_flash.h>
#include <wolfHAL/platform/arm/cortex_m7.h>

#define WHAL_PIC32CZ_FLASH_REGMAP       { .base = 0x44002000, .size = 0x4000 }
#define WHAL_PIC32CZ_FLASH_DRIVER       Pic32czFlash

#define WHAL_PIC32CZ_SUPPLY_REGMAP      { .base = 0x44020000, .size = 0x2000 }
#define WHAL_PIC32CZ_SUPPLY_DRIVER      Pic32czSupc

#define WHAL_PIC32CZ_CLOCK_PLL_REGMAP   { .base = 0x44040000, .size = 0x14000 }
#define WHAL_PIC32CZ_CLOCK_PLL_DRIVER   Pic32czClockPll

#define WHAL_PIC32CZ_GPIO_REGMAP        { .base = 0x44840000, .size = 0x2000 }
#define WHAL_PIC32CZ_GPIO_DRIVER        Pic32czGpio

#define WHAL_PIC32CZ_SERCOM4_UART_REGMAP    { .base = 0x46004000, .size = 0x2000 }
#define WHAL_PIC32CZ_SERCOM4_UART_DRIVER    Pic32czUart

#define WHAL_PIC32CZ_SUPPLY_PLL     \
    .enableMask = (1 << 18)

#endif /* WHAL_PIC32CZ_H */
