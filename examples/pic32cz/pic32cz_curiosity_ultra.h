#ifndef WHAL_PIC32CZ_CURIOSITY_ULTRA
#define WHAL_PIC32CZ_CURIOSITY_ULTRA

#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/platform/microchip/pic32cz.h>

WHAL_SUPPLY_DEV_DECLARE(supply, Pic32czSupc)
WHAL_CLOCK_DEV_DECLARE(clock, Pic32czClockPll)
WHAL_GPIO_DEV_DECLARE(gpio, Pic32czGpio)
WHAL_TIMER_DEV_DECLARE(timer, SysTick)
WHAL_UART_DEV_DECLARE(uart, Pic32czUart)
WHAL_FLASH_DEV_DECLARE(flash, Pic32czFlash)

#endif /* WHAL_PIC32CZ_CURIOSITY_ULTRA */
