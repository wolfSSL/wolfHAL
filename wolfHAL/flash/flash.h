/* flash.h
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

#ifndef WHAL_FLASH_H
#define WHAL_FLASH_H

#include <wolfHAL/error.h>
#include <stddef.h>
#include <stdint.h>

/*
 * @file flash.h
 * @brief Generic flash abstraction and driver interface.
 */

typedef struct whal_Flash whal_Flash;

/*
 * @brief Driver vtable for flash devices.
 */
typedef struct {
    /* Bring the flash peripheral into a usable state. */
    whal_Error (*Init)(whal_Flash *flashDev);
    /* Release any resources owned by the flash driver. */
    whal_Error (*Deinit)(whal_Flash *flashDev);
    /* Lock a flash region to prevent modification. */
    whal_Error (*Lock)(whal_Flash *flashDev, size_t addr, size_t len);
    /* Unlock a flash region to allow modification. */
    whal_Error (*Unlock)(whal_Flash *flashDev, size_t addr, size_t len);
    /* Read data from flash into a buffer. */
    whal_Error (*Read)(whal_Flash *flashDev, size_t addr, void *data, size_t dataSz);
    /* Program a region of flash starting at @p addr. */
    whal_Error (*Write)(whal_Flash *flashDev, size_t addr, const void *data, size_t dataSz);
    /* Erase a flash range starting at @p addr. */
    whal_Error (*Erase)(whal_Flash *flashDev, size_t addr, size_t dataSz);
} whal_FlashDriver;

/*
 * @brief Flash device instance tying configuration to a driver implementation.
 */
struct whal_Flash {
    const size_t base;
    const whal_FlashDriver *driver;
    void *cfg;
};

/*
 * @brief Initialize a flash device and its driver.
 *
 * @param flashDev Flash instance to initialize.
 *
 * @retval WHAL_SUCCESS Driver-specific init completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Flash_Init(whal_Flash *flashDev);
/*
 * @brief Deinitialize a flash device.
 *
 * @param flashDev Flash instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Driver-specific deinit completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Flash_Deinit(whal_Flash *flashDev);
/*
 * @brief Lock flash to prevent modification.
 *
 * Current drivers lock the whole device and ignore @p addr and @p len.
 *
 * @param flashDev Flash instance to lock.
 * @param addr     Byte address of the region to lock (driver-defined).
 * @param len      Number of bytes to lock (driver-defined).
 *
 * @retval WHAL_SUCCESS Lock applied.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len);
/*
 * @brief Unlock flash to allow modification.
 *
 * Current drivers unlock the whole device and ignore @p addr and @p len.
 *
 * @param flashDev Flash instance to unlock.
 * @param addr     Byte address of the region to unlock (driver-defined).
 * @param len      Number of bytes to unlock (driver-defined).
 *
 * @retval WHAL_SUCCESS Unlock applied.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len);
/*
 * @brief Read data from flash into a buffer.
 *
 * @param flashDev Flash instance to read from.
 * @param addr     Byte address in flash to read.
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Flash_Read(whal_Flash *flashDev, size_t addr, void *data, size_t dataSz);
/*
 * @brief Write data into flash.
 *
 * @param flashDev Flash instance to program.
 * @param addr     Byte address in flash to start writing.
 * @param data     Buffer containing data to write.
 * @param dataSz   Number of bytes to write.
 *
 * @retval WHAL_SUCCESS Write accepted or completed.
 * @retval WHAL_EINVAL   Null pointer or bad arguments.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Flash_Write(whal_Flash *flashDev, size_t addr, const void *data, size_t dataSz);
/*
 * @brief Erase a region of flash.
 *
 * @param flashDev Flash instance to erase.
 * @param addr     Byte address in flash where erasure starts.
 * @param dataSz   Number of bytes (or sector-aligned size) to erase.
 *
 * @retval WHAL_SUCCESS Erase accepted or completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Flash_Erase(whal_Flash *flashDev, size_t addr, size_t dataSz);

#endif /* WHAL_FLASH_H */
