#include <wolfHAL/gpio/pic32cz_gpio.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/error.h>

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
#define PIC32CZ_DIR_REG(port) (0x00 + (port * 0x80))

/* Output register - bit N is the output value for pin N */
#define PIC32CZ_OUT_REG(port) (0x10 + (port * 0x80))

/* Input register - bit N is the sampled input value for pin N */
#define PIC32CZ_IN_REG(port) (0x20 + (port * 0x80))

/*
 * Peripheral multiplexer register - selects alternate function for pins.
 * Each byte contains two 4-bit PMUX fields:
 *   - Lower nibble (bits 3:0): Even pin PMUX
 *   - Upper nibble (bits 7:4): Odd pin PMUX
 *
 * The APB bridge only supports 32-bit access, so the offset is word-aligned
 * and the shift positions the pin's nibble within the 32-bit word.
 */
#define PIC32CZ_PMUXx_REG(port, pin) ((0x30 + (((pin) / 8) * 4)) + ((port) * 0x80))
#define PIC32CZ_PMUXx_MASK(pin) (0xFul << (((pin) % 8) * 4))

/*
 * Pin configuration register - one byte per pin.
 *
 * Word-aligned for 32-bit access through the APB bridge. The pin argument
 * positions each field's mask within the 32-bit word.
 */
#define PIC32CZ_PINCFGx_REG(port, pin) ((0x40 + ((pin) & ~3)) + ((port) * 0x80))
#define PIC32CZ_PINCFGx_PMUXEN(pin) (WHAL_MASK(0) << (((pin) & 3) * 8))
#define PIC32CZ_PINCFGx_INEN(pin)   (WHAL_MASK(1) << (((pin) & 3) * 8))
#define PIC32CZ_PINCFGx_PULLEN(pin) (WHAL_MASK(2) << (((pin) & 3) * 8))

whal_Error WHAL_DRV_FN(Pic32czGpio, init)(whal_Gpio *gpioDev)
{
    if (!gpioDev) {
        return WHAL_EINVAL;
    }

    const whal_Pic32czGpio_Cfg *cfg = gpioDev->cfg;

    for (size_t i = 0; i < cfg->pinCfgCount; ++i) {
        whal_Pic32czGpio_PinCfg *pinCfg = &cfg->pinCfg[i];
        size_t pinMask = WHAL_MASK(pinCfg->pin);

        if (pinCfg->pmuxEn) {
            /*
             * Configure pin for peripheral function:
             * 1. Set the PMUX value to select the peripheral function
             * 2. Enable PMUXEN in PINCFG to route pin to peripheral
             */
            size_t pmuxMask = PIC32CZ_PMUXx_MASK(pinCfg->pin);
            size_t pmuxenMask = PIC32CZ_PINCFGx_PMUXEN(pinCfg->pin);

            whal_Reg_Update(gpioDev->regmap.base,
                            PIC32CZ_PMUXx_REG(pinCfg->port, pinCfg->pin),
                            pmuxMask,
                            whal_SetBits(pmuxMask, pinCfg->pmux));

            whal_Reg_Update(gpioDev->regmap.base,
                            PIC32CZ_PINCFGx_REG(pinCfg->port, pinCfg->pin),
                            pmuxenMask,
                            whal_SetBits(pmuxenMask, 1));

            continue;
        }

        /*
         * Configure pin for GPIO function:
         * 1. Set direction (input or output)
         * 2. Set initial output value (for outputs)
         * 3. Configure input buffer enable and pull resistor
         */

        /* Set pin direction */
        whal_Reg_Update(gpioDev->regmap.base, PIC32CZ_DIR_REG(pinCfg->port),
                        pinMask,
                        whal_SetBits(pinMask, pinCfg->dir));

        /* Set initial output value */
        whal_Reg_Update(gpioDev->regmap.base, PIC32CZ_OUT_REG(pinCfg->port),
                        pinMask,
                        whal_SetBits(pinMask, pinCfg->out));

        /* Configure input buffer and pull resistor */
        {
            size_t inenMask = PIC32CZ_PINCFGx_INEN(pinCfg->pin);
            size_t pullenMask = PIC32CZ_PINCFGx_PULLEN(pinCfg->pin);

            whal_Reg_Update(gpioDev->regmap.base,
                            PIC32CZ_PINCFGx_REG(pinCfg->port, pinCfg->pin),
                            inenMask | pullenMask,
                            whal_SetBits(inenMask, pinCfg->inEn) |
                            whal_SetBits(pullenMask, pinCfg->pullEn));
        }
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czGpio, deinit)(whal_Gpio *gpioDev)
{
    if (!gpioDev) {
        return WHAL_EINVAL;
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czGpio, get)(whal_Gpio *gpioDev, size_t pin, size_t *value)
{
    if (!gpioDev || !value) {
        return WHAL_EINVAL;
    }

    const whal_Pic32czGpio_Cfg *cfg = gpioDev->cfg;
    whal_Pic32czGpio_PinCfg *pinCfg = &cfg->pinCfg[pin];
    size_t pinMask = WHAL_MASK(pinCfg->pin);
    size_t reg;

    /*
     * Read from appropriate register based on pin direction:
     * - Output pins: Read back from OUT register (current driven value)
     * - Input pins: Read from IN register (sampled external value)
     */
    if (pinCfg->dir == WHAL_PIC32CZ_DIR_OUTPUT) {
        reg = PIC32CZ_OUT_REG(pinCfg->port);
    }
    else {
        reg = PIC32CZ_IN_REG(pinCfg->port);
    }

    whal_Reg_Get(gpioDev->regmap.base, reg, pinMask, value);

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czGpio, set)(whal_Gpio *gpioDev, size_t pin, size_t value)
{
    if (!gpioDev) {
        return WHAL_EINVAL;
    }

    const whal_Pic32czGpio_Cfg *cfg = gpioDev->cfg;
    whal_Pic32czGpio_PinCfg *pinCfg = &cfg->pinCfg[pin];
    size_t pinMask = WHAL_MASK(pinCfg->pin);

    /* Update the output register to drive the new value */
    whal_Reg_Update(gpioDev->regmap.base, PIC32CZ_OUT_REG(pinCfg->port),
                    pinMask,
                    whal_SetBits(pinMask, value));

    return WHAL_SUCCESS;
}

