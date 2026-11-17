/* spimem.h
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

#ifndef WHAL_SPIMEM_H
#define WHAL_SPIMEM_H

#include <wolfHAL/error.h>
#include <stdint.h>
#include <stddef.h>

/*
 * @file spimem.h
 * @brief Generic SPI memory controller abstraction and driver interface.
 *
 * One whole flash command per operation, with its own bus mode, for
 * controllers that are not a byte-stream SPI bus.
 */

typedef struct whal_SpiMem whal_SpiMem;

/* Data direction of an operation. */
#define WHAL_SPIMEM_DIR_NONE 0 /* no data phase */
#define WHAL_SPIMEM_DIR_IN   1 /* read data from the memory */
#define WHAL_SPIMEM_DIR_OUT  2 /* write data to the memory */

/* Lines per phase (opLines, addrLines, dataLines). */
#define WHAL_SPIMEM_LINES_1 1
#define WHAL_SPIMEM_LINES_2 2
#define WHAL_SPIMEM_LINES_4 4
#define WHAL_SPIMEM_LINES_8 8

/*
 * @brief One SPI memory operation.
 */
typedef struct {
    uint16_t opcode;     /* command; with opBytes 2, bits 15:8 go first */
    uint8_t opBytes;     /* command bytes: 1 or 2 */
    uint8_t opLines;     /* lines for the opcode: 1, 2, 4 or 8 */
    uint8_t addrLines;   /* lines for the address (if addrBytes is not 0) */
    uint8_t dataLines;   /* lines for the data (if dir is not NONE) */
    uint8_t dtr;         /* 1 = DTR for all phases, 0 = single rate */
    uint8_t addrBytes;   /* address bytes sent after the opcode: 0, 3 or 4 */
    uint8_t dummyCycles; /* dummy clock cycles before the data phase */
    uint8_t dir;         /* WHAL_SPIMEM_DIR_* */
    uint32_t addr;       /* address, if addrBytes is not 0 */
    void *data;          /* data buffer (in or out), NULL if dataSz is 0 */
    size_t dataSz;       /* data bytes */
} whal_SpiMemOp;

/* Initializer for a 1-bit single rate (1-1-1) operation with a 1-byte
 * opcode. Set the opcode, address, direction and data after it. */
#define WHAL_SPIMEM_OP_INIT_1BIT \
    { .opBytes = 1, .opLines = 1, .addrLines = 1, .dataLines = 1 }

/*
 * @brief A memory-mapped read window (like Linux spi-mem dirmap).
 *
 * The map shows memory bytes offset to offset + length - 1. tmpl is the
 * read operation used for it: opcode, opBytes, lines, dtr, addrBytes and
 * dummyCycles (its dir, addr, data and dataSz are not used).
 */
typedef struct {
    whal_SpiMemOp tmpl;
    size_t offset;
    size_t length;
} whal_SpiMemMap;

/*
 * @brief Driver vtable for SPI memory controllers.
 */
typedef struct {
    /* Initialize the controller. */
    whal_Error (*Init)(whal_SpiMem *spiMemDev);
    /* Deinitialize the controller. */
    whal_Error (*Deinit)(whal_SpiMem *spiMemDev);
    /* Run one operation. */
    whal_Error (*Exec)(whal_SpiMem *spiMemDev, const whal_SpiMemOp *op);
    /* Report the largest data size the controller can run in one operation
     * like op. */
    whal_Error (*MaxDataSz)(whal_SpiMem *spiMemDev, const whal_SpiMemOp *op,
                            size_t *maxSz);
    /* Open a memory-mapped read window (optional). */
    whal_Error (*MapCreate)(whal_SpiMem *spiMemDev, const whal_SpiMemMap *map);
    /* Read through an open window (optional). */
    whal_Error (*MapRead)(whal_SpiMem *spiMemDev, const whal_SpiMemMap *map,
                          size_t offset, void *data, size_t dataSz);
    /* Close the window (optional). */
    whal_Error (*MapDestroy)(whal_SpiMem *spiMemDev,
                             const whal_SpiMemMap *map);
} whal_SpiMemDriver;

/*
 * @brief SPI memory controller device instance.
 */
struct whal_SpiMem {
    const uintptr_t base;
    const whal_SpiMemDriver *driver;
    void *cfg;
};

/*
 * @brief Initialize a SPI memory controller.
 *
 * @param spiMemDev Device instance.
 *
 * @retval WHAL_SUCCESS Initialized.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP The driver does not implement Init.
 */
whal_Error whal_SpiMem_Init(whal_SpiMem *spiMemDev);

/*
 * @brief Deinitialize a SPI memory controller.
 *
 * @param spiMemDev Device instance.
 *
 * @retval WHAL_SUCCESS Deinitialized.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP The driver does not implement Deinit.
 */
whal_Error whal_SpiMem_Deinit(whal_SpiMem *spiMemDev);

/*
 * @brief Run one SPI memory operation (blocking).
 *
 * The controller may split a read with an address. Other operations run as
 * one command, so dataSz must fit whal_SpiMem_MaxDataSz().
 *
 * @param spiMemDev Device instance.
 * @param op        Operation to run.
 *
 * @retval WHAL_SUCCESS   Operation completed.
 * @retval WHAL_EINVAL    Invalid arguments (a line count that is not 1, 2,
 *                        4 or 8, opBytes not 1 or 2, an opcode above 0xFF
 *                        with opBytes 1), or an operation the controller
 *                        cannot run (too much data, too many dummy cycles).
 * @retval WHAL_ETIMEOUT  The controller did not finish in time.
 * @retval WHAL_EHARDWARE The controller reported an error.
 * @retval WHAL_ENOTSUP   The driver does not implement Exec, or the
 *                        controller cannot run the bus mode.
 */
whal_Error whal_SpiMem_Exec(whal_SpiMem *spiMemDev, const whal_SpiMemOp *op);

/*
 * @brief Get the largest data size the controller can run in one operation
 *        like op (op->dataSz is ignored).
 *
 * Memory drivers use it to split writes, e.g. a page program into several
 * programs of at most *maxSz bytes.
 *
 * @param spiMemDev Device instance.
 * @param op        Operation (opcode, address, dummy and direction).
 * @param maxSz     Receives the largest data size in bytes.
 *
 * @retval WHAL_SUCCESS Size reported.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP The driver does not implement MaxDataSz.
 */
whal_Error whal_SpiMem_MaxDataSz(whal_SpiMem *spiMemDev,
                                 const whal_SpiMemOp *op, size_t *maxSz);

/*
 * @brief Open a memory-mapped read window.
 *
 * While a map is open, run no Exec read that the controller would do by
 * DMA, and no write or erase in the mapped range, or close the map first.
 *
 * @param spiMemDev Device instance.
 * @param map       Window: read op template, offset and length.
 *
 * @retval WHAL_SUCCESS Window open.
 * @retval WHAL_EINVAL  Invalid arguments, or a window or read op the
 *                      controller cannot map (out of range, wrong address
 *                      size).
 * @retval WHAL_ENOTSUP The driver or controller has no memory-mapped mode.
 */
whal_Error whal_SpiMem_MapCreate(whal_SpiMem *spiMemDev,
                                 const whal_SpiMemMap *map);

/*
 * @brief Read through an open window.
 *
 * @param spiMemDev Device instance.
 * @param map       Window from whal_SpiMem_MapCreate().
 * @param offset    Byte offset inside the window (not the memory address).
 * @param data      Receive buffer.
 * @param dataSz    Bytes to read; offset + dataSz must fit in the window.
 *
 * @retval WHAL_SUCCESS Data read.
 * @retval WHAL_EINVAL  Invalid arguments or out of the window.
 * @retval WHAL_ENOTSUP The driver has no memory-mapped mode.
 */
whal_Error whal_SpiMem_MapRead(whal_SpiMem *spiMemDev,
                               const whal_SpiMemMap *map, size_t offset,
                               void *data, size_t dataSz);

/*
 * @brief Close a window opened by whal_SpiMem_MapCreate().
 *
 * @param spiMemDev Device instance.
 * @param map       Window to close.
 *
 * @retval WHAL_SUCCESS Window closed.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP The driver has no memory-mapped mode.
 */
whal_Error whal_SpiMem_MapDestroy(whal_SpiMem *spiMemDev,
                                  const whal_SpiMemMap *map);

#endif /* WHAL_SPIMEM_H */
