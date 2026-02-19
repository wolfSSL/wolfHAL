#include <wolfHAL/wolfHAL.h>
#include "pic32cz_curiosity_ultra.h"

volatile size_t g_tick = 0;
volatile uint8_t g_waiting = 0;
volatile uint8_t g_tickOverflow = 0;

void SysTick_Handler()
{
    size_t tickBefore = g_tick++;
    if (g_waiting) {
        if (tickBefore > g_tick)
            g_tickOverflow = 1;
    }
}

void WaitMs(size_t ms)
{
    size_t startCount = g_tick;
    g_waiting = 1;
    while (1) {
        size_t currentCount = g_tick;
        if (g_tickOverflow) {
            if ((SIZE_MAX - startCount) + currentCount > ms) {
                break;
            }
        } else if (currentCount - startCount > ms) {
            break;
        }
    }

    g_waiting = 0;
    g_tickOverflow = 0;
}

void main(void)
{
    whal_Error err;
    uint8_t data[] = "Hello world!\r\n";
    uint8_t test[] = "test1\r\n";
    uint8_t tmp[sizeof(test)] = {0};

    err = WHAL_CLOCK_INIT(clock);
    if (err) {
        goto loop;
    }

    err = WHAL_GPIO_INIT(gpio);
    if (err) {
        goto loop;
    }

    err = WHAL_UART_INIT(uart);
    if (err) {
        goto loop;
    }

    err = WHAL_UART_SEND(uart, data, sizeof(data));
    if (err) {
        goto loop;
    }

    err = WHAL_FLASH_INIT(flash);
    if (err) {
        goto loop;
    }

    err = WHAL_FLASH_ERASE(flash, 0x0C000000, 0x1000);
    if (err) {
        goto loop;
    }

    do {
        err = WHAL_FLASH_WRITE(flash, 0x0C000000, test, sizeof(test));
    } while (err == WHAL_ENOTREADY);

    if (err) {
        goto loop;
    }

    err = WHAL_FLASH_READ(flash, 0x0C000000, tmp, sizeof(tmp));
    if (err) {
        goto loop;
    }

    err = WHAL_UART_SEND(uart, tmp, sizeof(tmp));
    if (err) {
        goto loop;
    }

    err = WHAL_TIMER_INIT(timer);
    if (err) {
        goto loop;
    }

    WHAL_TIMER_START(timer);

    while (1) {
        WHAL_GPIO_SET(gpio, 0, 1);

        WaitMs(1000);

        WHAL_GPIO_SET(gpio, 0, 0);

        WaitMs(1000);
    }

loop:
    while (1);
}
