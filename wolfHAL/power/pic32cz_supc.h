/* pic32cz_supc.h
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

#ifndef WHAL_PIC32CZ_SUPC_H
#define WHAL_PIC32CZ_SUPC_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/*
 * @file pic32cz_supc.h
 * @brief PIC32CZ supply controller (SUPC) driver.
 *
 * Boards toggle SUPC voltage-regulator outputs imperatively from Board_Init
 * by calling the helpers below. Power is a board-level driver — no whal_Power
 * device struct, no generic API, no vtable. The base address is the chip's
 * fixed SUPC location (WHAL_PIC32CZ_SUPC_BASE).
 */

#define WHAL_PIC32CZ_SUPC_BASE                  0x44020000

#define WHAL_PIC32CZ_SUPC_VREGCTRL_REG          0x1C
#define WHAL_PIC32CZ_SUPC_VREGCTRL_AVREGEN_Pos  16
#define WHAL_PIC32CZ_SUPC_VREGCTRL_AVREGEN_Msk  (WHAL_BITMASK(3) << WHAL_PIC32CZ_SUPC_VREGCTRL_AVREGEN_Pos)

/*
 * @brief Descriptor for one SUPC voltage-regulator output (mask + bit position
 *        within VREGCTRL.AVREGEN).
 */
typedef struct whal_Pic32cz_Supc_Supply {
    size_t enableMask;
    size_t enablePos;
} whal_Pic32cz_Supc_Supply;

/*
 * @brief Set a regulator's enable bit in SUPC VREGCTRL.AVREGEN.
 *
 * @param supply Descriptor for the regulator output to enable.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Pic32cz_Supc_EnableSupply(
    const whal_Pic32cz_Supc_Supply *supply)
{
    whal_Reg32_Update(WHAL_PIC32CZ_SUPC_BASE, WHAL_PIC32CZ_SUPC_VREGCTRL_REG,
                      supply->enableMask,
                      whal_SetBits(supply->enableMask, supply->enablePos, 1));
    return WHAL_SUCCESS;
}

/*
 * @brief Clear a regulator's enable bit in SUPC VREGCTRL.AVREGEN.
 *
 * @param supply Descriptor for the regulator output to disable.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Pic32cz_Supc_DisableSupply(
    const whal_Pic32cz_Supc_Supply *supply)
{
    whal_Reg32_Update(WHAL_PIC32CZ_SUPC_BASE, WHAL_PIC32CZ_SUPC_VREGCTRL_REG,
                      supply->enableMask,
                      whal_SetBits(supply->enableMask, supply->enablePos, 0));
    return WHAL_SUCCESS;
}

#endif /* WHAL_PIC32CZ_SUPC_H */
