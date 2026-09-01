/* stm32wb0_rng.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32WB0_RNG_DEV initializer */
#include <wolfHAL/rng/stm32wb0_rng.h>
#include <wolfHAL/rng/rng.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

const whal_Rng whal_Stm32wb0_Rng_Dev = WHAL_CFG_STM32WB0_RNG_DEV;

/*
 * STM32WB0 RNG register layout (RM0529 section 14.2). All accesses must
 * be 32-bit wide — narrower accesses raise an AHB error and hard fault.
 */

/* Control register */
#define RNG_CR_REG          0x00
#define RNG_CR_RNG_DIS_Pos  2
#define RNG_CR_RNG_DIS_Msk  (1UL << RNG_CR_RNG_DIS_Pos)
#define RNG_CR_TST_CLK_Pos  3
#define RNG_CR_TST_CLK_Msk  (1UL << RNG_CR_TST_CLK_Pos)

/* Status register */
#define RNG_SR_REG          0x04
#define RNG_SR_RNGRDY_Pos   0
#define RNG_SR_RNGRDY_Msk   (1UL << RNG_SR_RNGRDY_Pos)
#define RNG_SR_REVCLK_Pos   1
#define RNG_SR_REVCLK_Msk   (1UL << RNG_SR_REVCLK_Pos)
#define RNG_SR_FAULT_Pos    2
#define RNG_SR_FAULT_Msk    (1UL << RNG_SR_FAULT_Pos)

/* Value register — 16-bit random sample in bits [15:0]. */
#define RNG_VAL_REG         0x08
#define RNG_VAL_Msk         0xFFFFu

#ifdef WHAL_CFG_STM32WB0_RNG_DIRECT_API_MAPPING
#define whal_Stm32wb0_Rng_Init     whal_Rng_Init
#define whal_Stm32wb0_Rng_Deinit   whal_Rng_Deinit
#define whal_Stm32wb0_Rng_Generate whal_Rng_Generate
#endif /* WHAL_CFG_STM32WB0_RNG_DIRECT_API_MAPPING */

whal_Error whal_Stm32wb0_Rng_Init(whal_Rng *rngDev)
{
    size_t base = whal_Stm32wb0_Rng_Dev.base;
    (void)rngDev;

    /* Full CR write: clears RNG_DIS to enable the oscillators and wipes
     * any stale state the bootROM may have left in reserved bits.
     * Matches ST's HAL/LL pattern. The TST_CLK / REVCLK probe is the
     * optional clock-error detection feature, not a required init step. */
    whal_Reg_Write(base, RNG_CR_REG, 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb0_Rng_Deinit(whal_Rng *rngDev)
{
    size_t base = whal_Stm32wb0_Rng_Dev.base;
    (void)rngDev;

    /* Disable: set RNG_DIS to power down the internal free-running
     * oscillators. */
    whal_Reg_Update(base, RNG_CR_REG, RNG_CR_RNG_DIS_Msk,
                    whal_SetBits(RNG_CR_RNG_DIS_Msk, RNG_CR_RNG_DIS_Pos, 1));
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb0_Rng_Generate(whal_Rng *rngDev, void *rngData,
                                      size_t rngDataSz)
{
    uint8_t *rngBuf = (uint8_t *)rngData;
    size_t base = whal_Stm32wb0_Rng_Dev.base;
    size_t offset = 0;
    (void)rngDev;

    if (!rngData)
        return WHAL_EINVAL;

    while (offset < rngDataSz) {

        /* 32-bit-only read, low half is the random value. */
        uint32_t word = whal_Reg_Read(base, RNG_VAL_REG);
        uint16_t rnd  = (uint16_t)(word & RNG_VAL_Msk);

        for (size_t i = 0; i < 2 && offset < rngDataSz; i++, offset++) {
            rngBuf[offset] = (uint8_t)(rnd >> (i * 8));
        }
    }

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_STM32WB0_RNG_DIRECT_API_MAPPING
const whal_RngDriver whal_Stm32wb0_Rng_Driver = {
    .Init     = whal_Stm32wb0_Rng_Init,
    .Deinit   = whal_Stm32wb0_Rng_Deinit,
    .Generate = whal_Stm32wb0_Rng_Generate,
};
#endif /* !WHAL_CFG_STM32WB0_RNG_DIRECT_API_MAPPING */
