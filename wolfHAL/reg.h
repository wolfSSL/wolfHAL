/* reg.h
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

#ifndef WHAL_REG_H
#define WHAL_REG_H

#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

/*
 * @file reg.h
 * @brief Helpers for accessing and manipulating memory-mapped registers.
 */

/*
 * @brief Update a masked field within a memory-mapped register.
 *
 * The function applies a read-modify-write using @p mask to only touch the
 * desired bit field.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 * @param mask   Bit mask selecting the field to update.
 * @param value  Value already positioned within @p mask; it is masked but
 *               not shifted.
 *
 * @note No return value. Callers are responsible for passing valid inputs.
 */
static inline void whal_Reg_Update(size_t base, size_t offset, size_t mask,
                                   size_t value)
{
    volatile size_t *reg = (size_t *)(base + offset);
    *reg = (*reg & ~mask) | (value & mask);
}

/*
 * @brief Read a masked field from a memory-mapped register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 * @param msk    Bit mask selecting the field to extract.
 * @param pos    Bit position of the field's LSB.
 * @param value  Output storage for the decoded field.
 *
 * @note No return value. Callers are responsible for passing valid inputs.
 */
static inline void whal_Reg_Get(size_t base, size_t offset, size_t msk,
                                size_t pos, size_t *value)
{
    size_t val = *(volatile size_t *)(base + offset);
    *value = whal_GetBits(msk, pos, val);
}

/*
 * @brief Write a 32-bit value to a memory-mapped register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 * @param value  Value to write.
 */
static inline void whal_Reg_Write(size_t base, size_t offset, size_t value)
{
    *(volatile size_t *)(base + offset) = value;
}

/*
 * @brief Read a 32-bit value from a memory-mapped register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 *
 * @return The register value.
 */
static inline size_t whal_Reg_Read(size_t base, size_t offset)
{
    return *(volatile size_t *)(base + offset);
}

/*
 * @brief Poll a register until (reg & mask) == value, or timeout.
 *
 * @param base    Register block base address.
 * @param offset  Register offset.
 * @param mask    Bit mask to extract the field.
 * @param value   Expected value of the masked field.
 * @param timeout Timeout instance (NULL for unbounded wait).
 */
whal_Error whal_Reg_ReadPoll(size_t base, size_t offset,
                             size_t mask, size_t value,
                             whal_Timeout *timeout);

#endif /* WHAL_REG_H */
