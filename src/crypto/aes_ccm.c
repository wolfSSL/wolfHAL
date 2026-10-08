/* aes_ccm.c
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

whal_Error whal_AesCcm_Oneshot(whal_AesCcm *dev, whal_Crypto_Dir dir,
                               const void *key, size_t keySz,
                               const void *nonce, size_t nonceSz,
                               const void *aad, size_t aadSz,
                               const void *in, void *out, size_t sz,
                               void *tag, size_t tagSz)
{
    if (!dev)
        return WHAL_EINVAL;
    if (!dev->driver || !dev->driver->Oneshot)
        return WHAL_ENOTSUP;
    return dev->driver->Oneshot(dev, dir, key, keySz, nonce, nonceSz,
                                aad, aadSz, in, out, sz, tag, tagSz);
}

whal_Error whal_AesCcm_Start(whal_AesCcm *dev, whal_Crypto_Dir dir,
                             const void *key, size_t keySz,
                             const void *nonce, size_t nonceSz,
                             const void *aad, size_t aadSz,
                             size_t tagSz, size_t sz)
{
    if (!dev)
        return WHAL_EINVAL;
    if (!dev->driver || !dev->driver->Start)
        return WHAL_ENOTSUP;
    return dev->driver->Start(dev, dir, key, keySz, nonce, nonceSz,
                              aad, aadSz, tagSz, sz);
}

whal_Error whal_AesCcm_Process(whal_AesCcm *dev,
                               const void *in, void *out, size_t sz)
{
    if (!dev)
        return WHAL_EINVAL;
    if (!dev->driver || !dev->driver->Process)
        return WHAL_ENOTSUP;
    return dev->driver->Process(dev, in, out, sz);
}

whal_Error whal_AesCcm_Finalize(whal_AesCcm *dev,
                                void *tag, size_t tagSz)
{
    if (!dev)
        return WHAL_EINVAL;
    if (!dev->driver || !dev->driver->Finalize)
        return WHAL_ENOTSUP;
    return dev->driver->Finalize(dev, tag, tagSz);
}
