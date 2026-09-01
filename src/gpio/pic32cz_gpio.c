/* pic32cz_gpio.c
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

#include "wolfHAL_board.h"  /* provides WHAL_CFG_PIC32CZ_GPIO_DEV initializer */
#include <wolfHAL/gpio/pic32cz_gpio.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/error.h>

const whal_Gpio whal_Pic32cz_Gpio_Dev = WHAL_CFG_PIC32CZ_GPIO_DEV;

/*
 * PIC32CZ PORT Register Map
 *
 * Each port (A, B, C, ...) has a 0x80 byte register group containing:
 * - DIR: Direction register (1=output, 0=input)
 * - OUT: Output value register
 * - IN: Input value register (read-only)
 * - PMUX: Peripheral multiplexer selection (4 bits per pin, packed)
 * - PINCFG: Per-pin configuration (input enable, pull enable, pmux enable)
 */

/* Direction register - bit N controls pin N (1=output, 0=input) */
#define DIR_REG(port) (0x00 + (port * 0x80))

/* Output register - bit N is the output value for pin N */
#define OUT_REG(port) (0x10 + (port * 0x80))

/* Input register - bit N is the sampled input value for pin N */
#define IN_REG(port) (0x20 + (port * 0x80))

/*
 * Peripheral multiplexer register - selects alternate function for pins.
 * Each byte contains two 4-bit PMUX fields:
 *   - Lower nibble (bits 3:0): Even pin PMUX
 *   - Upper nibble (bits 7:4): Odd pin PMUX
 *
 * The APB bridge only supports 32-bit access, so the offset is word-aligned
 * and the shift positions the pin's nibble within the 32-bit word.
 */
#define PMUXx_REG(port, pin) ((0x30 + (((pin) / 8) * 4)) + ((port) * 0x80))
#define PMUXx_Pos(pin) (((pin) % 8) * 4)
#define PMUXx_Msk(pin) (WHAL_BITMASK(4) << (PMUXx_Pos(pin)))

/*
 * Pin configuration register - one byte per pin.
 *
 * Word-aligned for 32-bit access through the APB bridge. The pin argument
 * positions each field's mask within the 32-bit word.
 */
#define PINCFGx_REG(port, pin) ((0x40 + ((pin) & ~3)) + ((port) * 0x80))
#define PINCFGx_PMUXEN_Pos(pin) (((pin) & 3) * 8)
#define PINCFGx_PMUXEN_Msk(pin) (1UL << (PINCFGx_PMUXEN_Pos(pin)))

#define PINCFGx_INEN_Pos(pin)   (1 + (((pin) & 3) * 8))
#define PINCFGx_INEN_Msk(pin)   (1UL << (PINCFGx_INEN_Pos(pin)))

#define PINCFGx_PULLEN_Pos(pin) (2 + (((pin) & 3) * 8))
#define PINCFGx_PULLEN_Msk(pin) (1UL << (PINCFGx_PULLEN_Pos(pin)))

#ifdef WHAL_CFG_PIC32CZ_GPIO_DIRECT_API_MAPPING
#define whal_Pic32cz_Gpio_Init   whal_Gpio_Init
#define whal_Pic32cz_Gpio_Deinit whal_Gpio_Deinit
#define whal_Pic32cz_Gpio_Get    whal_Gpio_Get
#define whal_Pic32cz_Gpio_Set    whal_Gpio_Set
#endif /* WHAL_CFG_PIC32CZ_GPIO_DIRECT_API_MAPPING */

whal_Error whal_Pic32cz_Gpio_Init(whal_Gpio *gpioDev)
{
    const whal_Pic32cz_Gpio_Cfg *cfg =
        (const whal_Pic32cz_Gpio_Cfg *)whal_Pic32cz_Gpio_Dev.cfg;
    size_t base = whal_Pic32cz_Gpio_Dev.base;
    (void)gpioDev;

    for (size_t i = 0; i < cfg->pinCfgCount; ++i) {
        whal_Pic32cz_Gpio_PinCfg *pinCfg = &cfg->pinCfg[i];
        size_t pinMask = (1UL << (pinCfg->pin));

        if (pinCfg->pmuxEn) {
            /*
             * Configure pin for peripheral function:
             * 1. Set the PMUX value to select the peripheral function
             * 2. Enable PMUXEN in PINCFG to route pin to peripheral
             */
            size_t pmuxMask = PMUXx_Msk(pinCfg->pin);
            size_t pmuxPos = PMUXx_Pos(pinCfg->pin);
            size_t pmuxenMask = PINCFGx_PMUXEN_Msk(pinCfg->pin);
            size_t pmuxenPos = PINCFGx_PMUXEN_Pos(pinCfg->pin);

            whal_Reg_Update(base,
                            PMUXx_REG(pinCfg->port, pinCfg->pin),
                            pmuxMask,
                            whal_SetBits(pmuxMask, pmuxPos, pinCfg->pmux));

            whal_Reg_Update(base,
                            PINCFGx_REG(pinCfg->port, pinCfg->pin),
                            pmuxenMask,
                            whal_SetBits(pmuxenMask, pmuxenPos, 1));

            continue;
        }

        /*
         * Configure pin for GPIO function:
         * 1. Set direction (input or output)
         * 2. Set initial output value (for outputs)
         * 3. Configure input buffer enable and pull resistor
         */

        /* Set pin direction */
        whal_Reg_Update(base, DIR_REG(pinCfg->port),
                        pinMask,
                        whal_SetBits(pinMask, pinCfg->pin, pinCfg->dir));

        /* Set initial output value */
        whal_Reg_Update(base, OUT_REG(pinCfg->port),
                        pinMask,
                        whal_SetBits(pinMask, pinCfg->pin, pinCfg->out));

        /* Configure input buffer and pull resistor */
        {
            size_t inenMask = PINCFGx_INEN_Msk(pinCfg->pin);
            size_t inenPos = PINCFGx_INEN_Pos(pinCfg->pin);
            size_t pullenMask = PINCFGx_PULLEN_Msk(pinCfg->pin);
            size_t pullenPos = PINCFGx_PULLEN_Pos(pinCfg->pin);

            whal_Reg_Update(base,
                            PINCFGx_REG(pinCfg->port, pinCfg->pin),
                            inenMask | pullenMask,
                            whal_SetBits(inenMask, inenPos, pinCfg->inEn) |
                            whal_SetBits(pullenMask, pullenPos, pinCfg->pullEn));
        }
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Gpio_Deinit(whal_Gpio *gpioDev)
{
    (void)gpioDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Gpio_Get(whal_Gpio *gpioDev, size_t pin, size_t *value)
{
    const whal_Pic32cz_Gpio_Cfg *cfg =
        (const whal_Pic32cz_Gpio_Cfg *)whal_Pic32cz_Gpio_Dev.cfg;
    size_t base = whal_Pic32cz_Gpio_Dev.base;
    whal_Pic32cz_Gpio_PinCfg *pinCfg;
    size_t pinMask;
    size_t reg;
    (void)gpioDev;

    if (!value || pin >= cfg->pinCfgCount)
        return WHAL_EINVAL;

    pinCfg = &cfg->pinCfg[pin];
    pinMask = (1UL << (pinCfg->pin));

    /*
     * Read from appropriate register based on pin direction:
     * - Output pins: Read back from OUT register (current driven value)
     * - Input pins: Read from IN register (sampled external value)
     */
    if (pinCfg->dir == WHAL_PIC32CZ_DIR_OUTPUT) {
        reg = OUT_REG(pinCfg->port);
    }
    else {
        reg = IN_REG(pinCfg->port);
    }

    whal_Reg_Get(base, reg, pinMask, pinCfg->pin, value);

    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Gpio_Set(whal_Gpio *gpioDev, size_t pin, size_t value)
{
    const whal_Pic32cz_Gpio_Cfg *cfg =
        (const whal_Pic32cz_Gpio_Cfg *)whal_Pic32cz_Gpio_Dev.cfg;
    size_t base = whal_Pic32cz_Gpio_Dev.base;
    whal_Pic32cz_Gpio_PinCfg *pinCfg;
    size_t pinMask;
    (void)gpioDev;

    if (pin >= cfg->pinCfgCount)
        return WHAL_EINVAL;

    pinCfg = &cfg->pinCfg[pin];
    pinMask = (1UL << (pinCfg->pin));

    /* Update the output register to drive the new value */
    whal_Reg_Update(base, OUT_REG(pinCfg->port),
                    pinMask,
                    whal_SetBits(pinMask, pinCfg->pin, value));

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_PIC32CZ_GPIO_DIRECT_API_MAPPING
const whal_GpioDriver whal_Pic32cz_Gpio_Driver = {
    .Init = whal_Pic32cz_Gpio_Init,
    .Deinit = whal_Pic32cz_Gpio_Deinit,
    .Get = whal_Pic32cz_Gpio_Get,
    .Set = whal_Pic32cz_Gpio_Set,
};
#endif /* !WHAL_CFG_PIC32CZ_GPIO_DIRECT_API_MAPPING */
