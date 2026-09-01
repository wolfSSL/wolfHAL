/* stm32h5_rng.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32H5_RNG_DEV initializer */
#include <wolfHAL/rng/stm32h5_rng.h>
#include <wolfHAL/rng/rng.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

const whal_Rng whal_Stm32h5_Rng_Dev = WHAL_CFG_STM32H5_RNG_DEV;

/*
 * STM32H5 RNG Register Definitions
 *
 * The STM32H5 RNG has a 4-word output FIFO and requires a CONDRST
 * sequence to apply configuration. NIST-certified values from AN4230
 * are used for the STM32H563/573/562 family.
 */

/* Control Register */
#define RNG_CR_REG       0x00
#define RNG_CR_RNGEN_Pos 2
#define RNG_CR_RNGEN_Msk (1UL << RNG_CR_RNGEN_Pos)

#define RNG_CR_CONDRST_Pos 30
#define RNG_CR_CONDRST_Msk (1UL << RNG_CR_CONDRST_Pos)

/*
 * NIST-certified RNG_CR configuration for STM32H563/573/562 (from AN4230).
 * This value encodes CONFIG1=0x0F, CONFIG2=0x0, CONFIG3=0xE, NISTC=1,
 * CLKDIV=0, ARDIS=0, CED=0.
 */
#define RNG_CR_NIST_CFG  0x00F01E00UL

/* Status Register */
#define RNG_SR_REG      0x04
#define RNG_SR_DRDY_Pos 0
#define RNG_SR_DRDY_Msk (1UL << RNG_SR_DRDY_Pos)

#define RNG_SR_CECS_Pos 1
#define RNG_SR_CECS_Msk (1UL << RNG_SR_CECS_Pos)

#define RNG_SR_SECS_Pos 2
#define RNG_SR_SECS_Msk (1UL << RNG_SR_SECS_Pos)

/* Data Register - 32-bit random value */
#define RNG_DR_REG      0x08

/* Noise Source Control Register */
#define RNG_NSCR_REG    0x0C

/* Health Test Control Register */
#define RNG_HTCR_REG    0x10

/* NIST-certified HTCR and NSCR values for STM32H563/573/562 (from AN4230) */
#define RNG_HTCR_NIST_VAL  0x00006A91UL
#define RNG_NSCR_NIST_VAL  0x0003AF66UL

/* Magic value required to unlock HTCR writes */
#define RNG_HTCR_MAGIC     0x17590ABCUL

#ifdef WHAL_CFG_STM32H5_RNG_DIRECT_API_MAPPING
#define whal_Stm32h5_Rng_Init     whal_Rng_Init
#define whal_Stm32h5_Rng_Deinit   whal_Rng_Deinit
#define whal_Stm32h5_Rng_Generate whal_Rng_Generate
#endif /* WHAL_CFG_STM32H5_RNG_DIRECT_API_MAPPING */

whal_Error whal_Stm32h5_Rng_Init(whal_Rng *rngDev)
{
    const whal_Stm32h5_Rng_Cfg *cfg =
        (const whal_Stm32h5_Rng_Cfg *)whal_Stm32h5_Rng_Dev.cfg;
    size_t base = whal_Stm32h5_Rng_Dev.base;
    whal_Error err;
    (void)rngDev;

    /*
     * Apply NIST-certified configuration via CONDRST sequence:
     * 1. Write CONDRST=1 with configuration bits, RNGEN=0
     * 2. Write HTCR magic key then HTCR value (while CONDRST=1)
     * 3. Write NSCR value (while CONDRST=1)
     * 4. Write CONDRST=0 with RNGEN=1 to start
     */
    whal_Reg_Write(base, RNG_CR_REG,
                   RNG_CR_NIST_CFG | RNG_CR_CONDRST_Msk);

    whal_Reg_Write(base, RNG_HTCR_REG, RNG_HTCR_MAGIC);
    whal_Reg_Write(base, RNG_HTCR_REG, RNG_HTCR_NIST_VAL);
    whal_Reg_Write(base, RNG_NSCR_REG, RNG_NSCR_NIST_VAL);

    whal_Reg_Write(base, RNG_CR_REG,
                   RNG_CR_NIST_CFG | RNG_CR_RNGEN_Msk);

    /* Wait for CONDRST to clear (reset complete) */
    err = whal_Reg_ReadPoll(base, RNG_CR_REG,
                            RNG_CR_CONDRST_Msk, 0, cfg->timeout);

    return err;
}

whal_Error whal_Stm32h5_Rng_Deinit(whal_Rng *rngDev)
{
    (void)rngDev;
    /* Disable the RNG peripheral */
    whal_Reg_Update(whal_Stm32h5_Rng_Dev.base, RNG_CR_REG, RNG_CR_RNGEN_Msk,
                    whal_SetBits(RNG_CR_RNGEN_Msk, RNG_CR_RNGEN_Pos, 0));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32h5_Rng_Generate(whal_Rng *rngDev, void *rngData,
                                     size_t rngDataSz)
{
    uint8_t *rngBuf = (uint8_t *)rngData;
    whal_Error err = WHAL_SUCCESS;
    const whal_Stm32h5_Rng_Cfg *cfg =
        (const whal_Stm32h5_Rng_Cfg *)whal_Stm32h5_Rng_Dev.cfg;
    size_t base = whal_Stm32h5_Rng_Dev.base;
    size_t sr;
    size_t offset = 0;
    (void)rngDev;

    if (!rngData)
        return WHAL_EINVAL;
#ifdef WHAL_CFG_NO_TIMEOUT
    (void)(cfg);
#endif

    while (offset < rngDataSz) {
        /* Wait for a random value to be ready */
        WHAL_TIMEOUT_START(cfg->timeout);
        while (1) {
            if (WHAL_TIMEOUT_EXPIRED(cfg->timeout)) {
                err = WHAL_ETIMEOUT;
                goto exit;
            }

            sr = whal_Reg_Read(base, RNG_SR_REG);

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
        uint32_t rnd = (uint32_t)whal_Reg_Read(base, RNG_DR_REG);

        /* Copy bytes into output buffer */
        for (size_t i = 0; i < 4 && offset < rngDataSz; i++, offset++)
            rngBuf[offset] = (uint8_t)(rnd >> (i * 8));
    }

exit:
    return err;
}

#ifndef WHAL_CFG_STM32H5_RNG_DIRECT_API_MAPPING
const whal_RngDriver whal_Stm32h5_Rng_Driver = {
    .Init = whal_Stm32h5_Rng_Init,
    .Deinit = whal_Stm32h5_Rng_Deinit,
    .Generate = whal_Stm32h5_Rng_Generate,
};
#endif /* !WHAL_CFG_STM32H5_RNG_DIRECT_API_MAPPING */
