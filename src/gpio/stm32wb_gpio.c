#include <stdint.h>
#include <wolfHAL/error.h>
#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/gpio/stm32wb_gpio.h>
#include <wolfHAL/regmap.h>
#include <wolfHAL/bitops.h>

/*
 * STM32WB GPIO Register Definitions
 *
 * Each GPIO port has a 0x400 byte register block. Register offsets
 * are relative to the port base address.
 */

/* Size of each GPIO port register block */
#define STGPIO_PORT_SIZE 0x400

/* Mode register - 2 bits per pin, selects input/output/altfn/analog */
#define STGPIO_GPIOx_MODE_REG     0x00
/* Output type register - 1 bit per pin, push-pull or open-drain */
#define STGPIO_GPIOx_OUTTYPE_REG  0x04
/* Output speed register - 2 bits per pin */
#define STGPIO_GPIOx_SPEED_REG    0x08
/* Pull-up/pull-down register - 2 bits per pin */
#define STGPIO_GPIOx_PULL_REG     0x0C
/* Input data register - read-only, 1 bit per pin */
#define STGPIO_GPIOx_IDR_REG      0x10
/* Output data register - 1 bit per pin */
#define STGPIO_GPIOx_ODR_REG      0x14
/* Alternate function low register - 4 bits per pin for pins 0-7 */
#define STGPIO_GPIOx_ALTFNL_REG   0x20
/* Alternate function high register - 4 bits per pin for pins 8-15 */
#define STGPIO_GPIOx_ALTFNH_REG   0x24

/*
 * Configure alternate function for a pin.
 *
 * The AFRL (pins 0-7) and AFRH (pins 8-15) registers are combined into
 * a 64-bit value for easier manipulation. Each pin uses 4 bits to select
 * one of 16 alternate functions (AF0-AF15).
 */
static inline void whal_Stm32wbGpio_InitAltFn(whal_Regmap *portReg, whal_Stm32wbGpio_PinCfg *pinCfg)
{
    uint8_t pin = pinCfg->pin;
    uint8_t maskBit;
    uint64_t mask;

    /* Each pin uses 4 bits: pin 0 = bits 0-3, pin 1 = bits 4-7, etc. */
    maskBit = pin << 2;
    mask = WHAL_MASK_RANGE(maskBit + 3, maskBit);

    /* Access AFRL and AFRH as a single 64-bit register */
    uint64_t *reg = (uint64_t *)(portReg->base + STGPIO_GPIOx_ALTFNL_REG);
    *reg = (*reg & ~mask) | (whal_SetBits(mask, pinCfg->altFn) & mask);
}

/*
 * Initialize a single GPIO pin with the specified configuration.
 */
static inline whal_Error whal_Stm32wbGpio_InitPin(whal_Gpio *gpioDev, whal_Stm32wbGpio_PinCfg *pinCfg)
{
    whal_Regmap portReg;

    if (pinCfg->pin > 15) {
        return WHAL_EINVAL;
    }

    /* Calculate port base address */
    portReg.size = STGPIO_PORT_SIZE;
    portReg.base = (size_t)(gpioDev->regmap.base + (pinCfg->port * STGPIO_PORT_SIZE));

    uint8_t pin = pinCfg->pin;
    /* 2-bit field mask for MODE, SPEED, PULL registers */
    uint8_t maskBit = pin << 1;
    size_t mask1 = WHAL_MASK_RANGE(maskBit + 1, maskBit);
    /* 1-bit mask for OUTTYPE register */
    size_t mask2 = WHAL_MASK(pin);

    /* Configure pin mode (input/output/altfn/analog) */
    whal_Reg_Update(portReg.base, STGPIO_GPIOx_MODE_REG, mask1,
                    whal_SetBits(mask1, pinCfg->mode));

    /* Configure output speed */
    whal_Reg_Update(portReg.base, STGPIO_GPIOx_SPEED_REG, mask1,
                    whal_SetBits(mask1, pinCfg->speed));

    /* Configure output type (push-pull or open-drain) */
    whal_Reg_Update(portReg.base, STGPIO_GPIOx_OUTTYPE_REG, mask2,
                    whal_SetBits(mask2, pinCfg->outType));

    /* Configure alternate function if in ALTFN mode */
    if (pinCfg->mode == WHAL_STM32WB_GPIO_MODE_ALTFN) {
        whal_Stm32wbGpio_InitAltFn(&portReg, pinCfg);
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbGpio, init)(whal_Gpio *gpioDev)
{
    whal_Error err;
    whal_Stm32wbGpio_Cfg *cfg;
    whal_Stm32wbGpio_PinCfg *pinCfg;

    if (!gpioDev || !gpioDev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Stm32wbGpio_Cfg *)gpioDev->cfg;
    pinCfg = cfg->pinCfg;

    for (size_t i = 0; i < cfg->clkCount; ++i) {
        /* Enable GPIO port clock before accessing registers */
        err = WHAL_DRV_FN(Stm32wbRccPll, enable)(cfg->clkCtrl, cfg->clk[i]);
        if (err) {
            return err;
        }
    }

    /* Initialize each pin in the configuration array */
    for (size_t pin = 0; pin < cfg->pinCount; ++pin) {
        err = whal_Stm32wbGpio_InitPin(gpioDev, &pinCfg[pin]);
        if (err) {
            return err;
        }
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbGpio, deinit)(whal_Gpio *gpioDev)
{
    whal_Error err;
    whal_Stm32wbGpio_Cfg *cfg;

    cfg = (whal_Stm32wbGpio_Cfg *)gpioDev->cfg;

    for (size_t i = 0; i < cfg->clkCount; ++i) {
        /* Disable GPIO port clock */
        err = WHAL_DRV_FN(Stm32wbRccPll, disable)(cfg->clkCtrl, cfg->clk[i]);
        if (err) {
            return err;
        }
    }

    return WHAL_SUCCESS;
}

/*
 * Shared helper for reading or writing GPIO pin values.
 *
 * For set operations (set=1): writes value to ODR register
 * For get operations (set=0): reads value from IDR register
 */
static whal_Error whal_Stm32wbGpio_SetOrGet(whal_Gpio *gpioDev, size_t pin, size_t *value, uint8_t set)
{
    whal_Regmap portReg;
    whal_Stm32wbGpio_Cfg *cfg;
    whal_Stm32wbGpio_PinCfg *pinCfg;
    size_t mask;

    if (!gpioDev || !gpioDev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Stm32wbGpio_Cfg *)gpioDev->cfg;
    pinCfg = cfg->pinCfg;

    if (pinCfg[pin].pin > 15) {
        return WHAL_EINVAL;
    }

    /* Calculate port base address from config */
    portReg.size = STGPIO_PORT_SIZE;
    portReg.base = (size_t)(gpioDev->regmap.base + (pinCfg[pin].port * STGPIO_PORT_SIZE));

    mask = WHAL_MASK(pinCfg[pin].pin);
    if (set) {
        /* Write to output data register */
        whal_Reg_Update(portReg.base, STGPIO_GPIOx_ODR_REG, mask,
                        whal_SetBits(mask, *value));
    }
    else {
        /* Read from input data register */
        whal_Reg_Get(portReg.base, STGPIO_GPIOx_IDR_REG, mask, value);
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbGpio, get)(whal_Gpio *gpioDev, size_t pin, size_t *value)
{
    return whal_Stm32wbGpio_SetOrGet(gpioDev, pin, value, 0);
}

whal_Error WHAL_DRV_FN(Stm32wbGpio, set)(whal_Gpio *gpioDev, size_t pin, size_t value)
{
    return whal_Stm32wbGpio_SetOrGet(gpioDev, pin, &value, 1);
}
