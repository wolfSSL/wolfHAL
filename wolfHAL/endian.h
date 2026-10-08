/* endian.h
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

#ifndef WHAL_ENDIAN_H
#define WHAL_ENDIAN_H

/*
 * @file endian.h
 * @brief Byte-order conversion helpers.
 */

#include <stdint.h>
#include <stddef.h>

/*
 * @brief Load a 32-bit value from a big-endian byte array.
 */
static inline uint32_t whal_LoadBe32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | p[3];
}

/*
 * @brief Load a 32-bit value from a little-endian byte array.
 */
static inline uint32_t whal_LoadLe32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/*
 * @brief Store a 32-bit value into a big-endian byte array.
 */
static inline void whal_StoreBe32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/*
 * @brief Load n bytes from a byte array into a 32-bit big-endian value.
 *
 * The first byte becomes the MSB and the remaining (4-n) bytes are zero.
 * Equivalent to whal_LoadBe32 when n == 4.
 *
 * @param p Source byte array.
 * @param n Number of bytes to load (must be 0..4).
 */
static inline uint32_t whal_LoadBe32Partial(const uint8_t *p, size_t n)
{
    uint32_t v = 0;
    size_t i;
    for (i = 0; i < n; i++)
        v |= (uint32_t)p[i] << (24 - i * 8);
    return v;
}

/*
 * @brief Load n bytes from a byte array into a 32-bit little-endian value.
 *
 * The first byte becomes the LSB and the remaining (4-n) bytes are zero.
 *
 * @param p Source byte array.
 * @param n Number of bytes to load (must be 0..4).
 */
static inline uint32_t whal_LoadLe32Partial(const uint8_t *p, size_t n)
{
    uint32_t v = 0;
    size_t i;
    for (i = 0; i < n; i++)
        v |= (uint32_t)p[i] << (i * 8);
    return v;
}

#endif /* WHAL_ENDIAN_H */
