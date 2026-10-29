/* ieee802154.c
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

#include <wolfHAL/ieee802154/ieee802154.h>
#include <wolfHAL/error.h>

whal_Error whal_Ieee802154_Init(whal_Ieee802154 *radioDev)
{
    if (!radioDev || !radioDev->driver || !radioDev->driver->Init)
        return WHAL_EINVAL;
    return radioDev->driver->Init(radioDev);
}

whal_Error whal_Ieee802154_Deinit(whal_Ieee802154 *radioDev)
{
    if (!radioDev || !radioDev->driver || !radioDev->driver->Deinit)
        return WHAL_EINVAL;
    return radioDev->driver->Deinit(radioDev);
}

whal_Error whal_Ieee802154_GetCaps(whal_Ieee802154 *radioDev, uint32_t *caps)
{
    if (!radioDev || !radioDev->driver || !radioDev->driver->GetCaps || !caps)
        return WHAL_EINVAL;
    return radioDev->driver->GetCaps(radioDev, caps);
}

whal_Error whal_Ieee802154_SetAttr(whal_Ieee802154 *radioDev,
                                   whal_Ieee802154_Attr attr,
                                   const whal_Ieee802154_AttrVal *val)
{
    if (!radioDev || !radioDev->driver || !radioDev->driver->SetAttr || !val)
        return WHAL_EINVAL;
    return radioDev->driver->SetAttr(radioDev, attr, val);
}

whal_Error whal_Ieee802154_GetAttr(whal_Ieee802154 *radioDev,
                                   whal_Ieee802154_Attr attr,
                                   whal_Ieee802154_AttrVal *val)
{
    if (!radioDev || !radioDev->driver || !radioDev->driver->GetAttr || !val)
        return WHAL_EINVAL;
    return radioDev->driver->GetAttr(radioDev, attr, val);
}

whal_Error whal_Ieee802154_Transmit(whal_Ieee802154 *radioDev,
                                    const uint8_t *frame, size_t len,
                                    const whal_Ieee802154_TxCfg *txCfg)
{
    if (!radioDev || !radioDev->driver || !radioDev->driver->Transmit ||
        !frame || !txCfg || len == 0 || len > WHAL_IEEE802154_FRAME_MAX)
        return WHAL_EINVAL;
    return radioDev->driver->Transmit(radioDev, frame, len, txCfg);
}

whal_Error whal_Ieee802154_GetRssi(whal_Ieee802154 *radioDev, int8_t *rssi)
{
    if (!radioDev || !radioDev->driver || !radioDev->driver->GetRssi || !rssi)
        return WHAL_EINVAL;
    return radioDev->driver->GetRssi(radioDev, rssi);
}

whal_Error whal_Ieee802154_Cca(whal_Ieee802154 *radioDev, bool *busy)
{
    if (!radioDev || !radioDev->driver || !radioDev->driver->Cca || !busy)
        return WHAL_EINVAL;
    return radioDev->driver->Cca(radioDev, busy);
}

whal_Error whal_Ieee802154_EnergyDetect(whal_Ieee802154 *radioDev,
                                        uint32_t durationUs)
{
    if (!radioDev || !radioDev->driver || !radioDev->driver->EnergyDetect)
        return WHAL_EINVAL;
    return radioDev->driver->EnergyDetect(radioDev, durationUs);
}

whal_Error whal_Ieee802154_PendingTableEnable(whal_Ieee802154 *radioDev,
                                              bool enable)
{
    if (!radioDev || !radioDev->driver || !radioDev->driver->PendingTableEnable)
        return WHAL_EINVAL;
    return radioDev->driver->PendingTableEnable(radioDev, enable);
}

whal_Error whal_Ieee802154_PendingTableAddShort(whal_Ieee802154 *radioDev,
                                                uint16_t addr)
{
    if (!radioDev || !radioDev->driver ||
        !radioDev->driver->PendingTableAddShort)
        return WHAL_EINVAL;
    return radioDev->driver->PendingTableAddShort(radioDev, addr);
}

whal_Error whal_Ieee802154_PendingTableAddExtended(whal_Ieee802154 *radioDev,
                                                   const uint8_t *addr)
{
    if (!radioDev || !radioDev->driver ||
        !radioDev->driver->PendingTableAddExtended || !addr)
        return WHAL_EINVAL;
    return radioDev->driver->PendingTableAddExtended(radioDev, addr);
}

whal_Error whal_Ieee802154_PendingTableRemoveShort(whal_Ieee802154 *radioDev,
                                                   uint16_t addr)
{
    if (!radioDev || !radioDev->driver ||
        !radioDev->driver->PendingTableRemoveShort)
        return WHAL_EINVAL;
    return radioDev->driver->PendingTableRemoveShort(radioDev, addr);
}

whal_Error whal_Ieee802154_PendingTableRemoveExtended(whal_Ieee802154 *radioDev,
                                                      const uint8_t *addr)
{
    if (!radioDev || !radioDev->driver ||
        !radioDev->driver->PendingTableRemoveExtended || !addr)
        return WHAL_EINVAL;
    return radioDev->driver->PendingTableRemoveExtended(radioDev, addr);
}

whal_Error whal_Ieee802154_PendingTableClear(whal_Ieee802154 *radioDev)
{
    if (!radioDev || !radioDev->driver || !radioDev->driver->PendingTableClear)
        return WHAL_EINVAL;
    return radioDev->driver->PendingTableClear(radioDev);
}
