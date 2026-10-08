/* aes_cbc.c
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

whal_Error whal_AesCbc_Oneshot(whal_AesCbc *dev, whal_Crypto_Dir dir,
                               const void *key, size_t keySz,
                               const void *iv,
                               const void *in, void *out, size_t sz)
{
    if (!dev)
        return WHAL_EINVAL;
    if (!dev->driver || !dev->driver->Oneshot)
        return WHAL_ENOTSUP;
    return dev->driver->Oneshot(dev, dir, key, keySz, iv, in, out, sz);
}

whal_Error whal_AesCbc_Start(whal_AesCbc *dev, whal_Crypto_Dir dir,
                             const void *key, size_t keySz,
                             const void *iv)
{
    if (!dev)
        return WHAL_EINVAL;
    if (!dev->driver || !dev->driver->Start)
        return WHAL_ENOTSUP;
    return dev->driver->Start(dev, dir, key, keySz, iv);
}

whal_Error whal_AesCbc_Process(whal_AesCbc *dev,
                               const void *in, void *out, size_t sz)
{
    if (!dev)
        return WHAL_EINVAL;
    if (!dev->driver || !dev->driver->Process)
        return WHAL_ENOTSUP;
    return dev->driver->Process(dev, in, out, sz);
}
