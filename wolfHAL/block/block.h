/* block.h
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

#ifndef WHAL_BLOCK_H
#define WHAL_BLOCK_H

#include <wolfHAL/error.h>
#include <stddef.h>
#include <stdint.h>

/*
 * @file block.h
 * @brief Generic block device abstraction and driver interface.
 *
 * Block devices are addressed in fixed-size blocks rather than individual
 * bytes. This is suitable for SD cards, eMMC, and similar storage where
 * the smallest addressable unit is a block (e.g. 512 bytes or 4096 bytes).
 */

typedef struct whal_Block whal_Block;

/*
 * @brief Driver vtable for block devices.
 */
typedef struct {
    /* Bring the block device into a usable state. */
    whal_Error (*Init)(whal_Block *blockDev);
    /* Release any resources owned by the block driver. */
    whal_Error (*Deinit)(whal_Block *blockDev);
    /* Read blocks from the device into a buffer. */
    whal_Error (*Read)(whal_Block *blockDev, uint32_t block, void *data, uint32_t blockCount);
    /* Write data to the device starting at @p block. */
    whal_Error (*Write)(whal_Block *blockDev, uint32_t block, const void *data, uint32_t blockCount);
    /* Erase blocks on the device starting at @p block. */
    whal_Error (*Erase)(whal_Block *blockDev, uint32_t block, uint32_t blockCount);
} whal_BlockDriver;

/*
 * @brief Block device instance tying configuration to a driver implementation.
 */
struct whal_Block {
    const uintptr_t base;
    const whal_BlockDriver *driver;
    void *cfg;
};

/*
 * @brief Initialize a block device and its driver.
 *
 * @param blockDev Block device instance to initialize.
 *
 * @retval WHAL_SUCCESS Driver-specific init completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Block_Init(whal_Block *blockDev);
/*
 * @brief Deinitialize a block device.
 *
 * @param blockDev Block device instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Driver-specific deinit completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Block_Deinit(whal_Block *blockDev);
/*
 * @brief Read blocks from a block device into a buffer.
 *
 * @param blockDev   Block device instance to read from.
 * @param block      Starting block number.
 * @param data       Destination buffer.
 * @param blockCount Number of blocks to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Block_Read(whal_Block *blockDev, uint32_t block, void *data,
                           uint32_t blockCount);
/*
 * @brief Write data to a block device.
 *
 * @param blockDev   Block device instance to program.
 * @param block      Starting block number.
 * @param data       Buffer containing data to write.
 * @param blockCount Number of blocks to write.
 *
 * @retval WHAL_SUCCESS Write accepted or completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Block_Write(whal_Block *blockDev, uint32_t block,
                            const void *data, uint32_t blockCount);
/*
 * @brief Erase blocks on a block device.
 *
 * @param blockDev   Block device instance to erase.
 * @param block      Starting block number.
 * @param blockCount Number of blocks to erase.
 *
 * @retval WHAL_SUCCESS Erase accepted or completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Block_Erase(whal_Block *blockDev, uint32_t block,
                            uint32_t blockCount);

#endif /* WHAL_BLOCK_H */
