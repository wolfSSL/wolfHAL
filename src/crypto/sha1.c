/* sha1.c
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

#include <wolfHAL/crypto/crypto.h>

whal_Error whal_Sha1_Oneshot(whal_Sha1 *dev,
                             const void *in, size_t inSz,
                             void *digest, size_t digestSz)
{
    if (!dev)
        return WHAL_EINVAL;
    if (!dev->driver || !dev->driver->Oneshot)
        return WHAL_ENOTSUP;
    return dev->driver->Oneshot(dev, in, inSz, digest, digestSz);
}

whal_Error whal_Sha1_Start(whal_Sha1 *dev)
{
    if (!dev)
        return WHAL_EINVAL;
    if (!dev->driver || !dev->driver->Start)
        return WHAL_ENOTSUP;
    return dev->driver->Start(dev);
}

whal_Error whal_Sha1_Process(whal_Sha1 *dev, const void *in, size_t inSz)
{
    if (!dev)
        return WHAL_EINVAL;
    if (!dev->driver || !dev->driver->Process)
        return WHAL_ENOTSUP;
    return dev->driver->Process(dev, in, inSz);
}

whal_Error whal_Sha1_Finalize(whal_Sha1 *dev, void *digest, size_t digestSz)
{
    if (!dev)
        return WHAL_EINVAL;
    if (!dev->driver || !dev->driver->Finalize)
        return WHAL_ENOTSUP;
    return dev->driver->Finalize(dev, digest, digestSz);
}
