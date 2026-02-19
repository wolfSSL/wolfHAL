#include <wolfHAL/platform/st/stm32wb55xx.h>
#include "stm32wb55xx_nucleo.h"

WHAL_CLOCK_DEV_DEFINE(clock, WHAL_STM32WB55_RCC_PLL_DRIVER, WHAL_STM32WB55_RCC_PLL_REGMAP,
    (&(whal_Stm32wbRcc_Cfg) {
        .flash = &whal_dev_flash,
        .flashLatency = WHAL_STM32WB_FLASH_LATENCY_3,

        .sysClkSrc = WHAL_STM32WB_RCC_SYSCLK_SRC_PLL,
        .sysClkCfg = &(whal_Stm32wbRcc_PllClkCfg)
        {
            .clkSrc = WHAL_STM32WB_RCC_PLLCLK_SRC_MSI,
            /* 64 MHz */
            .n = 32,
            .m = 0,
            .r = 1,
            .q = 0,
            .p = 0,
        },
    }));

WHAL_GPIO_DEV_DEFINE(gpio, WHAL_STM32WB55_GPIO_DRIVER, WHAL_STM32WB55_GPIO_REGMAP,
    (&(whal_Stm32wbGpio_Cfg) {
        .clkCtrl = &whal_dev_clock,
        .clk = (const void *[2]) {
            &(whal_Stm32wbRcc_Clk){WHAL_STM32WB55_GPIOA_CLOCK},
            &(whal_Stm32wbRcc_Clk){WHAL_STM32WB55_GPIOB_CLOCK},
        },
        .clkCount = 2,

        .pinCfg = (whal_Stm32wbGpio_PinCfg[7]) {
            [LED_PIN] = { /* LED */
                .port = WHAL_STM32WB_GPIO_PORT_B,
                .pin = 5,
                .mode = WHAL_STM32WB_GPIO_MODE_OUT,
                .outType = WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL,
                .speed = WHAL_STM32WB_GPIO_SPEED_LOW,
                .pull = WHAL_STM32WB_GPIO_PULL_UP,
                .altFn = 0,
            },
            [LPUART1_TX_PIN] = { /* UART1 TX */
                .port = WHAL_STM32WB_GPIO_PORT_B,
                .pin = 6,
                .mode = WHAL_STM32WB_GPIO_MODE_ALTFN,
                .outType = WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL,
                .speed = WHAL_STM32WB_GPIO_SPEED_FAST,
                .pull = WHAL_STM32WB_GPIO_PULL_UP,
                .altFn = 7,
            },
            [LPUART1_RX_PIN] = { /* UART1 RX */
                .port = WHAL_STM32WB_GPIO_PORT_B,
                .pin = 7,
                .mode = WHAL_STM32WB_GPIO_MODE_ALTFN,
                .outType = WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL,
                .speed = WHAL_STM32WB_GPIO_SPEED_FAST,
                .pull = WHAL_STM32WB_GPIO_PULL_UP,
                .altFn = 7,
            },
            [SPI1_SCK_PIN] = { /* SPI1 SCK */
                .port = WHAL_STM32WB_GPIO_PORT_A,
                .pin = 5,
                .mode = WHAL_STM32WB_GPIO_MODE_ALTFN,
                .outType = WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL,
                .speed = WHAL_STM32WB_GPIO_SPEED_FAST,
                .pull = WHAL_STM32WB_GPIO_PULL_UP,
                .altFn = 5,
            },
            [SPI1_MISO_PIN] = { /* SPI1 MISO */
                .port = WHAL_STM32WB_GPIO_PORT_A,
                .pin = 6,
                .mode = WHAL_STM32WB_GPIO_MODE_ALTFN,
                .outType = WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL,
                .speed = WHAL_STM32WB_GPIO_SPEED_FAST,
                .pull = WHAL_STM32WB_GPIO_PULL_UP,
                .altFn = 5,
            },
            [SPI1_MOSI_PIN] = { /* SPI1 MOSI */
                .port = WHAL_STM32WB_GPIO_PORT_A,
                .pin = 7,
                .mode = WHAL_STM32WB_GPIO_MODE_ALTFN,
                .outType = WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL,
                .speed = WHAL_STM32WB_GPIO_SPEED_FAST,
                .pull = WHAL_STM32WB_GPIO_PULL_UP,
                .altFn = 5,
            },
            [CS_PIN] = { /* SPI1 CS */
                .port = WHAL_STM32WB_GPIO_PORT_A,
                .pin = 4,
                .mode = WHAL_STM32WB_GPIO_MODE_OUT,
                .outType = WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL,
                .speed = WHAL_STM32WB_GPIO_SPEED_LOW,
                .pull = WHAL_STM32WB_GPIO_PULL_UP,
                .altFn = 0,
            },
        },
        .pinCount = 7,
    }));

WHAL_TIMER_DEV_DEFINE(timer, WHAL_CORTEX_M4_SYSTICK_DRIVER, WHAL_CORTEX_M4_SYSTICK_REGMAP,
    (&(whal_SysTick_Cfg) {
        .cyclesPerTick = 64000000 / 1000,
        .clkSrc = WHAL_SYSTICK_CLKSRC_SYSCLK,
        .tickInt = WHAL_SYSTICK_TICKINT_ENABLED,
    }));

WHAL_UART_DEV_DEFINE(uart, WHAL_STM32WB55_UART1_DRIVER, WHAL_STM32WB55_UART1_REGMAP,
    (&(whal_Stm32wbUart_Cfg) {
        .clkCtrl = &whal_dev_clock,
        .clk = &(whal_Stm32wbRcc_Clk) {WHAL_STM32WB55_UART1_CLOCK},

        .baud = 115200,
    }));

WHAL_FLASH_DEV_DEFINE(flash, WHAL_STM32WB55_FLASH_DRIVER, WHAL_STM32WB55_FLASH_REGMAP,
    (&(whal_Stm32wbFlash_Cfg) {
        .clkCtrl = &whal_dev_clock,
        .clk = &(whal_Stm32wbRcc_Clk) {WHAL_STM32WB55_FLASH_CLOCK},

        .startAddr = 0x08000000,
        .size = 0x100000,
    }));

WHAL_SPI_DEV_DEFINE(spi, WHAL_STM32WB55_SPI1_DRIVER, WHAL_STM32WB55_SPI1_REGMAP,
    (&(whal_Stm32wbSpi_Cfg) {
        .clkCtrl = &whal_dev_clock,
        .clk = &(whal_Stm32wbRcc_Clk) {WHAL_STM32WB55_SPI1_CLOCK},
    }));

WHAL_RNG_DEV_DEFINE(rng, WHAL_STM32WB55_RNG_DRIVER, WHAL_STM32WB55_RNG_REGMAP,
    (&(whal_Stm32wbRng_Cfg) {
        .clkCtrl = &whal_dev_clock,
        .clk = &(whal_Stm32wbRcc_Clk) {WHAL_STM32WB55_RNG_CLOCK},
    }));
