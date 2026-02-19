#include <stdint.h>
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/bitops.h>
#include "stm32wb55xx_nucleo.h"

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

    err = WHAL_CLOCK_INIT(clock);
    if (err) {
        goto loop;
    }

    err = WHAL_GPIO_INIT(gpio);
    if (err) {
        goto loop;
    }
    err = WHAL_GPIO_SET(gpio, LED_PIN, 1);
    if (err) {
        goto loop;
    }

    err = WHAL_UART_INIT(uart);
    if (err) {
        goto loop;
    }

    err = WHAL_FLASH_INIT(flash);
    if (err) {
        goto loop;
    }

    err = WHAL_TIMER_INIT(timer);
    if (err) {
        goto loop;
    }

    err = WHAL_TIMER_START(timer);
    if (err) {
        goto loop;
    }

    WHAL_FLASH_UNLOCK(flash, 0, 0);

    uint8_t data[] = "TESTING TESTING HELLO\r\n";
    uint8_t tmp[sizeof(data)] = {0};
    WHAL_FLASH_ERASE(flash, 0x08080000, 0x1000);

    do {
        err = WHAL_FLASH_WRITE(flash, 0x08080000, data, sizeof(data));
    } while (err == WHAL_ENOTREADY);

    WHAL_FLASH_READ(flash, 0x08080000, tmp, sizeof(tmp));

    WHAL_FLASH_LOCK(flash, 0, 0);

    WHAL_UART_SEND(uart, tmp, sizeof(tmp));

    while (1) {
        uint8_t input[8];
        err = WHAL_UART_SEND(uart, (uint8_t *)"Enter Stuff:\r\n", 14);
        if (err) {
            goto loop;
        }

        err = WHAL_UART_RECV(uart, input, sizeof(input));
        if (err) {
            goto loop;
        }

        err = WHAL_UART_SEND(uart, input, sizeof(input));
        if (err) {
            goto loop;
        }
        err = WHAL_GPIO_SET(gpio, LED_PIN, 1);
        if (err) {
            goto loop;
        }

        WaitMs(1000);

        err = WHAL_GPIO_SET(gpio, LED_PIN, 0);
        if (err) {
            goto loop;
        }

        WaitMs(1000);
    }

loop:
    while (1);

}
