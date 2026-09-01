/* stm32wba_rng.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32WBA_RNG_DEV initializer */
#include <wolfHAL/rng/stm32wba_rng.h>
#include <wolfHAL/rng/rng.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

const whal_Rng whal_Stm32wba_Rng_Dev = WHAL_CFG_STM32WBA_RNG_DEV;

/*
 * STM32WBA RNG Register Definitions (RM0493 section 24.7)
 */

/* Control Register (offset 0x000, reset 0x0080 0D00) */
#define RNG_CR_REG            0x00
#define RNG_CR_RNGEN_Pos      2
#define RNG_CR_RNGEN_Msk      (1UL << RNG_CR_RNGEN_Pos)
#define RNG_CR_IE_Pos         3
#define RNG_CR_IE_Msk         (1UL << RNG_CR_IE_Pos)
#define RNG_CR_CED_Pos        5
#define RNG_CR_CED_Msk        (1UL << RNG_CR_CED_Pos)
#define RNG_CR_ARDIS_Pos      7
#define RNG_CR_ARDIS_Msk      (1UL << RNG_CR_ARDIS_Pos)
#define RNG_CR_RNG_CONFIG3_Pos 8
#define RNG_CR_RNG_CONFIG3_Msk (0xFUL << RNG_CR_RNG_CONFIG3_Pos)
#define RNG_CR_NISTC_Pos      12
#define RNG_CR_NISTC_Msk      (1UL << RNG_CR_NISTC_Pos)
#define RNG_CR_RNG_CONFIG2_Pos 13
#define RNG_CR_RNG_CONFIG2_Msk (7UL << RNG_CR_RNG_CONFIG2_Pos)
#define RNG_CR_CLKDIV_Pos     16
#define RNG_CR_CLKDIV_Msk     (0xFUL << RNG_CR_CLKDIV_Pos)
#define RNG_CR_RNG_CONFIG1_Pos 20
#define RNG_CR_RNG_CONFIG1_Msk (0x3FUL << RNG_CR_RNG_CONFIG1_Pos)
#define RNG_CR_CONDRST_Pos    30
#define RNG_CR_CONDRST_Msk    (1UL << RNG_CR_CONDRST_Pos)
#define RNG_CR_CONFIGLOCK_Pos 31
#define RNG_CR_CONFIGLOCK_Msk (1UL << RNG_CR_CONFIGLOCK_Pos)

/* Status Register (offset 0x004) */
#define RNG_SR_REG            0x04
#define RNG_SR_DRDY_Pos       0
#define RNG_SR_DRDY_Msk       (1UL << RNG_SR_DRDY_Pos)
#define RNG_SR_CECS_Pos       1
#define RNG_SR_CECS_Msk       (1UL << RNG_SR_CECS_Pos)
#define RNG_SR_SECS_Pos       2
#define RNG_SR_SECS_Msk       (1UL << RNG_SR_SECS_Pos)
#define RNG_SR_CEIS_Pos       5
#define RNG_SR_CEIS_Msk       (1UL << RNG_SR_CEIS_Pos)
#define RNG_SR_SEIS_Pos       6
#define RNG_SR_SEIS_Msk       (1UL << RNG_SR_SEIS_Pos)

/* Data Register (offset 0x008) */
#define RNG_DR_REG            0x08

/*
 * Configuration C values (RM0493 Table 178):
 *   NISTC=0, RNG_CONFIG1=0x0F, CLKDIV=0x0, RNG_CONFIG2=0x0,
 *   RNG_CONFIG3=0xD, CED=0, N=2
 */
#define RNG_CR_CONFIG_C  (whal_SetBits(RNG_CR_RNG_CONFIG1_Msk, RNG_CR_RNG_CONFIG1_Pos, 0x0F) | \
                          whal_SetBits(RNG_CR_RNG_CONFIG3_Msk, RNG_CR_RNG_CONFIG3_Pos, 0x0D))

#ifdef WHAL_CFG_STM32WBA_RNG_DIRECT_API_MAPPING
#define whal_Stm32wba_Rng_Init     whal_Rng_Init
#define whal_Stm32wba_Rng_Deinit   whal_Rng_Deinit
#define whal_Stm32wba_Rng_Generate whal_Rng_Generate
#endif

whal_Error whal_Stm32wba_Rng_Init(whal_Rng *rngDev)
{
    const whal_Stm32wba_Rng_Cfg *cfg =
        (const whal_Stm32wba_Rng_Cfg *)whal_Stm32wba_Rng_Dev.cfg;
    size_t base = whal_Stm32wba_Rng_Dev.base;
    (void)rngDev;

    /* Apply Configuration C with CONDRST=1 and RNGEN=1 */
    whal_Reg_Write(base, RNG_CR_REG,
                   RNG_CR_CONDRST_Msk | RNG_CR_CONFIG_C | RNG_CR_RNGEN_Msk);

    /* Clear CONDRST to start conditioning */
    whal_Reg_Write(base, RNG_CR_REG,
                   RNG_CR_CONFIG_C | RNG_CR_RNGEN_Msk);

    /* Wait for CONDRST to self-clear */
    return whal_Reg_ReadPoll(base, RNG_CR_REG, RNG_CR_CONDRST_Msk, 0,
                             cfg->timeout);
}

whal_Error whal_Stm32wba_Rng_Deinit(whal_Rng *rngDev)
{
    (void)rngDev;

    /* Disable RNG */
    whal_Reg_Update(whal_Stm32wba_Rng_Dev.base, RNG_CR_REG, RNG_CR_RNGEN_Msk, 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Rng_Generate(whal_Rng *rngDev, void *rngData, size_t rngDataSz)
{
    uint8_t *rngBuf = (uint8_t *)rngData;
    whal_Error err = WHAL_SUCCESS;
    const whal_Stm32wba_Rng_Cfg *cfg =
        (const whal_Stm32wba_Rng_Cfg *)whal_Stm32wba_Rng_Dev.cfg;
    size_t base = whal_Stm32wba_Rng_Dev.base;
    size_t sr;
    size_t offset = 0;
    (void)rngDev;

    if (!rngData)
        return WHAL_EINVAL;
#ifdef WHAL_CFG_NO_TIMEOUT
    (void)(cfg);
#endif

    while (offset < rngDataSz) {
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

        uint32_t rnd = *(volatile uint32_t *)(base + RNG_DR_REG);

        for (size_t i = 0; i < 4 && offset < rngDataSz; i++, offset++)
            rngBuf[offset] = (uint8_t)(rnd >> (i * 8));
    }

exit:
    return err;
}

#ifndef WHAL_CFG_STM32WBA_RNG_DIRECT_API_MAPPING
const whal_RngDriver whal_Stm32wba_Rng_Driver = {
    .Init = whal_Stm32wba_Rng_Init,
    .Deinit = whal_Stm32wba_Rng_Deinit,
    .Generate = whal_Stm32wba_Rng_Generate,
};
#endif
