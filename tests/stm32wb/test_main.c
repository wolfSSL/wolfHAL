#include <stdint.h>
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/timer/timer.h>
#include <wolfHAL/bitops.h>
#include "stm32wb55xx_nucleo.h"
#include "../test.h"

int g_whalTestPassed;
int g_whalTestFailed;
int g_whalTestCurFailed;

volatile size_t g_tick = 0;

void SysTick_Handler(void)
{
    g_tick++;
}

/* whalTest_Puts: send a string over UART, translating \n to \r\n */
void whalTest_Puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            WHAL_UART_SEND(uart, (const uint8_t *)"\r\n", 2);
        else
            WHAL_UART_SEND(uart, (const uint8_t *)s, 1);
        s++;
    }
}

/* Busy-wait delay using SysTick */
static void delay_ms(size_t ms)
{
    size_t start = g_tick;
    while (g_tick - start < ms)
        ;
}

void test_clock(void);
void test_gpio(void);
void test_flash(void);
void test_timer(void);
void test_rng(void);

void main(void)
{
    whal_Error err;

    g_whalTestPassed = 0;
    g_whalTestFailed = 0;

    /* Bootstrap: clock -> GPIO -> UART -> timer */
    err = WHAL_CLOCK_INIT(clock);
    if (err)
        goto fail;

    err = WHAL_GPIO_INIT(gpio);
    if (err)
        goto fail;

    /* LED on to indicate boot */
    WHAL_GPIO_SET(gpio, LED_PIN, 1);

    err = WHAL_UART_INIT(uart);
    if (err)
        goto fail;

    err = WHAL_FLASH_INIT(flash);
    if (err)
        goto fail;

    err = WHAL_TIMER_INIT(timer);
    if (err)
        goto fail;

    err = WHAL_TIMER_START(timer);
    if (err)
        goto fail;

    whalTest_Printf("wolfHAL HW Test Suite\n");
    whalTest_Printf("=====================\n");

    /* Run test suites */
    test_clock();
    test_gpio();
    test_flash();
    test_timer();
    test_rng();

    WHAL_TEST_SUMMARY();

    /* Visual indication: solid LED = all pass, blink = failure */
    if (g_whalTestFailed == 0) {
        WHAL_GPIO_SET(gpio, LED_PIN, 1);
        while (1)
            ;
    }

fail:
    /* Rapid blink = failure */
    while (1) {
        WHAL_GPIO_SET(gpio, LED_PIN, 1);
        delay_ms(100);
        WHAL_GPIO_SET(gpio, LED_PIN, 0);
        delay_ms(100);
    }
}
