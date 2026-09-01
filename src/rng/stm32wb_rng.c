/* stm32wb_rng.c
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

#include <stdint.h>
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32WB_RNG_DEV initializer */
#include <wolfHAL/rng/stm32wb_rng.h>
#include <wolfHAL/rng/rng.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

const whal_Rng whal_Stm32wb_Rng_Dev = WHAL_CFG_STM32WB_RNG_DEV;

/*
 * STM32WB RNG Register Definitions
 *
 * The RNG peripheral uses an analog noise source to produce 32-bit
 * random values. One value is available at a time in DR, signaled
 * by the DRDY flag in SR.
 */

/* Control Register */
#define RNG_CR_REG   0x00
#define RNG_CR_RNGEN_Pos 2                                              /* RNG enable */
#define RNG_CR_RNGEN_Msk (1UL << RNG_CR_RNGEN_Pos)

#define RNG_CR_CED_Pos   5                                              /* Clock error detection disable */
#define RNG_CR_CED_Msk   (1UL << RNG_CR_CED_Pos)

/* Status Register */
#define RNG_SR_REG  0x04
#define RNG_SR_DRDY_Pos 0                                               /* Data ready */
#define RNG_SR_DRDY_Msk (1UL << RNG_SR_DRDY_Pos)

#define RNG_SR_CECS_Pos 1                                               /* Clock error current status */
#define RNG_SR_CECS_Msk (1UL << RNG_SR_CECS_Pos)

#define RNG_SR_SECS_Pos 2                                               /* Seed error current status */
#define RNG_SR_SECS_Msk (1UL << RNG_SR_SECS_Pos)

#define RNG_SR_CEIS_Pos 5                                               /* Clock error interrupt status */
#define RNG_SR_CEIS_Msk (1UL << RNG_SR_CEIS_Pos)

#define RNG_SR_SEIS_Pos 6                                               /* Seed error interrupt status */
#define RNG_SR_SEIS_Msk (1UL << RNG_SR_SEIS_Pos)

/* Data Register - 32-bit random value */
#define RNG_DR_REG  0x08

#ifdef WHAL_CFG_STM32WB_RNG_DIRECT_API_MAPPING
#define whal_Stm32wb_Rng_Init     whal_Rng_Init
#define whal_Stm32wb_Rng_Deinit   whal_Rng_Deinit
#define whal_Stm32wb_Rng_Generate whal_Rng_Generate
#endif /* WHAL_CFG_STM32WB_RNG_DIRECT_API_MAPPING */

whal_Error whal_Stm32wb_Rng_Init(whal_Rng *rngDev)
{
    (void)rngDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_Rng_Deinit(whal_Rng *rngDev)
{
    (void)rngDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_Rng_Generate(whal_Rng *rngDev, void *rngData, size_t rngDataSz)
{
    uint8_t *rngBuf = (uint8_t *)rngData;
    whal_Error err = WHAL_SUCCESS;
    const whal_Stm32wb_Rng_Cfg *cfg =
        (const whal_Stm32wb_Rng_Cfg *)whal_Stm32wb_Rng_Dev.cfg;
    size_t base = whal_Stm32wb_Rng_Dev.base;
    size_t sr;
    size_t offset = 0;
    (void)rngDev;

    if (!rngData) {
        return WHAL_EINVAL;
    }
#ifdef WHAL_CFG_NO_TIMEOUT
    (void)(cfg);
#endif

    /* Enable the RNG peripheral */
    whal_Reg_Update(base, RNG_CR_REG, RNG_CR_RNGEN_Msk,
                    whal_SetBits(RNG_CR_RNGEN_Msk, RNG_CR_RNGEN_Pos, 1));

    while (offset < rngDataSz) {
        /* Wait for a random value to be ready */
        WHAL_TIMEOUT_START(cfg->timeout);
        while (1) {
            if (WHAL_TIMEOUT_EXPIRED(cfg->timeout)) {
                err = WHAL_ETIMEOUT;
                goto exit;
            }

            sr = whal_Reg_Read(base, RNG_SR_REG);

            /* Check for seed or clock error */
            if (sr & RNG_SR_SECS_Msk) {
                err = WHAL_EHARDWARE;
                goto exit;
            }
            if (sr & RNG_SR_CECS_Msk) {
                err = WHAL_EHARDWARE;
                goto exit;
            }

            if (sr & RNG_SR_DRDY_Msk)
                break;
        }

        /* Read 32-bit random value */
        uint32_t rnd = *(volatile uint32_t *)(base + RNG_DR_REG);

        /* Copy bytes into output buffer */
        for (size_t i = 0; i < 4 && offset < rngDataSz; i++, offset++) {
            rngBuf[offset] = (uint8_t)(rnd >> (i * 8));
        }
    }

exit:
    /* Disable the RNG peripheral */
    whal_Reg_Update(base, RNG_CR_REG, RNG_CR_RNGEN_Msk,
                    whal_SetBits(RNG_CR_RNGEN_Msk, RNG_CR_RNGEN_Pos, 0));

    return err;
}

#ifndef WHAL_CFG_STM32WB_RNG_DIRECT_API_MAPPING
const whal_RngDriver whal_Stm32wb_Rng_Driver = {
    .Init = whal_Stm32wb_Rng_Init,
    .Deinit = whal_Stm32wb_Rng_Deinit,
    .Generate = whal_Stm32wb_Rng_Generate,
};
#endif /* !WHAL_CFG_STM32WB_RNG_DIRECT_API_MAPPING */
