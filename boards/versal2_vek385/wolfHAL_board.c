/* wolfHAL_board.c
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

/* Example board configuration for the AMD VEK385 (Versal Gen 2) */

#include <stdint.h>
#include <stddef.h>
#include "wolfHAL_board.h"

/* Arm generic timer (CNTPCT_EL0), as a millisecond tick. */
static uint64_t Board_TimerFreq(void)
{
    uint64_t freq;

    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    return freq;
}

static uint64_t Board_TimerCount(void)
{
    uint64_t count;

    __asm__ volatile("isb; mrs %0, cntpct_el0" : "=r"(count) : : "memory");
    return count;
}

#ifdef WHAL_CFG_64BIT_TICK
uint64_t Board_GetTick(void)
{
    return Board_TimerCount() / (Board_TimerFreq() / 1000);
}
#else
uint32_t Board_GetTick(void)
{
    return (uint32_t)(Board_TimerCount() / (Board_TimerFreq() / 1000));
}
#endif

whal_Timeout g_whalTimeout = {
    .timeoutTicks = 1000, /* 1s timeout */
    .GetTick = Board_GetTick,
};

/* UART */
whal_Uart g_whalUart = {
    .base = WHAL_VERSAL2_UART1_BASE,
    .driver = WHAL_VERSAL2_UART_DRIVER,
    .cfg = &(whal_Pl011_Uart_Cfg) {
        .ibrd = WHAL_PL011_UART_IBRD(BOARD_UART_CLK_HZ, BOARD_UART_BAUD),
        .fbrd = WHAL_PL011_UART_FBRD(BOARD_UART_CLK_HZ, BOARD_UART_BAUD),
        .lcrH = WHAL_PL011_UART_LCRH_8N1_FIFO,
        .timeout = &g_whalTimeout,
    },
};

/* Erases can take seconds on the board. */
static whal_Timeout g_whalFlashTimeout = {
    .timeoutTicks = 10000,
    .GetTick = Board_GetTick,
};

/* OSPI DMA reads. The MMU and caches are off, so no cache upkeep. */
static whal_DmaMemXfer g_whalOspiDmaXfer;

whal_Dma g_whalOspiDma = {
    .base = WHAL_VERSAL2_OSPI_BASE,
    .driver = &whal_Versal2_OspiDma_Driver,
    .cfg = &(whal_Versal2_OspiDma_Cfg) {
        .srcRdAddr = WHAL_VERSAL2_OSPI_DMA_SRC_RD_ADDR,
        .dstCtrl = WHAL_VERSAL2_OSPI_DMA_DST_CTRL,
        .timeout = &g_whalFlashTimeout,
        .xfer = &g_whalOspiDmaXfer,
    },
};

/* OSPI controller */
whal_SpiMem g_whalOspi = {
    .base = WHAL_VERSAL2_OSPI_BASE,
    .driver = WHAL_VERSAL2_OSPI_DRIVER,
    .cfg = &(whal_CadenceOspi_SpiMem_Cfg) {
        .devSize = WHAL_VERSAL2_OSPI_DEV_SIZE_VEK385,
        .baudDiv = BOARD_OSPI_BAUD_DIV,
        .csLines = BOARD_OSPI_CS_LINES,
        .rdCapture = BOARD_OSPI_RD_CAPTURE,
        .indTrigBase = WHAL_VERSAL2_OSPI_IND_TRIG_BASE,
        .indTrigRange = WHAL_VERSAL2_OSPI_IND_TRIG_RANGE,
        .sramPartition = WHAL_VERSAL2_OSPI_SRAM_PARTITION,
        .dmaPeriphCfg = WHAL_VERSAL2_OSPI_DMA_PERIPH_CFG,
        .dma = &g_whalOspiDma,
        .dmaCh = WHAL_VERSAL2_OSPI_DMA_CH,
        .dmaAlign = 4,
        .dmaMaxLen = WHAL_VERSAL2_OSPI_DMA_MAX_LEN,
        .mapBase = WHAL_VERSAL2_OSPI_MAP_BASE,
        .mapSize = WHAL_VERSAL2_OSPI_MAP_SIZE,
        .MapSelect = whal_Versal2_Ospi_MapSelect,
        .timeout = &g_whalTimeout,
    },
};

/* MT35XU02G NOR flash on the OSPI, 1-bit */
whal_Flash g_whalFlash = {
    .driver = &whal_SpiMemNor_Flash_Driver,
    .cfg = &(whal_SpiMemNor_Flash_Cfg) {
        .spiMemDev = &g_whalOspi,
        .capacity = BOARD_FLASH_SIZE,
        .pageSz = 256,
        .eraseSz = BOARD_FLASH_SECTOR_SZ,
        .addrBytes = 4,
        .readOp = BOARD_FLASH_READ4,
        .readDummy = 0,
        .progOp = BOARD_FLASH_PROG4,
        .eraseOp = BOARD_FLASH_ERASE4,
        .flags = WHAL_SPIMEM_NOR_FLAG_RESET |
                 WHAL_SPIMEM_NOR_FLAG_RESET_OCTAL_DTR,
        .id = BOARD_FLASH_JEDEC_ID,
        .errOp = WHAL_SPIMEM_NOR_MT35X_ERR_OP,
        .errMask = WHAL_SPIMEM_NOR_MT35X_ERR_MASK,
        .errClearOp = WHAL_SPIMEM_NOR_MT35X_ERR_CLEAR_OP,
        .rstRegOp = WHAL_SPIMEM_NOR_MT35X_RST_REG_OP,
        .rstRegAddrBytes = WHAL_SPIMEM_NOR_MT35X_RST_REG_ADDR_BYTES,
        .rstRegAddr = WHAL_SPIMEM_NOR_MT35X_RST_REG_ADDR,
        .rstRegVal = WHAL_SPIMEM_NOR_MT35X_RST_REG_VAL,
        .timeout = &g_whalFlashTimeout,
    },
};

void Board_WaitMs(size_t ms)
{
    uint64_t start = Board_TimerCount();
    uint64_t ticks = (Board_TimerFreq() / 1000) * ms;

    while ((Board_TimerCount() - start) < ticks) {
    }
}

whal_Error Board_Init(void)
{
    whal_Error err;

    /* Clocks and pins are set up by the boot firmware (PLM). */
    err = whal_Uart_Init(&g_whalUart);
    if (err) {
        return err;
    }

    err = whal_Dma_Init(&g_whalOspiDma);
    if (err) {
        return err;
    }

    err = whal_SpiMem_Init(&g_whalOspi);
    if (err) {
        return err;
    }

    err = whal_Flash_Init(&g_whalFlash);
    if (err) {
        return err;
    }

    return WHAL_SUCCESS;
}

whal_Error Board_Deinit(void)
{
    whal_Error err;

    err = whal_Flash_Deinit(&g_whalFlash);
    if (err) {
        return err;
    }

    err = whal_SpiMem_Deinit(&g_whalOspi);
    if (err) {
        return err;
    }

    err = whal_Dma_Deinit(&g_whalOspiDma);
    if (err) {
        return err;
    }

    err = whal_Uart_Deinit(&g_whalUart);
    if (err) {
        return err;
    }

    return WHAL_SUCCESS;
}
