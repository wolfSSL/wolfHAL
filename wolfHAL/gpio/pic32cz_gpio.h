/* pic32cz_gpio.h
 *
 * Copyright (C) 2026 wolfSSL Inc.
 *
 * This file is part of wolfHAL.
 *
 * wolfHAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfHAL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#ifndef WHAL_PIC32CZ_GPIO_H
#define WHAL_PIC32CZ_GPIO_H

#include <stdint.h>
#include <wolfHAL/gpio/gpio.h>

/*
 * @file pic32cz_gpio.h
 * @brief PIC32CZ PORT (GPIO) driver configuration.
 *
 * The PIC32CZ PORT module provides GPIO functionality with:
 * - Multiple ports (A, B, C, etc.) each with up to 32 pins
 * - Configurable direction (input/output)
 * - Optional input enable and pull resistors
 * - Peripheral multiplexing (PMUX) for alternate functions
 *
 * Each pin can either be used as GPIO or connected to a peripheral
 * function via the PMUX registers. When pmuxEn is set, the pin is
 * controlled by the selected peripheral rather than GPIO.
 */

/*
 * @brief Pin direction selection.
 */
enum {
    WHAL_PIC32CZ_DIR_INPUT,  /* Configure pin as input */
    WHAL_PIC32CZ_DIR_OUTPUT, /* Configure pin as output */
};

/*
 * @brief Peripheral multiplexer function selection.
 *
 * Each pin can be assigned to one of several peripheral functions (A-N).
 * The available functions vary by pin - consult the datasheet's
 * I/O Multiplexing table for valid assignments.
 */
enum {
    WHAL_PIC32CZ_PMUX_EIC = 0x0,  /* External Interrupts */
    WHAL_PIC32CZ_PMUX_AC = 0x1,  /* ADC and Analog Comparator */
    WHAL_PIC32CZ_PMUX_SERCOM = 0x2,  /* SERCOMn (UART, I2C, SPI) */
    WHAL_PIC32CZ_PMUX_SERCOM_ALT = 0x3,  /* SERCOMn (UART, I2C, SPI) */
    WHAL_PIC32CZ_PMUX_EBI = 0x4,  /* External Bus Interface */
    WHAL_PIC32CZ_PMUX_TCC = 0x5,  /* Timer/counter controller */
    WHAL_PIC32CZ_PMUX_TCC_ALT_PDEC = 0x6,  /* Timer/counter controller
                                              and positional decoder */
    WHAL_PIC32CZ_PMUX_COM = 0x7,  /* SQI/CAN/USB */
    WHAL_PIC32CZ_PMUX_SDMMC = 0x8,  /* SD/MMC Host Controller */
    WHAL_PIC32CZ_PMUX_SPI_IXS = 0x9,  /* SPI_IXS Audio */
    WHAL_PIC32CZ_PMUX_PCC = 0xA,  /* Parallel Capture Controller */
    WHAL_PIC32CZ_PMUX_ETH = 0xB,  /* Ethernet */
    WHAL_PIC32CZ_PMUX_MISC = 0xC,  /* GCLK/CCL/AC Alt */
    WHAL_PIC32CZ_PMUX_PTC = 0xD,  /* Peripheral Touch Controller */
};

/*
 * @brief Single pin configuration.
 *
 * Describes the configuration for one GPIO pin. Pins can be configured
 * either as standard GPIO (input/output) or as peripheral function pins.
 *
 * For GPIO mode (pmuxEn = 0):
 *   - dir: Input or output direction
 *   - inEn: Enable input buffer (required for reading input pins)
 *   - pullEn: Enable internal pull resistor
 *   - out: Initial output value (for output pins)
 *
 * For peripheral mode (pmuxEn = 1):
 *   - pmux: Peripheral function (A-N) to assign
 *   - Other fields are ignored; peripheral controls the pin
 */
typedef struct whal_Pic32cz_Gpio_PinCfg {
    uint8_t port;   /* Port index (0=A, 1=B, 2=C, ...) */
    uint8_t pin;    /* Pin number within port (0-31) */
    uint8_t dir;    /* Direction: WHAL_PIC32CZ_DIR_INPUT/OUTPUT */
    uint8_t inEn;   /* Input buffer enable (1=enabled) */
    uint8_t pullEn; /* Pull resistor enable (1=enabled) */
    uint8_t out;    /* Initial output value (0 or 1) */
    uint8_t pmuxEn; /* Peripheral mux enable (1=use pmux function) */
    uint8_t pmux;   /* Peripheral function (WHAL_PIC32CZ_PMUX_x) */
} whal_Pic32cz_Gpio_PinCfg;

/*
 * @brief GPIO device configuration.
 *
 * Contains an array of pin configurations to be applied during Init.
 */
typedef struct whal_Pic32cz_Gpio_Cfg {
    size_t pinCfgCount;              /* Number of pins to configure */
    whal_Pic32cz_Gpio_PinCfg *pinCfg; /* Array of pin configurations */
} whal_Pic32cz_Gpio_Cfg;

/*
 * @brief Platform-owned GPIO device singleton. Defined in the driver TU
 * from the WHAL_CFG_PIC32CZ_GPIO_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Gpio whal_Pic32cz_Gpio_Dev;

#ifndef WHAL_CFG_PIC32CZ_GPIO_DIRECT_API_MAPPING
/*
 * @brief Driver instance for PIC32CZ GPIO.
 */
extern const whal_GpioDriver whal_Pic32cz_Gpio_Driver;

/*
 * @brief Initialize the PIC32CZ GPIO peripheral.
 *
 * @param gpioDev GPIO device instance.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Gpio_Init(whal_Gpio *gpioDev);
/*
 * @brief Deinitialize the PIC32CZ GPIO peripheral.
 *
 * @param gpioDev GPIO device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Gpio_Deinit(whal_Gpio *gpioDev);
/*
 * @brief Read a GPIO pin value.
 *
 * @param gpioDev GPIO device instance.
 * @param pin     Pin index in the configured pin table.
 * @param value   Output for the sampled pin value.
 *
 * @retval WHAL_SUCCESS Pin value read.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Gpio_Get(whal_Gpio *gpioDev, size_t pin, size_t *value);
/*
 * @brief Set a GPIO pin value.
 *
 * @param gpioDev GPIO device instance.
 * @param pin     Pin index in the configured pin table.
 * @param value   Value to drive.
 *
 * @retval WHAL_SUCCESS Pin updated.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Gpio_Set(whal_Gpio *gpioDev, size_t pin, size_t value);
#endif /* !WHAL_CFG_PIC32CZ_GPIO_DIRECT_API_MAPPING */

#endif /* WHAL_PIC32CZ_GPIO_H */
