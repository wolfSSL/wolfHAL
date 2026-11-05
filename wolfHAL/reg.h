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
#include <stdint.h>
#include <wolfHAL/error.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

/*
 * @file reg.h
 * @brief Memory-mapped register access with an explicit width.
 *
 * whal_Reg32_* access 32-bit registers, whal_Reg64_* 64-bit registers.
 * The older whal_Reg_* API needs WHAL_CFG_REG_LEGACY.
 */

/* 64-bit registers: only where a 64-bit access is a single access */
#if UINTPTR_MAX > 0xFFFFFFFFu
#define WHAL_HAVE_REG64
#endif

/*
 * @brief Write a 32-bit register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 * @param value  Value to write.
 */
static inline void whal_Reg32_Write(uintptr_t base, uint32_t offset,
                                    uint32_t value)
{
    *(volatile uint32_t *)(base + offset) = value;
}

/*
 * @brief Read a 32-bit register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 *
 * @return The register value.
 */
static inline uint32_t whal_Reg32_Read(uintptr_t base, uint32_t offset)
{
    return *(volatile uint32_t *)(base + offset);
}

/*
 * @brief Read-modify-write the bits in @p mask of a 32-bit register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 * @param mask   Bits to change.
 * @param value  New bits, already in position; bits outside @p mask are
 *               ignored.
 */
static inline void whal_Reg32_Update(uintptr_t base, uint32_t offset,
                                     uint32_t mask, uint32_t value)
{
    volatile uint32_t *reg = (volatile uint32_t *)(base + offset);
    *reg = (*reg & ~mask) | (value & mask);
}

/*
 * @brief Read a field of a 32-bit register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 * @param mask   Bits of the field.
 * @param pos    Bit position of the field's LSB, below 32.
 * @param value  Output for the field, shifted down to bit 0.
 */
static inline void whal_Reg32_Get(uintptr_t base, uint32_t offset,
                                  uint32_t mask, uint32_t pos,
                                  uint32_t *value)
{
    *value = whal_GetBits(mask, pos, whal_Reg32_Read(base, offset));
}

/*
 * @brief Poll a 32-bit register until (reg & mask) == value, or timeout.
 *
 * @param base    Base address of the register block.
 * @param offset  Byte offset from @p base to the register.
 * @param mask    Bits to compare.
 * @param value   Expected bits. Must fit in @p mask, or the poll never
 *                matches.
 * @param timeout Timeout instance (NULL for unbounded wait).
 *
 * @retval WHAL_SUCCESS  The masked bits matched @p value.
 * @retval WHAL_ETIMEOUT The timeout expired first.
 */
whal_Error whal_Reg32_ReadPoll(uintptr_t base, uint32_t offset,
                               uint32_t mask, uint32_t value,
                               whal_Timeout *timeout);

#ifdef WHAL_HAVE_REG64

/*
 * @brief Write a 64-bit register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 * @param value  Value to write.
 */
static inline void whal_Reg64_Write(uintptr_t base, uint32_t offset,
                                    uint64_t value)
{
    *(volatile uint64_t *)(base + offset) = value;
}

/*
 * @brief Read a 64-bit register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 *
 * @return The register value.
 */
static inline uint64_t whal_Reg64_Read(uintptr_t base, uint32_t offset)
{
    return *(volatile uint64_t *)(base + offset);
}

/*
 * @brief Read-modify-write the bits in @p mask of a 64-bit register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 * @param mask   Bits to change.
 * @param value  New bits, already in position; bits outside @p mask are
 *               ignored.
 */
static inline void whal_Reg64_Update(uintptr_t base, uint32_t offset,
                                     uint64_t mask, uint64_t value)
{
    volatile uint64_t *reg = (volatile uint64_t *)(base + offset);
    *reg = (*reg & ~mask) | (value & mask);
}

/*
 * @brief Read a field of a 64-bit register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 * @param mask   Bits of the field.
 * @param pos    Bit position of the field's LSB, below 64.
 * @param value  Output for the field, shifted down to bit 0.
 */
static inline void whal_Reg64_Get(uintptr_t base, uint32_t offset,
                                  uint64_t mask, uint32_t pos,
                                  uint64_t *value)
{
    *value = whal_GetBits(mask, pos, whal_Reg64_Read(base, offset));
}

/*
 * @brief Poll a 64-bit register until (reg & mask) == value, or timeout.
 *
 * @param base    Base address of the register block.
 * @param offset  Byte offset from @p base to the register.
 * @param mask    Bits to compare.
 * @param value   Expected bits. Must fit in @p mask, or the poll never
 *                matches.
 * @param timeout Timeout instance (NULL for unbounded wait).
 *
 * @retval WHAL_SUCCESS  The masked bits matched @p value.
 * @retval WHAL_ETIMEOUT The timeout expired first.
 */
whal_Error whal_Reg64_ReadPoll(uintptr_t base, uint32_t offset,
                               uint64_t mask, uint64_t value,
                               whal_Timeout *timeout);

#endif /* WHAL_HAVE_REG64 */

#ifdef WHAL_CFG_REG_LEGACY

/* Legacy whal_Reg_* API: size_t arguments, 32-bit accesses. Where size_t
 * is wider, values and masks above 32 bits return WHAL_EINVAL. */
#if SIZE_MAX > 0xFFFFFFFFu
#define WHAL_REG_FITS32(x) ((x) <= 0xFFFFFFFFu)
#else
#define WHAL_REG_FITS32(x) (1)
#endif

/*
 * @brief Legacy: write a 32-bit register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 * @param value  Value to write; must fit in 32 bits.
 *
 * @retval WHAL_SUCCESS Written.
 * @retval WHAL_EINVAL  @p value does not fit; nothing written.
 */
static inline whal_Error whal_Reg_Write(size_t base, size_t offset,
                                        size_t value)
{
    if (!WHAL_REG_FITS32(value)) {
        return WHAL_EINVAL;
    }
    whal_Reg32_Write(base + offset, 0, value & 0xFFFFFFFFu);
    return WHAL_SUCCESS;
}

/*
 * @brief Legacy: read a 32-bit register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 *
 * @return The register value.
 */
static inline size_t whal_Reg_Read(size_t base, size_t offset)
{
    return whal_Reg32_Read(base + offset, 0);
}

/*
 * @brief Legacy: read-modify-write the bits in @p mask of a 32-bit register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 * @param mask   Bits to change; must fit in 32 bits.
 * @param value  New bits, already in position; must fit in 32 bits.
 *
 * @retval WHAL_SUCCESS Updated.
 * @retval WHAL_EINVAL  @p mask or @p value does not fit; nothing written.
 */
static inline whal_Error whal_Reg_Update(size_t base, size_t offset,
                                         size_t mask, size_t value)
{
    if (!WHAL_REG_FITS32(mask) || !WHAL_REG_FITS32(value)) {
        return WHAL_EINVAL;
    }
    whal_Reg32_Update(base + offset, 0, mask & 0xFFFFFFFFu,
                      value & 0xFFFFFFFFu);
    return WHAL_SUCCESS;
}

/*
 * @brief Legacy: read a field of a 32-bit register.
 *
 * @param base   Base address of the register block.
 * @param offset Byte offset from @p base to the register.
 * @param msk    Bits of the field; must fit in 32 bits.
 * @param pos    Bit position of the field's LSB.
 * @param value  Output for the field; 0 on error.
 *
 * @retval WHAL_SUCCESS Read.
 * @retval WHAL_EINVAL  @p msk does not fit.
 */
static inline whal_Error whal_Reg_Get(size_t base, size_t offset, size_t msk,
                                      size_t pos, size_t *value)
{
    size_t reg;

    if (!WHAL_REG_FITS32(msk)) {
        *value = 0;
        return WHAL_EINVAL;
    }
    reg = whal_Reg32_Read(base + offset, 0);
    *value = whal_GetBits(msk, pos, reg);
    return WHAL_SUCCESS;
}

/*
 * @brief Legacy: poll a 32-bit register until (reg & mask) == value.
 *
 * @param base    Base address of the register block.
 * @param offset  Byte offset from @p base to the register.
 * @param mask    Bits to compare; must fit in 32 bits.
 * @param value   Expected bits; must fit in 32 bits and in @p mask.
 * @param timeout Timeout instance (NULL for unbounded wait).
 *
 * @retval WHAL_SUCCESS  The masked bits matched @p value.
 * @retval WHAL_ETIMEOUT The timeout expired first.
 * @retval WHAL_EINVAL   @p mask or @p value does not fit.
 */
whal_Error whal_Reg_ReadPoll(size_t base, size_t offset,
                             size_t mask, size_t value,
                             whal_Timeout *timeout);

#endif /* WHAL_CFG_REG_LEGACY */

#endif /* WHAL_REG_H */
