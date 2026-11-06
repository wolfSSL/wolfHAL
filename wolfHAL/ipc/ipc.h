/* ipc.h
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

#ifndef WHAL_IPC_H
#define WHAL_IPC_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>

/*
 * @file ipc.h
 * @brief Generic IPC abstraction and driver interface.
 */

typedef struct whal_Ipc whal_Ipc;

/*
 * @brief Driver vtable for IPC devices.
 */
typedef struct {
    /* Initialize the IPC hardware. */
    whal_Error (*Init)(whal_Ipc *ipcDev);
    /* Deinitialize the IPC hardware. */
    whal_Error (*Deinit)(whal_Ipc *ipcDev);
    /* Transmit a buffer. */
    whal_Error (*Send)(whal_Ipc *ipcDev, const void *data, size_t dataSz);
    /* Receive into a buffer. */
    whal_Error (*Recv)(whal_Ipc *ipcDev, void *data, size_t dataSz);
} whal_IpcDriver;

/*
 * @brief IPC device instance with a base address, driver vtable, and platform-specific cfg.
 */
struct whal_Ipc {
    const uintptr_t base;
    const whal_IpcDriver *driver;
    void *cfg;
};

/*
 * @brief Initializes an IPC device and its driver.
 *
 * @param ipcDev Pointer to the IPC instance to initialize.
 *
 * @retval WHAL_SUCCESS Driver-specific init completed.
 * @retval WHAL_EINVAL  Null pointer or driver rejected configuration.
 */
whal_Error whal_Ipc_Init(whal_Ipc *ipcDev);

/*
 * @brief Deinitializes an IPC device and releases resources.
 *
 * @param ipcDev Pointer to the IPC instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Driver-specific deinit completed.
 * @retval WHAL_EINVAL  Null pointer or driver refused to deinit.
 */
whal_Error whal_Ipc_Deinit(whal_Ipc *ipcDev);

/*
 * @brief Sends a buffer over the IPC channel.
 *
 * @param ipcDev Pointer to the IPC instance.
 * @param data   Buffer to transmit.
 * @param dataSz Number of bytes to send.
 *
 * @retval WHAL_SUCCESS Buffer was queued or transmitted.
 * @retval WHAL_EINVAL  Null pointer or driver failed to send.
 */
whal_Error whal_Ipc_Send(whal_Ipc *ipcDev, const void *data, size_t dataSz);

/*
 * @brief Receives data from the IPC channel into a buffer.
 *
 * @param ipcDev Pointer to the IPC instance.
 * @param data   Destination buffer.
 * @param dataSz Maximum number of bytes to read.
 *
 * @retval WHAL_SUCCESS Buffer was filled or receive started.
 * @retval WHAL_EINVAL  Null pointer or driver failed to receive.
 */
whal_Error whal_Ipc_Recv(whal_Ipc *ipcDev, void *data, size_t dataSz);

#endif /* WHAL_IPC_H */
