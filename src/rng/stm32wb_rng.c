#include <stdint.h>
#include <wolfHAL/rng/stm32wb_rng.h>
#include <wolfHAL/rng/rng.h>
#include <wolfHAL/clock/clock.h>
#include <wolfHAL/clock/stm32wb_rcc.h>
#include <wolfHAL/error.h>
#include <wolfHAL/regmap.h>
#include <wolfHAL/bitops.h>

/*
 * STM32WB RNG Register Definitions
 *
 * The RNG peripheral uses an analog noise source to produce 32-bit
 * random values. One value is available at a time in DR, signaled
 * by the DRDY flag in SR.
 */

/* Control Register */
#define SRNG_CR_REG   0x00
#define SRNG_CR_RNGEN WHAL_MASK(2) /* RNG enable */
#define SRNG_CR_CED   WHAL_MASK(5) /* Clock error detection disable */

/* Status Register */
#define SRNG_SR_REG  0x04
#define SRNG_SR_DRDY WHAL_MASK(0) /* Data ready */
#define SRNG_SR_CECS WHAL_MASK(1) /* Clock error current status */
#define SRNG_SR_SECS WHAL_MASK(2) /* Seed error current status */
#define SRNG_SR_CEIS WHAL_MASK(5) /* Clock error interrupt status */
#define SRNG_SR_SEIS WHAL_MASK(6) /* Seed error interrupt status */

/* Data Register - 32-bit random value */
#define SRNG_DR_REG  0x08

whal_Error WHAL_DRV_FN(Stm32wbRng, init)(whal_Rng *rngDev)
{
    whal_Error err;
    whal_Stm32wbRng_Cfg *cfg;

    if (!rngDev || !rngDev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Stm32wbRng_Cfg *)rngDev->cfg;

    err = WHAL_DRV_FN(Stm32wbRccPll, enable)(cfg->clkCtrl, cfg->clk);
    if (err != WHAL_SUCCESS) {
        return err;
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbRng, deinit)(whal_Rng *rngDev)
{
    whal_Error err;

    if (!rngDev || !rngDev->cfg) {
        return WHAL_EINVAL;
    }

    whal_Stm32wbRng_Cfg *cfg = (whal_Stm32wbRng_Cfg *)rngDev->cfg;

    err = WHAL_DRV_FN(Stm32wbRccPll, disable)(cfg->clkCtrl, cfg->clk);
    if (err) {
        return err;
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbRng, generate)(whal_Rng *rngDev, uint8_t *rngData, size_t rngDataSz)
{
    if (!rngDev || !rngData) {
        return WHAL_EINVAL;
    }

    whal_Error err = WHAL_SUCCESS;
    const whal_Regmap *reg = &rngDev->regmap;
    size_t status;
    size_t offset = 0;

    /* Enable the RNG peripheral */
    whal_Reg_Update(reg->base, SRNG_CR_REG, SRNG_CR_RNGEN,
                    whal_SetBits(SRNG_CR_RNGEN, 1));

    while (offset < rngDataSz) {
        /* Wait for a random value to be ready */
        do {
            /* Check for seed or clock error */
            whal_Reg_Get(reg->base, SRNG_SR_REG, SRNG_SR_SECS, &status);
            if (status) {
                err = WHAL_EHARDWARE;
                goto exit;
            }
            whal_Reg_Get(reg->base, SRNG_SR_REG, SRNG_SR_CECS, &status);
            if (status) {
                err = WHAL_EHARDWARE;
                goto exit;
            }

            whal_Reg_Get(reg->base, SRNG_SR_REG, SRNG_SR_DRDY, &status);
        } while (!status);

        /* Read 32-bit random value */
        uint32_t rnd = *(volatile uint32_t *)(reg->base + SRNG_DR_REG);

        /* Copy bytes into output buffer */
        for (size_t i = 0; i < 4 && offset < rngDataSz; i++, offset++) {
            rngData[offset] = (uint8_t)(rnd >> (i * 8));
        }
    }

exit:
    /* Disable the RNG peripheral */
    whal_Reg_Update(reg->base, SRNG_CR_REG, SRNG_CR_RNGEN,
                    whal_SetBits(SRNG_CR_RNGEN, 0));

    return err;
}

