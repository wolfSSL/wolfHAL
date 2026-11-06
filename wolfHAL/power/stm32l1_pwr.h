/* stm32l1_pwr.h
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

#ifndef WHAL_STM32L1_PWR_H
#define WHAL_STM32L1_PWR_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32l1_pwr.h
 * @brief STM32L1 PWR (power control) driver.
 *
 * Boards bring up the regulator imperatively from Board_Init by calling
 * the helpers below. Power is a board-level driver — there is no whal_Power
 * device struct, no generic API, no vtable. The base address is the chip's
 * fixed PWR location (WHAL_STM32L1_PWR_BASE).
 *
 * The internal voltage regulator output range (PWR_CR.VOS) bounds the
 * maximum permitted SYSCLK and PLL VCO frequencies. Must be configured
 * before bringing the PLL above the reset-default limits.
 *
 *   Range 1 (1.8 V): SYSCLK <= 32 MHz, PLL VCO <= 96 MHz.
 *   Range 2 (1.5 V): SYSCLK <= 16 MHz, PLL VCO <= 48 MHz. Reset default.
 *   Range 3 (1.2 V): SYSCLK <=  4 MHz, PLL disabled.
 */

#define WHAL_STM32L1_PWR_BASE         0x40007000

#define WHAL_STM32L1_PWR_CR_REG       0x00
#define WHAL_STM32L1_PWR_CR_VOS_Pos   11
#define WHAL_STM32L1_PWR_CR_VOS_Msk   (WHAL_BITMASK(2) << WHAL_STM32L1_PWR_CR_VOS_Pos)
#define WHAL_STM32L1_PWR_CSR_REG      0x04
#define WHAL_STM32L1_PWR_CSR_VOSF_Pos 4
#define WHAL_STM32L1_PWR_CSR_VOSF_Msk (1UL << WHAL_STM32L1_PWR_CSR_VOSF_Pos)

/*
 * @brief Internal regulator voltage scaling range (PWR_CR.VOS).
 */
typedef enum {
    WHAL_STM32L1_PWR_VOS_RANGE_1 = 1,
    WHAL_STM32L1_PWR_VOS_RANGE_2 = 2,
    WHAL_STM32L1_PWR_VOS_RANGE_3 = 3,
} whal_Stm32l1_Pwr_VosRange;

/*
 * @brief Switch the regulator to a new VOS range.
 *
 * Waits for any in-progress VOS transition to finish (PWR_CSR.VOSF == 0),
 * writes the new range to PWR_CR.VOS, then waits for VOSF == 0 again. Must
 * be called before raising the system clock past the current range's limit.
 *
 * @param range    Target VOS range.
 * @param timeout  Timeout instance (NULL for unbounded wait).
 *
 * @retval WHAL_SUCCESS  Range switched and regulator settled.
 * @retval WHAL_ETIMEOUT VOSF didn't clear within the timeout.
 */
static inline whal_Error whal_Stm32l1_Pwr_SetVosRange(
    whal_Stm32l1_Pwr_VosRange range, whal_Timeout *timeout)
{
    whal_Error err;

    err = whal_Reg32_ReadPoll(WHAL_STM32L1_PWR_BASE, WHAL_STM32L1_PWR_CSR_REG,
                              WHAL_STM32L1_PWR_CSR_VOSF_Msk, 0, timeout);
    if (err)
        return err;

    whal_Reg32_Update(WHAL_STM32L1_PWR_BASE, WHAL_STM32L1_PWR_CR_REG,
                      WHAL_STM32L1_PWR_CR_VOS_Msk,
                      whal_SetBits(WHAL_STM32L1_PWR_CR_VOS_Msk,
                                   WHAL_STM32L1_PWR_CR_VOS_Pos, range));

    return whal_Reg32_ReadPoll(WHAL_STM32L1_PWR_BASE, WHAL_STM32L1_PWR_CSR_REG,
                               WHAL_STM32L1_PWR_CSR_VOSF_Msk, 0, timeout);
}

#endif /* WHAL_STM32L1_PWR_H */
