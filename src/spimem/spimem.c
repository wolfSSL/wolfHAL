/* spimem.c
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
#include <wolfHAL/spimem/spimem.h>
#include <wolfHAL/error.h>

inline whal_Error whal_SpiMem_Init(whal_SpiMem *spiMemDev)
{
    if (!spiMemDev)
        return WHAL_EINVAL;
    if (!spiMemDev->driver || !spiMemDev->driver->Init)
        return WHAL_ENOTSUP;

    return spiMemDev->driver->Init(spiMemDev);
}

inline whal_Error whal_SpiMem_Deinit(whal_SpiMem *spiMemDev)
{
    if (!spiMemDev)
        return WHAL_EINVAL;
    if (!spiMemDev->driver || !spiMemDev->driver->Deinit)
        return WHAL_ENOTSUP;

    return spiMemDev->driver->Deinit(spiMemDev);
}

inline whal_Error whal_SpiMem_Exec(whal_SpiMem *spiMemDev,
                                   const whal_SpiMemOp *op)
{
    if (!spiMemDev || !op)
        return WHAL_EINVAL;
    if (op->dataSz > 0 && !op->data)
        return WHAL_EINVAL;
    if (!spiMemDev->driver || !spiMemDev->driver->Exec)
        return WHAL_ENOTSUP;

    return spiMemDev->driver->Exec(spiMemDev, op);
}

inline whal_Error whal_SpiMem_MaxDataSz(whal_SpiMem *spiMemDev,
                                        const whal_SpiMemOp *op,
                                        size_t *maxSz)
{
    if (!spiMemDev || !op || !maxSz)
        return WHAL_EINVAL;
    if (!spiMemDev->driver || !spiMemDev->driver->MaxDataSz)
        return WHAL_ENOTSUP;

    return spiMemDev->driver->MaxDataSz(spiMemDev, op, maxSz);
}
