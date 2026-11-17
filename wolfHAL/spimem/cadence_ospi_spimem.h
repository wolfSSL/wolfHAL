/* cadence_ospi_spimem.h
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

#ifndef WHAL_CADENCE_OSPI_SPIMEM_H
#define WHAL_CADENCE_OSPI_SPIMEM_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/spimem/spimem.h>
#include <wolfHAL/dma/dma.h>
#include <wolfHAL/timeout.h>

/*
 * @file cadence_ospi_spimem.h
 * @brief Cadence OSPI flash controller driver (SPI memory controller).
 *
 * Versal Gen 2 OSPI (AM026 ch. 66). STIG commands of up to 8 data bytes,
 * optional polled whal_Dma for long reads, 2-byte opcodes in octal, optional
 * memory-mapped reads (direct access mode). No PHY or mode bits.
 */

/*
 * @brief Controller configuration.
 *
 * DMA reads: per read the driver passes a whal_DmaMemXfer (buf and len,
 * dmaAlign aligned) to whal_Dma_Configure, then calls Start before the
 * indirect read starts and Wait after. An error from any of them fails the
 * read. The DMA must have Wait.
 */
typedef struct {
    uint32_t devSize;        /* DEV_SIZE_CONFIG value */
    uint32_t baudDiv;        /* CONFIG mstr_baud_div field */
    uint32_t csLines;        /* CONFIG periph_cs_lines field */
    uint32_t rdCapture;      /* RD_DATA_CAPTURE value */
    /* Indirect read (DMA only). */
    uint32_t indTrigBase;    /* IND_AHB_ADDR_TRIG (AMD: 0xC0000000) */
    uint32_t indTrigRange;   /* IND_TRIG_ADDR_RANGE (AMD: 6 = 64 B) */
    uint32_t sramPartition;  /* SRAM_PARTITION_CFG (AMD: 0xFE) */
    uint32_t dmaPeriphCfg;   /* DMA_PERIPH_CONFIG (AMD: 0x602) */
    whal_Dma *dma;           /* polled DMA for reads; NULL = STIG only */
    size_t dmaCh;            /* channel on dma */
    size_t dmaAlign;         /* buf and len alignment for a DMA read */
    /* Memory-mapped reads (direct access mode). mapBase 0 = none. */
    uintptr_t mapBase;       /* window start (AMD: 0xC0000000) */
    size_t mapSize;          /* window size (AMD: 512 MB) */
    /* Optional: route the window on (1) or off (0) at the SoC; maps
     * call it on create (1) and destroy (0). */
    void (*MapSelect)(int on);
    whal_Timeout *timeout;
} whal_CadenceOspi_SpiMem_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU from the
 * WHAL_CFG_CADENCE_OSPI_SPIMEM_DEV initializer in wolfHAL_board.h.
 */
#ifdef WHAL_CFG_CADENCE_OSPI_SPIMEM_SINGLE_INSTANCE
extern const whal_SpiMem whal_CadenceOspi_SpiMem_Dev;
#endif

#ifndef WHAL_CFG_CADENCE_OSPI_SPIMEM_DIRECT_API_MAPPING
/*
 * @brief Driver instance for the Cadence OSPI controller.
 */
extern const whal_SpiMemDriver whal_CadenceOspi_SpiMem_Driver;

/*
 * @brief Initialize the controller.
 *
 * Controller off: sets clock, CS, read capture and devSize from cfg, DTR
 * and PHY modes off, instruction registers 1-bit. Then turns it on.
 *
 * @param spiMemDev Device instance.
 *
 * @retval WHAL_SUCCESS  Initialized.
 * @retval WHAL_EINVAL   Invalid arguments.
 * @retval WHAL_ETIMEOUT The controller did not become idle.
 */
whal_Error whal_CadenceOspi_SpiMem_Init(whal_SpiMem *spiMemDev);

/*
 * @brief Deinitialize the controller (turn it off).
 *
 * @param spiMemDev Device instance.
 *
 * @retval WHAL_SUCCESS Deinitialized.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_CadenceOspi_SpiMem_Deinit(whal_SpiMem *spiMemDev);

/*
 * @brief Run one SPI memory operation.
 *
 * Address reads go as 8-byte STIG commands, or DMA for the aligned middle
 * (dma set, no dummy, no DTR, addrBytes matches devSize). A DTR read must
 * be an even length.
 *
 * @param spiMemDev Device instance.
 * @param op        Operation to run.
 *
 * @retval WHAL_SUCCESS   Operation completed.
 * @retval WHAL_EINVAL    Invalid arguments, a line count that is not 1, 2,
 *                        4 or 8, opBytes not 1 or 2, more than 4 address
 *                        bytes, more than 31 dummy cycles, too much data,
 *                        or an odd DTR read.
 * @retval WHAL_ENOTSUP   A 2-byte opcode on fewer than 8 lines (AM028:
 *                        dual-byte opcode mode is octal only).
 * @retval WHAL_ETIMEOUT  The controller did not finish in time.
 * @retval WHAL_EHARDWARE (or another error) The DMA failed.
 */
whal_Error whal_CadenceOspi_SpiMem_Exec(whal_SpiMem *spiMemDev,
                                        const whal_SpiMemOp *op);

/*
 * @brief Get the largest data size for one operation like op.
 *
 * 8 bytes (one STIG command) for every operation except a read with an
 * address, which has no limit (SIZE_MAX).
 *
 * @param spiMemDev Device instance.
 * @param op        Operation.
 * @param maxSz     Receives the largest data size in bytes.
 *
 * @retval WHAL_SUCCESS Size reported.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_CadenceOspi_SpiMem_MaxDataSz(whal_SpiMem *spiMemDev,
                                             const whal_SpiMemOp *op,
                                             size_t *maxSz);

/*
 * @brief Open a memory-mapped read window (direct access mode).
 *
 * The window at cfg mapBase shows flash address 0 at its start; the map
 * must fit in cfg mapSize. Turns on the direct access controller and
 * calls cfg MapSelect(1) if set. The read op needs addrBytes matching
 * devSize and single rate (no DTR).
 *
 * @param spiMemDev Device instance.
 * @param map       Window: read op template, offset and length.
 *
 * @retval WHAL_SUCCESS Window open.
 * @retval WHAL_EINVAL  Invalid arguments, length 0, past mapSize, or a
 *                      read op the controller cannot use.
 * @retval WHAL_ENOTSUP No mapBase, a DTR op, or a 2-byte opcode on fewer
 *                      than 8 lines.
 */
whal_Error whal_CadenceOspi_SpiMem_MapCreate(whal_SpiMem *spiMemDev,
                                             const whal_SpiMemMap *map);

/*
 * @brief Read through an open window.
 *
 * Sets the read op's lines, opcode and dummy cycles again first, since
 * Exec may have changed them, then copies from the window.
 *
 * @param spiMemDev Device instance.
 * @param map       Window from whal_CadenceOspi_SpiMem_MapCreate().
 * @param offset    Byte offset inside the window.
 * @param data      Receive buffer.
 * @param dataSz    Bytes to read.
 *
 * @retval WHAL_SUCCESS  Data read.
 * @retval WHAL_EINVAL   Invalid arguments or out of the window.
 * @retval WHAL_ENOTSUP  No mapBase.
 * @retval WHAL_ETIMEOUT The controller did not become idle.
 */
whal_Error whal_CadenceOspi_SpiMem_MapRead(whal_SpiMem *spiMemDev,
                                           const whal_SpiMemMap *map,
                                           size_t offset, void *data,
                                           size_t dataSz);

/*
 * @brief Close a window: calls cfg MapSelect(0) if set.
 *
 * @param spiMemDev Device instance.
 * @param map       Window to close.
 *
 * @retval WHAL_SUCCESS Window closed.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP No mapBase.
 */
whal_Error whal_CadenceOspi_SpiMem_MapDestroy(whal_SpiMem *spiMemDev,
                                              const whal_SpiMemMap *map);
#endif /* !WHAL_CFG_CADENCE_OSPI_SPIMEM_DIRECT_API_MAPPING */

#endif /* WHAL_CADENCE_OSPI_SPIMEM_H */
