#ifndef WHAL_STM32WB_GPIO_H
#define WHAL_STM32WB_GPIO_H

#include <stdint.h>
#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/clock/clock.h>
#include <wolfHAL/clock/stm32wb_rcc.h>
#include <wolfHAL/regmap.h>

/*
 * @file stm32wb_gpio.h
 * @brief STM32WB GPIO driver configuration types.
 *
 * The STM32WB GPIO peripheral provides:
 * - Up to 8 GPIO ports (A-H), each with up to 16 pins
 * - Configurable pin modes: input, output, alternate function, analog
 * - Output types: push-pull or open-drain
 * - Configurable output speed for EMI/power tradeoff
 * - Internal pull-up and pull-down resistors
 * - Alternate function mapping for peripheral connections (UART, SPI, etc.)
 *
 * Each port occupies 0x400 bytes in the memory map starting from GPIOA base.
 */

/*
 * @brief GPIO port identifiers.
 *
 * Port index is used to calculate register offset: base + (port * 0x400)
 */
typedef enum {
    WHAL_STM32WB_GPIO_PORT_A,
    WHAL_STM32WB_GPIO_PORT_B,
    WHAL_STM32WB_GPIO_PORT_C,
    WHAL_STM32WB_GPIO_PORT_D,
    WHAL_STM32WB_GPIO_PORT_E,
    WHAL_STM32WB_GPIO_PORT_F,
    WHAL_STM32WB_GPIO_PORT_G,
    WHAL_STM32WB_GPIO_PORT_H,
} whal_Stm32wbGpio_Port;

/*
 * @brief GPIO pin mode selection.
 *
 * Determines the basic function of the pin.
 */
typedef enum {
    WHAL_STM32WB_GPIO_MODE_IN,     /* Digital input */
    WHAL_STM32WB_GPIO_MODE_OUT,    /* Digital output */
    WHAL_STM32WB_GPIO_MODE_ALTFN,  /* Alternate function (peripheral control) */
    WHAL_STM32WB_GPIO_MODE_ANALOG, /* Analog mode (ADC/DAC) */
} whal_Stm32wbGpio_Mode;

/*
 * @brief Output driver type.
 */
typedef enum {
    WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL,  /* Push-pull output */
    WHAL_STM32WB_GPIO_OUTTYPE_OPENDRAIN, /* Open-drain output */
} whal_Stm32wbGpio_OutType;

/*
 * @brief Output speed settings.
 *
 * Higher speeds allow faster edge transitions but increase EMI and power.
 * Use the lowest speed that meets timing requirements.
 */
typedef enum {
    WHAL_STM32WB_GPIO_SPEED_LOW,
    WHAL_STM32WB_GPIO_SPEED_MEDIUM,
    WHAL_STM32WB_GPIO_SPEED_FAST,
    WHAL_STM32WB_GPIO_SPEED_HIGH,
} whal_Stm32wbGpio_Speed;

/*
 * @brief Internal pull resistor configuration.
 */
typedef enum {
    WHAL_STM32WB_GPIO_PULL_NONE, /* No pull resistor (floating) */
    WHAL_STM32WB_GPIO_PULL_UP,   /* Internal pull-up */
    WHAL_STM32WB_GPIO_PULL_DOWN, /* Internal pull-down */
} whal_Stm32wbGpio_Pull;

/*
 * @brief Per-pin GPIO configuration.
 *
 * For alternate function mode, consult the datasheet's "Alternate function
 * mapping" table to find the correct altFn value for your peripheral.
 * For example, USART1_TX on PA9 uses AF7.
 */
typedef struct {
    whal_Stm32wbGpio_Port port;      /* GPIO port (A-H) */
    uint8_t pin;                      /* Pin number (0-15) */
    whal_Stm32wbGpio_Mode mode;      /* Pin mode */
    whal_Stm32wbGpio_OutType outType; /* Output type (push-pull/open-drain) */
    whal_Stm32wbGpio_Speed speed;    /* Output speed */
    whal_Stm32wbGpio_Pull pull;      /* Pull resistor config */
    uint8_t altFn;                    /* Alternate function (0-15, AF0-AF15) */
} whal_Stm32wbGpio_PinCfg;

/*
 * @brief GPIO device configuration.
 *
 * Contains clock control references and an array of pin configurations.
 */
typedef struct {
    whal_Clock *clkCtrl; /* Clock controller for enabling GPIO clock */
    const void **clk;    /* Array of clock descriptors */
    size_t clkCount;     /* Number of clock descriptors */

    whal_Stm32wbGpio_PinCfg *pinCfg; /* Array of pin configurations */
    size_t pinCount;                  /* Number of pins to configure */
} whal_Stm32wbGpio_Cfg;

whal_Error WHAL_DRV_FN(Stm32wbGpio, init)(whal_Gpio *gpioDev);
whal_Error WHAL_DRV_FN(Stm32wbGpio, deinit)(whal_Gpio *gpioDev);
whal_Error WHAL_DRV_FN(Stm32wbGpio, get)(whal_Gpio *gpioDev, size_t pin, size_t *value);
whal_Error WHAL_DRV_FN(Stm32wbGpio, set)(whal_Gpio *gpioDev, size_t pin, size_t value);

#endif /* WHAL_STM32WB_GPIO_H */
