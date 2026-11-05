/* reg.c
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

#include <wolfHAL/reg.h>

whal_Error whal_Reg32_ReadPoll(uintptr_t base, uint32_t offset,
                               uint32_t mask, uint32_t value,
                               whal_Timeout *timeout)
{
#ifdef WHAL_CFG_NO_TIMEOUT
    (void)(timeout);
#endif
    WHAL_TIMEOUT_START(timeout);
    while ((whal_Reg32_Read(base, offset) & mask) != value) {
        if (WHAL_TIMEOUT_EXPIRED(timeout)) {
            return WHAL_ETIMEOUT;
        }
    }
    return WHAL_SUCCESS;
}

#ifdef WHAL_HAVE_REG64
whal_Error whal_Reg64_ReadPoll(uintptr_t base, uint32_t offset,
                               uint64_t mask, uint64_t value,
                               whal_Timeout *timeout)
{
#ifdef WHAL_CFG_NO_TIMEOUT
    (void)(timeout);
#endif
    WHAL_TIMEOUT_START(timeout);
    while ((whal_Reg64_Read(base, offset) & mask) != value) {
        if (WHAL_TIMEOUT_EXPIRED(timeout)) {
            return WHAL_ETIMEOUT;
        }
    }
    return WHAL_SUCCESS;
}
#endif /* WHAL_HAVE_REG64 */

#ifdef WHAL_CFG_REG_LEGACY
whal_Error whal_Reg_ReadPoll(size_t base, size_t offset,
                             size_t mask, size_t value,
                             whal_Timeout *timeout)
{
    if (!WHAL_REG_FITS32(mask) || !WHAL_REG_FITS32(value)) {
        return WHAL_EINVAL;
    }
    return whal_Reg32_ReadPoll(base + offset, 0, mask & 0xFFFFFFFFu,
                               value & 0xFFFFFFFFu, timeout);
}
#endif /* WHAL_CFG_REG_LEGACY */
