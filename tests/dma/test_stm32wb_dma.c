/* test_stm32wb_dma.c
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

#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/dma/stm32wb_dma.h>
#include "wolfHAL_board.h"
#include "test.h"

#ifdef BOARD_DMA

/* Channel 1 is unused by the board and its NVIC line stays disabled */
#define TEST_CH         0
#define TEST_HW_CH      (TEST_CH + 1)

#define DMA_ISR         0x00
#define DMA_CCR         (0x08 + 0x14 * (TEST_HW_CH - 1))
#define DMA_CNDTR       (0x0C + 0x14 * (TEST_HW_CH - 1))
#define DMA_CPAR        (0x10 + 0x14 * (TEST_HW_CH - 1))
#define DMA_CMAR        (0x14 + 0x14 * (TEST_HW_CH - 1))
#define DMAMUX_CCR      (0x04 * TEST_CH)

#define CCR_TCIE        (1UL << 1)
#define CCR_TEIE        (1UL << 3)
#define CCR_DIR         (1UL << 4)
#define CCR_CIRC        (1UL << 5)
#define CCR_PINC        (1UL << 6)
#define CCR_MINC        (1UL << 7)
#define CCR_PSIZE(n)    ((uint32_t)(n) << 8)
#define CCR_MSIZE(n)    ((uint32_t)(n) << 10)
#define CCR_MEM2MEM     (1UL << 14)

#define ISR_TCIF        (1UL << (4 * (TEST_HW_CH - 1) + 1))
#define ISR_TEIF        (1UL << (4 * (TEST_HW_CH - 1) + 3))

/* Arbitrary peripheral-side address; Configure only stores it */
#define FAKE_PERIPH     0x40013828u

/* Reserved AHB2 address space; a DMA read here raises a transfer error */
#define RESERVED_ADDR   0x48001400u

#define XFER_SZ         32

static uint8_t g_src[XFER_SZ] __attribute__((aligned(4)));
static uint8_t g_dst[XFER_SZ] __attribute__((aligned(4)));

typedef struct {
    int calls;
    whal_Error err;
} CbRecord;

static void RecordCb(void *ctx, whal_Error err)
{
    CbRecord *rec = (CbRecord *)ctx;

    rec->calls++;
    rec->err = err;
}

static uintptr_t DmaBase(void)
{
    return g_whalDma1.base;
}

static uintptr_t MuxBase(void)
{
    return ((whal_Stm32wb_Dma_Cfg *)g_whalDma1.cfg)->dmamuxBase;
}

static void FillBuffers(void)
{
    for (size_t i = 0; i < XFER_SZ; i++) {
        g_src[i] = (uint8_t)(i * 7 + 3);
        g_dst[i] = 0;
    }
}

static void Test_Dma_PeriphToMem(void)
{
    whal_Stm32wb_Dma_ChCfg cfg = {
        .dir = WHAL_STM32WB_DMA_DIR_PERIPH_TO_MEM,
        .srcAddr = FAKE_PERIPH,
        .dstAddr = (uint32_t)(size_t)g_dst,
        .length = 7,
        .width = WHAL_STM32WB_DMA_WIDTH_16BIT,
        .srcInc = WHAL_STM32WB_DMA_INC_DISABLE,
        .dstInc = WHAL_STM32WB_DMA_INC_ENABLE,
        .dmamuxReqId = 14,
    };

    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_whalDma1, TEST_CH, &cfg),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_CCR),
                   CCR_TCIE | CCR_TEIE | CCR_MINC |
                   CCR_PSIZE(1) | CCR_MSIZE(1));
    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_CNDTR), 7);
    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_CPAR), FAKE_PERIPH);
    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_CMAR),
                   (uint32_t)(size_t)g_dst);
    WHAL_ASSERT_EQ(whal_Reg32_Read(MuxBase(), DMAMUX_CCR) & 0x3F, 14);

    WHAL_ASSERT_EQ(whal_Dma_Stop(&g_whalDma1, TEST_CH), WHAL_SUCCESS);
}

static void Test_Dma_MemToPeriph(void)
{
    whal_Stm32wb_Dma_ChCfg cfg = {
        .dir = WHAL_STM32WB_DMA_DIR_MEM_TO_PERIPH,
        .srcAddr = (uint32_t)(size_t)g_src,
        .dstAddr = FAKE_PERIPH,
        .length = 5,
        .width = WHAL_STM32WB_DMA_WIDTH_32BIT,
        .srcInc = WHAL_STM32WB_DMA_INC_ENABLE,
        .dstInc = WHAL_STM32WB_DMA_INC_DISABLE,
        .dmamuxReqId = 15,
        .circular = 1,
    };

    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_whalDma1, TEST_CH, &cfg),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_CCR),
                   CCR_TCIE | CCR_TEIE | CCR_DIR | CCR_CIRC | CCR_MINC |
                   CCR_PSIZE(2) | CCR_MSIZE(2));
    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_CNDTR), 5);
    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_CPAR), FAKE_PERIPH);
    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_CMAR),
                   (uint32_t)(size_t)g_src);
    WHAL_ASSERT_EQ(whal_Reg32_Read(MuxBase(), DMAMUX_CCR) & 0x3F, 15);

    WHAL_ASSERT_EQ(whal_Dma_Stop(&g_whalDma1, TEST_CH), WHAL_SUCCESS);
}

static void Test_Dma_MemToMemRegs(void)
{
    whal_Stm32wb_Dma_ChCfg cfg = {
        .dir = WHAL_STM32WB_DMA_DIR_MEM_TO_MEM,
        .srcAddr = (uint32_t)(size_t)g_src,
        .dstAddr = (uint32_t)(size_t)g_dst,
        .length = XFER_SZ,
        .width = WHAL_STM32WB_DMA_WIDTH_8BIT,
        .srcInc = WHAL_STM32WB_DMA_INC_ENABLE,
        .dstInc = WHAL_STM32WB_DMA_INC_ENABLE,
    };

    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_whalDma1, TEST_CH, &cfg),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_CCR),
                   CCR_TCIE | CCR_TEIE | CCR_PINC | CCR_MINC | CCR_MEM2MEM);
    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_CNDTR), XFER_SZ);
    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_CPAR),
                   (uint32_t)(size_t)g_src);
    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_CMAR),
                   (uint32_t)(size_t)g_dst);

    WHAL_ASSERT_EQ(whal_Dma_Stop(&g_whalDma1, TEST_CH), WHAL_SUCCESS);
}

static void Test_Dma_Reject(void)
{
    whal_Stm32wb_Dma_ChCfg cfg = {
        .dir = WHAL_STM32WB_DMA_DIR_MEM_TO_MEM,
        .srcAddr = (uint32_t)(size_t)g_src,
        .dstAddr = (uint32_t)(size_t)g_dst,
        .length = XFER_SZ,
    };
    size_t numCh = ((whal_Stm32wb_Dma_Cfg *)g_whalDma1.cfg)->numChannels;

    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_whalDma1, numCh, &cfg),
                   WHAL_EINVAL);

    cfg.dir = (whal_Stm32wb_Dma_Dir)(WHAL_STM32WB_DMA_DIR_MEM_TO_MEM + 1);
    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_whalDma1, TEST_CH, &cfg),
                   WHAL_EINVAL);
}

static void RunMemToMem(whal_Stm32wb_Dma_Width width, uint16_t items)
{
    whal_Stm32wb_Dma_ChCfg cfg = {
        .dir = WHAL_STM32WB_DMA_DIR_MEM_TO_MEM,
        .srcAddr = (uint32_t)(size_t)g_src,
        .dstAddr = (uint32_t)(size_t)g_dst,
        .length = items,
        .width = width,
        .srcInc = WHAL_STM32WB_DMA_INC_ENABLE,
        .dstInc = WHAL_STM32WB_DMA_INC_ENABLE,
    };
    CbRecord rec = {0};

    FillBuffers();

    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_whalDma1, TEST_CH, &cfg),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Dma_Start(&g_whalDma1, TEST_CH), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Reg32_ReadPoll(DmaBase(), DMA_ISR, ISR_TCIF, ISR_TCIF,
                                       &g_whalTimeout), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_ISR) & ISR_TEIF, 0);

    whal_Stm32wb_Dma_IRQHandler(&g_whalDma1, TEST_CH, RecordCb, &rec);
    WHAL_ASSERT_EQ(rec.calls, 1);
    WHAL_ASSERT_EQ(rec.err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_ISR) & ISR_TCIF, 0);

    WHAL_ASSERT_MEM_EQ(g_dst, g_src, XFER_SZ);

    WHAL_ASSERT_EQ(whal_Dma_Stop(&g_whalDma1, TEST_CH), WHAL_SUCCESS);
}

static void Test_Dma_MemToMem8(void)
{
    RunMemToMem(WHAL_STM32WB_DMA_WIDTH_8BIT, XFER_SZ);
}

static void Test_Dma_MemToMem32(void)
{
    RunMemToMem(WHAL_STM32WB_DMA_WIDTH_32BIT, XFER_SZ / 4);
}

static void Test_Dma_TransferError(void)
{
    whal_Stm32wb_Dma_ChCfg cfg = {
        .dir = WHAL_STM32WB_DMA_DIR_MEM_TO_MEM,
        .srcAddr = RESERVED_ADDR,
        .dstAddr = (uint32_t)(size_t)g_dst,
        .length = 4,
        .width = WHAL_STM32WB_DMA_WIDTH_32BIT,
        .srcInc = WHAL_STM32WB_DMA_INC_ENABLE,
        .dstInc = WHAL_STM32WB_DMA_INC_ENABLE,
    };
    CbRecord rec = {0};

    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_whalDma1, TEST_CH, &cfg),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Dma_Start(&g_whalDma1, TEST_CH), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Reg32_ReadPoll(DmaBase(), DMA_ISR, ISR_TEIF, ISR_TEIF,
                                       &g_whalTimeout), WHAL_SUCCESS);

    whal_Stm32wb_Dma_IRQHandler(&g_whalDma1, TEST_CH, RecordCb, &rec);
    WHAL_ASSERT_EQ(rec.calls, 1);
    WHAL_ASSERT_EQ(rec.err, WHAL_EHARDWARE);
    WHAL_ASSERT_EQ(whal_Reg32_Read(DmaBase(), DMA_ISR) &
                   (ISR_TEIF | ISR_TCIF), 0);

    WHAL_ASSERT_EQ(whal_Dma_Stop(&g_whalDma1, TEST_CH), WHAL_SUCCESS);
}

#else

static void Test_Dma_NoBoardDma(void)
{
    WHAL_SKIP();
}

#endif /* BOARD_DMA */

void whal_Test_Dma_Platform(void)
{
    WHAL_TEST_SUITE_START("dma_platform");
#ifdef BOARD_DMA
    WHAL_TEST(Test_Dma_PeriphToMem);
    WHAL_TEST(Test_Dma_MemToPeriph);
    WHAL_TEST(Test_Dma_MemToMemRegs);
    WHAL_TEST(Test_Dma_Reject);
    WHAL_TEST(Test_Dma_MemToMem8);
    WHAL_TEST(Test_Dma_MemToMem32);
    WHAL_TEST(Test_Dma_TransferError);
#else
    WHAL_TEST(Test_Dma_NoBoardDma);
#endif
    WHAL_TEST_SUITE_END();
}
