/* dma.c
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

#include <wolfHAL/dma/dma.h>

inline whal_Error whal_Dma_Init(whal_Dma *dmaDev)
{
    if (!dmaDev || !dmaDev->driver || !dmaDev->driver->Init) {
        return WHAL_EINVAL;
    }

    return dmaDev->driver->Init(dmaDev);
}

inline whal_Error whal_Dma_Deinit(whal_Dma *dmaDev)
{
    if (!dmaDev || !dmaDev->driver || !dmaDev->driver->Deinit) {
        return WHAL_EINVAL;
    }

    return dmaDev->driver->Deinit(dmaDev);
}

inline whal_Error whal_Dma_Configure(whal_Dma *dmaDev, size_t ch,
                                     const void *chCfg)
{
    if (!dmaDev || !dmaDev->driver || !dmaDev->driver->Configure || !chCfg) {
        return WHAL_EINVAL;
    }

    return dmaDev->driver->Configure(dmaDev, ch, chCfg);
}

inline whal_Error whal_Dma_Start(whal_Dma *dmaDev, size_t ch)
{
    if (!dmaDev || !dmaDev->driver || !dmaDev->driver->Start) {
        return WHAL_EINVAL;
    }

    return dmaDev->driver->Start(dmaDev, ch);
}

inline whal_Error whal_Dma_Stop(whal_Dma *dmaDev, size_t ch)
{
    if (!dmaDev || !dmaDev->driver || !dmaDev->driver->Stop) {
        return WHAL_EINVAL;
    }

    return dmaDev->driver->Stop(dmaDev, ch);
}

inline whal_Error whal_Dma_Wait(whal_Dma *dmaDev, size_t ch)
{
    if (!dmaDev) {
        return WHAL_EINVAL;
    }
    if (!dmaDev->driver || !dmaDev->driver->Wait) {
        return WHAL_ENOTSUP;
    }
    return dmaDev->driver->Wait(dmaDev, ch);
}
