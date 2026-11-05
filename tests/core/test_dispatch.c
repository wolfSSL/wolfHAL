/* test_dispatch.c
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

#include <stdint.h>
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/timer/timer.h>
#include <wolfHAL/rng/rng.h>
#include "../test.h"

/*
 * Mock drivers that return SUCCESS for all operations.
 * Used to verify the generic dispatch layer.
 *
 * Clock and power are board-level drivers — they have no generic
 * whal_<Type>_* API or vtable, so they are not exercised here.
 */

static whal_Error MockGpioInit(whal_Gpio *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockGpioDeinit(whal_Gpio *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockGpioGet(whal_Gpio *d, size_t p, size_t *v) { (void)d; (void)p; *v = 1; return WHAL_SUCCESS; }
static whal_Error MockGpioSet(whal_Gpio *d, size_t p, size_t v) { (void)d; (void)p; (void)v; return WHAL_SUCCESS; }

static const whal_GpioDriver mockGpioDriver = {
    .Init = MockGpioInit,
    .Deinit = MockGpioDeinit,
    .Get = MockGpioGet,
    .Set = MockGpioSet,
};

static whal_Error MockUartInit(whal_Uart *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockUartDeinit(whal_Uart *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockUartSend(whal_Uart *d, const void *data, size_t sz) { (void)d; (void)data; (void)sz; return WHAL_SUCCESS; }
static whal_Error MockUartRecv(whal_Uart *d, void *data, size_t sz) { (void)d; (void)data; (void)sz; return WHAL_SUCCESS; }

static whal_Error MockUartSendAsync(whal_Uart *d, const void *data, size_t sz) { (void)d; (void)data; (void)sz; return WHAL_SUCCESS; }
static whal_Error MockUartRecvAsync(whal_Uart *d, void *data, size_t sz) { (void)d; (void)data; (void)sz; return WHAL_SUCCESS; }

static const whal_UartDriver mockUartDriver = {
    .Init = MockUartInit,
    .Deinit = MockUartDeinit,
    .Send = MockUartSend,
    .Recv = MockUartRecv,
};

static const whal_UartDriver mockUartAsyncDriver = {
    .Init = MockUartInit,
    .Deinit = MockUartDeinit,
    .Send = MockUartSend,
    .Recv = MockUartRecv,
    .SendAsync = MockUartSendAsync,
    .RecvAsync = MockUartRecvAsync,
};

static whal_Error MockFlashInit(whal_Flash *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockFlashDeinit(whal_Flash *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockFlashLock(whal_Flash *d, size_t a, size_t l) { (void)d; (void)a; (void)l; return WHAL_SUCCESS; }
static whal_Error MockFlashUnlock(whal_Flash *d, size_t a, size_t l) { (void)d; (void)a; (void)l; return WHAL_SUCCESS; }
static whal_Error MockFlashRead(whal_Flash *d, size_t a, void *data, size_t sz) { (void)d; (void)a; (void)data; (void)sz; return WHAL_SUCCESS; }
static whal_Error MockFlashWrite(whal_Flash *d, size_t a, const void *data, size_t sz) { (void)d; (void)a; (void)data; (void)sz; return WHAL_SUCCESS; }
static whal_Error MockFlashErase(whal_Flash *d, size_t a, size_t sz) { (void)d; (void)a; (void)sz; return WHAL_SUCCESS; }

static const whal_FlashDriver mockFlashDriver = {
    .Init = MockFlashInit,
    .Deinit = MockFlashDeinit,
    .Lock = MockFlashLock,
    .Unlock = MockFlashUnlock,
    .Read = MockFlashRead,
    .Write = MockFlashWrite,
    .Erase = MockFlashErase,
};

static whal_Error MockTimerInit(whal_Timer *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockTimerDeinit(whal_Timer *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockTimerStart(whal_Timer *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockTimerStop(whal_Timer *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockTimerReset(whal_Timer *d) { (void)d; return WHAL_SUCCESS; }

static const whal_TimerDriver mockTimerDriver = {
    .Init = MockTimerInit,
    .Deinit = MockTimerDeinit,
    .Start = MockTimerStart,
    .Stop = MockTimerStop,
    .Reset = MockTimerReset,
};

static whal_Error MockRngInit(whal_Rng *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockRngDeinit(whal_Rng *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockRngGenerate(whal_Rng *d, void *data, size_t sz) { (void)d; (void)data; (void)sz; return WHAL_SUCCESS; }

static const whal_RngDriver mockRngDriver = {
    .Init = MockRngInit,
    .Deinit = MockRngDeinit,
    .Generate = MockRngGenerate,
};


/* --- GPIO dispatch tests --- */

static void Test_Gpio_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_Gpio_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Gpio_Set(NULL, 0, 0), WHAL_EINVAL);
    size_t val;
    WHAL_ASSERT_EQ(whal_Gpio_Get(NULL, 0, &val), WHAL_EINVAL);
}

static void Test_Gpio_NullDriver(void)
{
    whal_Gpio dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_Gpio_Init(&dev), WHAL_ENOTSUP);
}

static void Test_Gpio_ValidDispatch(void)
{
    whal_Gpio dev = { .driver = &mockGpioDriver };
    WHAL_ASSERT_EQ(whal_Gpio_Init(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Gpio_Set(&dev, 0, 1), WHAL_SUCCESS);
    size_t val;
    WHAL_ASSERT_EQ(whal_Gpio_Get(&dev, 0, &val), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(val, 1);
}

/* --- UART dispatch tests --- */

static void Test_Uart_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_Uart_Init(NULL), WHAL_EINVAL);
    uint8_t buf[1];
    WHAL_ASSERT_EQ(whal_Uart_Send(NULL, buf, 1), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Uart_Recv(NULL, buf, 1), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Uart_SendAsync(NULL, buf, 1), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Uart_RecvAsync(NULL, buf, 1), WHAL_EINVAL);
}

static void Test_Uart_NullDriver(void)
{
    whal_Uart dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_Uart_Init(&dev), WHAL_ENOTSUP);
}

static void Test_Uart_NullAsyncVtable(void)
{
    whal_Uart dev = { .driver = &mockUartDriver };
    uint8_t buf[1] = {0};
    WHAL_ASSERT_EQ(whal_Uart_SendAsync(&dev, buf, 1), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Uart_RecvAsync(&dev, buf, 1), WHAL_ENOTSUP);
}

static void Test_Uart_ValidDispatch(void)
{
    whal_Uart dev = { .driver = &mockUartDriver };
    WHAL_ASSERT_EQ(whal_Uart_Init(&dev), WHAL_SUCCESS);
    uint8_t buf[4] = {0};
    WHAL_ASSERT_EQ(whal_Uart_Send(&dev, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Uart_Recv(&dev, buf, sizeof(buf)), WHAL_SUCCESS);

    whal_Uart asyncDev = { .driver = &mockUartAsyncDriver };
    WHAL_ASSERT_EQ(whal_Uart_SendAsync(&asyncDev, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Uart_RecvAsync(&asyncDev, buf, sizeof(buf)), WHAL_SUCCESS);
}

/* --- Flash dispatch tests --- */

static void Test_Flash_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_Flash_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Lock(NULL, 0, 0), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Unlock(NULL, 0, 0), WHAL_EINVAL);
    uint8_t buf[1];
    WHAL_ASSERT_EQ(whal_Flash_Read(NULL, 0, buf, 1), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Write(NULL, 0, buf, 1), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Erase(NULL, 0, 1), WHAL_EINVAL);
}

static void Test_Flash_NullDriver(void)
{
    whal_Flash dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_Flash_Init(&dev), WHAL_ENOTSUP);
}

static void Test_Flash_ValidDispatch(void)
{
    whal_Flash dev = { .driver = &mockFlashDriver };
    WHAL_ASSERT_EQ(whal_Flash_Init(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Lock(&dev, 0, 0), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Unlock(&dev, 0, 0), WHAL_SUCCESS);
    uint8_t buf[4] = {0};
    WHAL_ASSERT_EQ(whal_Flash_Read(&dev, 0, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Write(&dev, 0, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&dev, 0, sizeof(buf)), WHAL_SUCCESS);
}

/* --- Timer dispatch tests --- */

static void Test_Timer_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_Timer_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Timer_Start(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Timer_Stop(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Timer_Reset(NULL), WHAL_EINVAL);
}

static void Test_Timer_NullDriver(void)
{
    whal_Timer dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_Timer_Init(&dev), WHAL_ENOTSUP);
}

static void Test_Timer_ValidDispatch(void)
{
    whal_Timer dev = { .driver = &mockTimerDriver };
    WHAL_ASSERT_EQ(whal_Timer_Init(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Timer_Start(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Timer_Stop(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Timer_Reset(&dev), WHAL_SUCCESS);
}

/* --- RNG dispatch tests --- */

static void Test_Rng_NullDev(void)
{
    uint8_t buf[1];
    WHAL_ASSERT_EQ(whal_Rng_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Rng_Deinit(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Rng_Generate(NULL, buf, 1), WHAL_EINVAL);
}

static void Test_Rng_NullDriver(void)
{
    whal_Rng dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_Rng_Init(&dev), WHAL_ENOTSUP);
}

static void Test_Rng_ValidDispatch(void)
{
    whal_Rng dev = { .driver = &mockRngDriver };
    WHAL_ASSERT_EQ(whal_Rng_Init(&dev), WHAL_SUCCESS);
    uint8_t buf[4] = {0};
    WHAL_ASSERT_EQ(whal_Rng_Generate(&dev, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Rng_Deinit(&dev), WHAL_SUCCESS);
}

/* --- SPI dispatch tests --- */

static whal_Error MockSpiInit(whal_Spi *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockSpiDeinit(whal_Spi *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockSpiStartCom(whal_Spi *d, whal_Spi_ComCfg *c) { (void)d; (void)c; return WHAL_SUCCESS; }
static whal_Error MockSpiEndCom(whal_Spi *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockSpiSendRecv(whal_Spi *d, const void *tx, size_t txLen, void *rx, size_t rxLen) { (void)d; (void)tx; (void)txLen; (void)rx; (void)rxLen; return WHAL_SUCCESS; }

static const whal_SpiDriver mockSpiDriver = {
    .Init = MockSpiInit,
    .Deinit = MockSpiDeinit,
    .StartCom = MockSpiStartCom,
    .EndCom = MockSpiEndCom,
    .SendRecv = MockSpiSendRecv,
};

static void Test_Spi_NullDev(void)
{
    whal_Spi_ComCfg comCfg = {0};
    uint8_t buf[1];
    WHAL_ASSERT_EQ(whal_Spi_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Spi_Deinit(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Spi_StartCom(NULL, &comCfg), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Spi_EndCom(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Spi_SendRecv(NULL, buf, 1, buf, 1), WHAL_EINVAL);
}

static void Test_Spi_NullDriver(void)
{
    whal_Spi dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_Spi_Init(&dev), WHAL_ENOTSUP);
}

static void Test_Spi_ValidDispatch(void)
{
    whal_Spi dev = { .driver = &mockSpiDriver };
    whal_Spi_ComCfg comCfg = {0};
    uint8_t buf[4] = {0};
    WHAL_ASSERT_EQ(whal_Spi_Init(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Spi_StartCom(&dev, &comCfg), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Spi_SendRecv(&dev, buf, sizeof(buf), buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Spi_EndCom(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Spi_Deinit(&dev), WHAL_SUCCESS);
}

/* --- SPI memory dispatch tests --- */

static whal_Error MockSpiMemInit(whal_SpiMem *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockSpiMemDeinit(whal_SpiMem *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockSpiMemExec(whal_SpiMem *d, const whal_SpiMemOp *op) { (void)d; (void)op; return WHAL_SUCCESS; }
static whal_Error MockSpiMemMaxDataSz(whal_SpiMem *d, const whal_SpiMemOp *op, size_t *maxSz) { (void)d; (void)op; *maxSz = 8; return WHAL_SUCCESS; }

static const whal_SpiMemDriver mockSpiMemDriver = {
    .Init = MockSpiMemInit,
    .Deinit = MockSpiMemDeinit,
    .Exec = MockSpiMemExec,
    .MaxDataSz = MockSpiMemMaxDataSz,
};

static void Test_SpiMem_NullDev(void)
{
    whal_SpiMemOp op = {0};
    size_t maxSz;
    WHAL_ASSERT_EQ(whal_SpiMem_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMem_Deinit(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(NULL, &op), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMem_MaxDataSz(NULL, &op, &maxSz), WHAL_EINVAL);
}

static void Test_SpiMem_NullArgs(void)
{
    whal_SpiMem dev = { .driver = &mockSpiMemDriver };
    whal_SpiMemOp op = {0};
    size_t maxSz;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, NULL), WHAL_EINVAL);
    /* A data phase needs a buffer. */
    op.dir = WHAL_SPIMEM_DIR_IN;
    op.dataSz = 4;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, &op), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMem_MaxDataSz(&dev, NULL, &maxSz), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMem_MaxDataSz(&dev, &op, NULL), WHAL_EINVAL);
}

static void Test_SpiMem_NullDriver(void)
{
    whal_SpiMem dev = { .driver = NULL };
    whal_SpiMemOp op = {0};
    size_t maxSz;
    WHAL_ASSERT_EQ(whal_SpiMem_Init(&dev), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, &op), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_SpiMem_MaxDataSz(&dev, &op, &maxSz), WHAL_ENOTSUP);
}

static void Test_SpiMem_ValidDispatch(void)
{
    whal_SpiMem dev = { .driver = &mockSpiMemDriver };
    uint8_t buf[4] = {0};
    whal_SpiMemOp op = {
        .opcode = 0x9F,
        .dir = WHAL_SPIMEM_DIR_IN,
        .data = buf,
        .dataSz = 3,
    };
    size_t maxSz = 0;
    WHAL_ASSERT_EQ(whal_SpiMem_Init(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, &op), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_SpiMem_MaxDataSz(&dev, &op, &maxSz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(maxSz, 8);
    WHAL_ASSERT_EQ(whal_SpiMem_Deinit(&dev), WHAL_SUCCESS);
}

/* --- Block dispatch tests --- */

static whal_Error MockBlockInit(whal_Block *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockBlockDeinit(whal_Block *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockBlockRead(whal_Block *d, uint32_t b, void *data, uint32_t c) { (void)d; (void)b; (void)data; (void)c; return WHAL_SUCCESS; }
static whal_Error MockBlockWrite(whal_Block *d, uint32_t b, const void *data, uint32_t c) { (void)d; (void)b; (void)data; (void)c; return WHAL_SUCCESS; }
static whal_Error MockBlockErase(whal_Block *d, uint32_t b, uint32_t c) { (void)d; (void)b; (void)c; return WHAL_SUCCESS; }

static const whal_BlockDriver mockBlockDriver = {
    .Init = MockBlockInit,
    .Deinit = MockBlockDeinit,
    .Read = MockBlockRead,
    .Write = MockBlockWrite,
    .Erase = MockBlockErase,
};

static void Test_Block_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_Block_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Block_Deinit(NULL), WHAL_EINVAL);
    uint8_t buf[1];
    WHAL_ASSERT_EQ(whal_Block_Read(NULL, 0, buf, 1), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Block_Write(NULL, 0, buf, 1), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Block_Erase(NULL, 0, 1), WHAL_EINVAL);
}

static void Test_Block_NullDriver(void)
{
    whal_Block dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_Block_Init(&dev), WHAL_ENOTSUP);
}

static void Test_Block_ValidDispatch(void)
{
    whal_Block dev = { .driver = &mockBlockDriver };
    WHAL_ASSERT_EQ(whal_Block_Init(&dev), WHAL_SUCCESS);
    uint8_t buf[4] = {0};
    WHAL_ASSERT_EQ(whal_Block_Read(&dev, 0, buf, 1), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Block_Write(&dev, 0, buf, 1), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Block_Erase(&dev, 0, 1), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Block_Deinit(&dev), WHAL_SUCCESS);
}

/* --- PWM dispatch tests --- */

static whal_Error MockPwmInit(whal_Pwm *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockPwmDeinit(whal_Pwm *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockPwmStart(whal_Pwm *d, uint8_t ch, const whal_Pwm_ChannelCfg *c) { (void)d; (void)ch; (void)c; return WHAL_SUCCESS; }
static whal_Error MockPwmStop(whal_Pwm *d, uint8_t ch) { (void)d; (void)ch; return WHAL_SUCCESS; }

static const whal_PwmDriver mockPwmDriver = {
    .Init = MockPwmInit,
    .Deinit = MockPwmDeinit,
    .Start = MockPwmStart,
    .Stop = MockPwmStop,
};

static void Test_Pwm_NullDev(void)
{
    whal_Pwm_ChannelCfg cfg = { .periodCycles = 100, .pulseCycles = 50 };
    WHAL_ASSERT_EQ(whal_Pwm_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Pwm_Deinit(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Pwm_Start(NULL, 0, &cfg), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Pwm_Stop(NULL, 0), WHAL_EINVAL);
}

static void Test_Pwm_NullDriver(void)
{
    whal_Pwm dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_Pwm_Init(&dev), WHAL_ENOTSUP);
}

static void Test_Pwm_BadWave(void)
{
    whal_Pwm dev = { .driver = &mockPwmDriver };
    whal_Pwm_ChannelCfg cfg = { .periodCycles = 100, .pulseCycles = 101 };
    whal_Pwm_ChannelCfg zero = { .periodCycles = 0, .pulseCycles = 0 };
    WHAL_ASSERT_EQ(whal_Pwm_Start(&dev, 0, NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Pwm_Start(&dev, 0, &cfg), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Pwm_Start(&dev, 0, &zero), WHAL_EINVAL);
    cfg.pulseCycles = 50;
    cfg.polarity = WHAL_PWM_POLARITY_INVERTED + 1;
    WHAL_ASSERT_EQ(whal_Pwm_Start(&dev, 0, &cfg), WHAL_EINVAL);
}

static void Test_Pwm_ValidDispatch(void)
{
    whal_Pwm dev = { .driver = &mockPwmDriver };
    whal_Pwm_ChannelCfg cfg = { .periodCycles = 100, .pulseCycles = 50 };
    whal_Pwm_ChannelCfg full = { .periodCycles = 100, .pulseCycles = 100 };
    whal_Pwm_ChannelCfg inverted = {
        .periodCycles = 100,
        .pulseCycles = 50,
        .polarity = WHAL_PWM_POLARITY_INVERTED,
    };
    WHAL_ASSERT_EQ(whal_Pwm_Init(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Pwm_Start(&dev, 0, &cfg), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Pwm_Stop(&dev, 0), WHAL_SUCCESS);
    /* 100% duty (pulse == period) passes the dispatch check */
    WHAL_ASSERT_EQ(whal_Pwm_Start(&dev, 0, &full), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Pwm_Stop(&dev, 0), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Pwm_Start(&dev, 0, &inverted), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Pwm_Stop(&dev, 0), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Pwm_Deinit(&dev), WHAL_SUCCESS);
}

/* --- Display dispatch tests --- */

static whal_Error MockDisplayInit(whal_Display *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockDisplayDeinit(whal_Display *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockDisplayUpdate(whal_Display *d, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const void *data, size_t dataSz) { (void)d; (void)x; (void)y; (void)w; (void)h; (void)data; (void)dataSz; return WHAL_SUCCESS; }

static const whal_DisplayDriver mockDisplayDriver = {
    .Init = MockDisplayInit,
    .Deinit = MockDisplayDeinit,
    .Update = MockDisplayUpdate,
};

static void Test_Display_NullDev(void)
{
    uint8_t buf[1] = {0};
    WHAL_ASSERT_EQ(whal_Display_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Display_Deinit(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Display_Update(NULL, 0, 0, 1, 1, buf, sizeof(buf)), WHAL_EINVAL);
}

static void Test_Display_NullDriver(void)
{
    whal_Display dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_Display_Init(&dev), WHAL_ENOTSUP);
}

static void Test_Display_BadArgs(void)
{
    whal_Display dev = { .driver = &mockDisplayDriver };
    uint8_t buf[1] = {0};
    WHAL_ASSERT_EQ(whal_Display_Update(&dev, 0, 0, 1, 1, NULL, 1), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Display_Update(&dev, 0, 0, 1, 1, buf, 0), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Display_Update(&dev, 0, 0, 0, 1, buf, sizeof(buf)), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Display_Update(&dev, 0, 0, 1, 0, buf, sizeof(buf)), WHAL_EINVAL);
}

static void Test_Display_ValidDispatch(void)
{
    whal_Display dev = { .driver = &mockDisplayDriver };
    uint8_t buf[4] = {0};
    WHAL_ASSERT_EQ(whal_Display_Init(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Display_Update(&dev, 0, 0, 2, 2, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Display_Deinit(&dev), WHAL_SUCCESS);
}

/* --- Crypto dispatch tests --- */

static uint8_t cIn[16];
static uint8_t cOut[32];
static uint8_t cTag[16];

static whal_Error MockCryptoInit(whal_Crypto *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockCryptoDeinit(whal_Crypto *d) { (void)d; return WHAL_SUCCESS; }

static const whal_CryptoDriver mockCryptoDriver = {
    .Init = MockCryptoInit,
    .Deinit = MockCryptoDeinit,
};

static void Test_Crypto_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_Crypto_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Crypto_Deinit(NULL), WHAL_EINVAL);
}

static void Test_Crypto_NullDriver(void)
{
    whal_Crypto dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_Crypto_Init(&dev), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Crypto_Deinit(&dev), WHAL_ENOTSUP);
}

static void Test_Crypto_ValidDispatch(void)
{
    whal_Crypto dev = { .driver = &mockCryptoDriver };
    WHAL_ASSERT_EQ(whal_Crypto_Init(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Crypto_Deinit(&dev), WHAL_SUCCESS);
}

static whal_Error MockAesEcbOneshot(whal_AesEcb *d, whal_Crypto_Dir dir, const void *k, size_t kSz, const void *in, void *out, size_t sz) { (void)d; (void)dir; (void)k; (void)kSz; (void)in; (void)out; (void)sz; return WHAL_SUCCESS; }
static whal_Error MockAesEcbStart(whal_AesEcb *d, whal_Crypto_Dir dir, const void *k, size_t kSz) { (void)d; (void)dir; (void)k; (void)kSz; return WHAL_SUCCESS; }
static whal_Error MockAesEcbProcess(whal_AesEcb *d, const void *in, void *out, size_t sz) { (void)d; (void)in; (void)out; (void)sz; return WHAL_SUCCESS; }
static whal_Error MockAesEcbStop(whal_AesEcb *d) { (void)d; return WHAL_SUCCESS; }

static const whal_AesEcbDriver mockAesEcbDriver = {
    .Oneshot = MockAesEcbOneshot,
    .Start = MockAesEcbStart,
    .Process = MockAesEcbProcess,
    .Stop = MockAesEcbStop,
};

static const whal_AesEcbDriver mockAesEcbDriverNoStop = {
    .Oneshot = MockAesEcbOneshot,
    .Start = MockAesEcbStart,
    .Process = MockAesEcbProcess,
};

static void Test_AesEcb_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_AesEcb_Oneshot(NULL, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, cOut, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesEcb_Start(NULL, WHAL_CRYPTO_ENCRYPT, cIn, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesEcb_Process(NULL, cIn, cOut, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesEcb_Stop(NULL), WHAL_EINVAL);
}

static void Test_AesEcb_NullDriver(void)
{
    whal_AesEcb dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_AesEcb_Oneshot(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, cOut, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesEcb_Start(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesEcb_Process(&dev, cIn, cOut, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesEcb_Stop(&dev), WHAL_ENOTSUP);
}

static void Test_AesEcb_NoStop(void)
{
    whal_AesEcb dev = { .driver = &mockAesEcbDriverNoStop };
    WHAL_ASSERT_EQ(whal_AesEcb_Stop(&dev), WHAL_ENOTSUP);
}

static void Test_AesEcb_ValidDispatch(void)
{
    whal_AesEcb dev = { .driver = &mockAesEcbDriver };
    WHAL_ASSERT_EQ(whal_AesEcb_Oneshot(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, cOut, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesEcb_Start(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesEcb_Process(&dev, cIn, cOut, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesEcb_Stop(&dev), WHAL_SUCCESS);
}

static whal_Error MockAesCbcOneshot(whal_AesCbc *d, whal_Crypto_Dir dir, const void *k, size_t kSz, const void *iv, const void *in, void *out, size_t sz) { (void)d; (void)dir; (void)k; (void)kSz; (void)iv; (void)in; (void)out; (void)sz; return WHAL_SUCCESS; }
static whal_Error MockAesCbcStart(whal_AesCbc *d, whal_Crypto_Dir dir, const void *k, size_t kSz, const void *iv) { (void)d; (void)dir; (void)k; (void)kSz; (void)iv; return WHAL_SUCCESS; }
static whal_Error MockAesCbcProcess(whal_AesCbc *d, const void *in, void *out, size_t sz) { (void)d; (void)in; (void)out; (void)sz; return WHAL_SUCCESS; }
static whal_Error MockAesCbcStop(whal_AesCbc *d) { (void)d; return WHAL_SUCCESS; }

static const whal_AesCbcDriver mockAesCbcDriver = {
    .Oneshot = MockAesCbcOneshot,
    .Start = MockAesCbcStart,
    .Process = MockAesCbcProcess,
    .Stop = MockAesCbcStop,
};

static const whal_AesCbcDriver mockAesCbcDriverNoStop = {
    .Oneshot = MockAesCbcOneshot,
    .Start = MockAesCbcStart,
    .Process = MockAesCbcProcess,
};

static void Test_AesCbc_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_AesCbc_Oneshot(NULL, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, cIn, cOut, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesCbc_Start(NULL, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesCbc_Process(NULL, cIn, cOut, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesCbc_Stop(NULL), WHAL_EINVAL);
}

static void Test_AesCbc_NullDriver(void)
{
    whal_AesCbc dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_AesCbc_Oneshot(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, cIn, cOut, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesCbc_Start(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesCbc_Process(&dev, cIn, cOut, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesCbc_Stop(&dev), WHAL_ENOTSUP);
}

static void Test_AesCbc_NoStop(void)
{
    whal_AesCbc dev = { .driver = &mockAesCbcDriverNoStop };
    WHAL_ASSERT_EQ(whal_AesCbc_Stop(&dev), WHAL_ENOTSUP);
}

static void Test_AesCbc_ValidDispatch(void)
{
    whal_AesCbc dev = { .driver = &mockAesCbcDriver };
    WHAL_ASSERT_EQ(whal_AesCbc_Oneshot(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, cIn, cOut, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesCbc_Start(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesCbc_Process(&dev, cIn, cOut, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesCbc_Stop(&dev), WHAL_SUCCESS);
}

static whal_Error MockAesCtrOneshot(whal_AesCtr *d, whal_Crypto_Dir dir, const void *k, size_t kSz, const void *iv, const void *in, void *out, size_t sz) { (void)d; (void)dir; (void)k; (void)kSz; (void)iv; (void)in; (void)out; (void)sz; return WHAL_SUCCESS; }
static whal_Error MockAesCtrStart(whal_AesCtr *d, whal_Crypto_Dir dir, const void *k, size_t kSz, const void *iv) { (void)d; (void)dir; (void)k; (void)kSz; (void)iv; return WHAL_SUCCESS; }
static whal_Error MockAesCtrProcess(whal_AesCtr *d, const void *in, void *out, size_t sz) { (void)d; (void)in; (void)out; (void)sz; return WHAL_SUCCESS; }
static whal_Error MockAesCtrStop(whal_AesCtr *d) { (void)d; return WHAL_SUCCESS; }

static const whal_AesCtrDriver mockAesCtrDriver = {
    .Oneshot = MockAesCtrOneshot,
    .Start = MockAesCtrStart,
    .Process = MockAesCtrProcess,
    .Stop = MockAesCtrStop,
};

static const whal_AesCtrDriver mockAesCtrDriverNoStop = {
    .Oneshot = MockAesCtrOneshot,
    .Start = MockAesCtrStart,
    .Process = MockAesCtrProcess,
};

static void Test_AesCtr_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_AesCtr_Oneshot(NULL, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, cIn, cOut, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesCtr_Start(NULL, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesCtr_Process(NULL, cIn, cOut, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesCtr_Stop(NULL), WHAL_EINVAL);
}

static void Test_AesCtr_NullDriver(void)
{
    whal_AesCtr dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_AesCtr_Oneshot(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, cIn, cOut, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesCtr_Start(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesCtr_Process(&dev, cIn, cOut, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesCtr_Stop(&dev), WHAL_ENOTSUP);
}

static void Test_AesCtr_NoStop(void)
{
    whal_AesCtr dev = { .driver = &mockAesCtrDriverNoStop };
    WHAL_ASSERT_EQ(whal_AesCtr_Stop(&dev), WHAL_ENOTSUP);
}

static void Test_AesCtr_ValidDispatch(void)
{
    whal_AesCtr dev = { .driver = &mockAesCtrDriver };
    WHAL_ASSERT_EQ(whal_AesCtr_Oneshot(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, cIn, cOut, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesCtr_Start(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesCtr_Process(&dev, cIn, cOut, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesCtr_Stop(&dev), WHAL_SUCCESS);
}

static whal_Error MockAesGcmOneshot(whal_AesGcm *d, whal_Crypto_Dir dir, const void *k, size_t kSz, const void *iv, size_t ivSz, const void *aad, size_t aadSz, const void *in, void *out, size_t sz, void *tag, size_t tagSz) { (void)d; (void)dir; (void)k; (void)kSz; (void)iv; (void)ivSz; (void)aad; (void)aadSz; (void)in; (void)out; (void)sz; (void)tag; (void)tagSz; return WHAL_SUCCESS; }
static whal_Error MockAesGcmStart(whal_AesGcm *d, whal_Crypto_Dir dir, const void *k, size_t kSz, const void *iv, size_t ivSz, const void *aad, size_t aadSz) { (void)d; (void)dir; (void)k; (void)kSz; (void)iv; (void)ivSz; (void)aad; (void)aadSz; return WHAL_SUCCESS; }
static whal_Error MockAesGcmProcess(whal_AesGcm *d, const void *in, void *out, size_t sz) { (void)d; (void)in; (void)out; (void)sz; return WHAL_SUCCESS; }
static whal_Error MockAesGcmFinalize(whal_AesGcm *d, void *tag, size_t tagSz) { (void)d; (void)tag; (void)tagSz; return WHAL_SUCCESS; }

static const whal_AesGcmDriver mockAesGcmDriver = {
    .Oneshot = MockAesGcmOneshot,
    .Start = MockAesGcmStart,
    .Process = MockAesGcmProcess,
    .Finalize = MockAesGcmFinalize,
};

static const whal_AesGcmDriver mockAesGcmDriverNoFinalize = {
    .Oneshot = MockAesGcmOneshot,
    .Start = MockAesGcmStart,
    .Process = MockAesGcmProcess,
};

static void Test_AesGcm_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_AesGcm_Oneshot(NULL, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, 12, cIn, 16, cIn, cOut, 16, cTag, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesGcm_Start(NULL, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, 12, cIn, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesGcm_Process(NULL, cIn, cOut, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesGcm_Finalize(NULL, cTag, 16), WHAL_EINVAL);
}

static void Test_AesGcm_NullDriver(void)
{
    whal_AesGcm dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_AesGcm_Oneshot(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, 12, cIn, 16, cIn, cOut, 16, cTag, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesGcm_Start(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, 12, cIn, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesGcm_Process(&dev, cIn, cOut, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesGcm_Finalize(&dev, cTag, 16), WHAL_ENOTSUP);
}

static void Test_AesGcm_NoFinalize(void)
{
    whal_AesGcm dev = { .driver = &mockAesGcmDriverNoFinalize };
    WHAL_ASSERT_EQ(whal_AesGcm_Finalize(&dev, cTag, 16), WHAL_ENOTSUP);
}

static void Test_AesGcm_ValidDispatch(void)
{
    whal_AesGcm dev = { .driver = &mockAesGcmDriver };
    WHAL_ASSERT_EQ(whal_AesGcm_Oneshot(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, 12, cIn, 16, cIn, cOut, 16, cTag, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesGcm_Start(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, 12, cIn, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesGcm_Process(&dev, cIn, cOut, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesGcm_Finalize(&dev, cTag, 16), WHAL_SUCCESS);
}

static whal_Error MockAesGmacOneshot(whal_AesGmac *d, const void *k, size_t kSz, const void *iv, size_t ivSz, const void *aad, size_t aadSz, void *tag, size_t tagSz) { (void)d; (void)k; (void)kSz; (void)iv; (void)ivSz; (void)aad; (void)aadSz; (void)tag; (void)tagSz; return WHAL_SUCCESS; }

static const whal_AesGmacDriver mockAesGmacDriver = {
    .Oneshot = MockAesGmacOneshot,
};

static const whal_AesGmacDriver mockAesGmacDriverNoOneshot = {
};

static void Test_AesGmac_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_AesGmac_Oneshot(NULL, cIn, 16, cIn, 12, cIn, 16, cTag, 16), WHAL_EINVAL);
}

static void Test_AesGmac_NullDriver(void)
{
    whal_AesGmac dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_AesGmac_Oneshot(&dev, cIn, 16, cIn, 12, cIn, 16, cTag, 16), WHAL_ENOTSUP);
}

static void Test_AesGmac_NoOneshot(void)
{
    whal_AesGmac dev = { .driver = &mockAesGmacDriverNoOneshot };
    WHAL_ASSERT_EQ(whal_AesGmac_Oneshot(&dev, cIn, 16, cIn, 12, cIn, 16, cTag, 16), WHAL_ENOTSUP);
}

static void Test_AesGmac_ValidDispatch(void)
{
    whal_AesGmac dev = { .driver = &mockAesGmacDriver };
    WHAL_ASSERT_EQ(whal_AesGmac_Oneshot(&dev, cIn, 16, cIn, 12, cIn, 16, cTag, 16), WHAL_SUCCESS);
}

static whal_Error MockAesCcmOneshot(whal_AesCcm *d, whal_Crypto_Dir dir, const void *k, size_t kSz, const void *n, size_t nSz, const void *aad, size_t aadSz, const void *in, void *out, size_t sz, void *tag, size_t tagSz) { (void)d; (void)dir; (void)k; (void)kSz; (void)n; (void)nSz; (void)aad; (void)aadSz; (void)in; (void)out; (void)sz; (void)tag; (void)tagSz; return WHAL_SUCCESS; }
static whal_Error MockAesCcmStart(whal_AesCcm *d, whal_Crypto_Dir dir, const void *k, size_t kSz, const void *n, size_t nSz, const void *aad, size_t aadSz, size_t tagSz, size_t sz) { (void)d; (void)dir; (void)k; (void)kSz; (void)n; (void)nSz; (void)aad; (void)aadSz; (void)tagSz; (void)sz; return WHAL_SUCCESS; }
static whal_Error MockAesCcmProcess(whal_AesCcm *d, const void *in, void *out, size_t sz) { (void)d; (void)in; (void)out; (void)sz; return WHAL_SUCCESS; }
static whal_Error MockAesCcmFinalize(whal_AesCcm *d, void *tag, size_t tagSz) { (void)d; (void)tag; (void)tagSz; return WHAL_SUCCESS; }

static const whal_AesCcmDriver mockAesCcmDriver = {
    .Oneshot = MockAesCcmOneshot,
    .Start = MockAesCcmStart,
    .Process = MockAesCcmProcess,
    .Finalize = MockAesCcmFinalize,
};

static const whal_AesCcmDriver mockAesCcmDriverNoFinalize = {
    .Oneshot = MockAesCcmOneshot,
    .Start = MockAesCcmStart,
    .Process = MockAesCcmProcess,
};

static void Test_AesCcm_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_AesCcm_Oneshot(NULL, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, 13, cIn, 16, cIn, cOut, 16, cTag, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesCcm_Start(NULL, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, 13, cIn, 16, 16, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesCcm_Process(NULL, cIn, cOut, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_AesCcm_Finalize(NULL, cTag, 16), WHAL_EINVAL);
}

static void Test_AesCcm_NullDriver(void)
{
    whal_AesCcm dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_AesCcm_Oneshot(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, 13, cIn, 16, cIn, cOut, 16, cTag, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesCcm_Start(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, 13, cIn, 16, 16, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesCcm_Process(&dev, cIn, cOut, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_AesCcm_Finalize(&dev, cTag, 16), WHAL_ENOTSUP);
}

static void Test_AesCcm_NoFinalize(void)
{
    whal_AesCcm dev = { .driver = &mockAesCcmDriverNoFinalize };
    WHAL_ASSERT_EQ(whal_AesCcm_Finalize(&dev, cTag, 16), WHAL_ENOTSUP);
}

static void Test_AesCcm_ValidDispatch(void)
{
    whal_AesCcm dev = { .driver = &mockAesCcmDriver };
    WHAL_ASSERT_EQ(whal_AesCcm_Oneshot(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, 13, cIn, 16, cIn, cOut, 16, cTag, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesCcm_Start(&dev, WHAL_CRYPTO_ENCRYPT, cIn, 16, cIn, 13, cIn, 16, 16, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesCcm_Process(&dev, cIn, cOut, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesCcm_Finalize(&dev, cTag, 16), WHAL_SUCCESS);
}

static whal_Error MockSha1Oneshot(whal_Sha1 *d, const void *in, size_t inSz, void *dig, size_t digSz) { (void)d; (void)in; (void)inSz; (void)dig; (void)digSz; return WHAL_SUCCESS; }
static whal_Error MockSha1Start(whal_Sha1 *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockSha1Process(whal_Sha1 *d, const void *in, size_t inSz) { (void)d; (void)in; (void)inSz; return WHAL_SUCCESS; }
static whal_Error MockSha1Finalize(whal_Sha1 *d, void *dig, size_t digSz) { (void)d; (void)dig; (void)digSz; return WHAL_SUCCESS; }

static const whal_Sha1Driver mockSha1Driver = {
    .Oneshot = MockSha1Oneshot,
    .Start = MockSha1Start,
    .Process = MockSha1Process,
    .Finalize = MockSha1Finalize,
};

static void Test_Sha1_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_Sha1_Oneshot(NULL, cIn, 16, cOut, 32), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Sha1_Start(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Sha1_Process(NULL, cIn, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Sha1_Finalize(NULL, cOut, 32), WHAL_EINVAL);
}

static void Test_Sha1_NullDriver(void)
{
    whal_Sha1 dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_Sha1_Oneshot(&dev, cIn, 16, cOut, 32), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Sha1_Start(&dev), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Sha1_Process(&dev, cIn, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Sha1_Finalize(&dev, cOut, 32), WHAL_ENOTSUP);
}

static void Test_Sha1_ValidDispatch(void)
{
    whal_Sha1 dev = { .driver = &mockSha1Driver };
    WHAL_ASSERT_EQ(whal_Sha1_Oneshot(&dev, cIn, 16, cOut, 32), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Sha1_Start(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Sha1_Process(&dev, cIn, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Sha1_Finalize(&dev, cOut, 32), WHAL_SUCCESS);
}

static whal_Error MockSha224Oneshot(whal_Sha224 *d, const void *in, size_t inSz, void *dig, size_t digSz) { (void)d; (void)in; (void)inSz; (void)dig; (void)digSz; return WHAL_SUCCESS; }
static whal_Error MockSha224Start(whal_Sha224 *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockSha224Process(whal_Sha224 *d, const void *in, size_t inSz) { (void)d; (void)in; (void)inSz; return WHAL_SUCCESS; }
static whal_Error MockSha224Finalize(whal_Sha224 *d, void *dig, size_t digSz) { (void)d; (void)dig; (void)digSz; return WHAL_SUCCESS; }

static const whal_Sha224Driver mockSha224Driver = {
    .Oneshot = MockSha224Oneshot,
    .Start = MockSha224Start,
    .Process = MockSha224Process,
    .Finalize = MockSha224Finalize,
};

static void Test_Sha224_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_Sha224_Oneshot(NULL, cIn, 16, cOut, 32), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Sha224_Start(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Sha224_Process(NULL, cIn, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Sha224_Finalize(NULL, cOut, 32), WHAL_EINVAL);
}

static void Test_Sha224_NullDriver(void)
{
    whal_Sha224 dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_Sha224_Oneshot(&dev, cIn, 16, cOut, 32), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Sha224_Start(&dev), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Sha224_Process(&dev, cIn, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Sha224_Finalize(&dev, cOut, 32), WHAL_ENOTSUP);
}

static void Test_Sha224_ValidDispatch(void)
{
    whal_Sha224 dev = { .driver = &mockSha224Driver };
    WHAL_ASSERT_EQ(whal_Sha224_Oneshot(&dev, cIn, 16, cOut, 32), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Sha224_Start(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Sha224_Process(&dev, cIn, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Sha224_Finalize(&dev, cOut, 32), WHAL_SUCCESS);
}

static whal_Error MockSha256Oneshot(whal_Sha256 *d, const void *in, size_t inSz, void *dig, size_t digSz) { (void)d; (void)in; (void)inSz; (void)dig; (void)digSz; return WHAL_SUCCESS; }
static whal_Error MockSha256Start(whal_Sha256 *d) { (void)d; return WHAL_SUCCESS; }
static whal_Error MockSha256Process(whal_Sha256 *d, const void *in, size_t inSz) { (void)d; (void)in; (void)inSz; return WHAL_SUCCESS; }
static whal_Error MockSha256Finalize(whal_Sha256 *d, void *dig, size_t digSz) { (void)d; (void)dig; (void)digSz; return WHAL_SUCCESS; }

static const whal_Sha256Driver mockSha256Driver = {
    .Oneshot = MockSha256Oneshot,
    .Start = MockSha256Start,
    .Process = MockSha256Process,
    .Finalize = MockSha256Finalize,
};

static void Test_Sha256_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_Sha256_Oneshot(NULL, cIn, 16, cOut, 32), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Sha256_Start(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Sha256_Process(NULL, cIn, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Sha256_Finalize(NULL, cOut, 32), WHAL_EINVAL);
}

static void Test_Sha256_NullDriver(void)
{
    whal_Sha256 dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_Sha256_Oneshot(&dev, cIn, 16, cOut, 32), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Sha256_Start(&dev), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Sha256_Process(&dev, cIn, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Sha256_Finalize(&dev, cOut, 32), WHAL_ENOTSUP);
}

static void Test_Sha256_ValidDispatch(void)
{
    whal_Sha256 dev = { .driver = &mockSha256Driver };
    WHAL_ASSERT_EQ(whal_Sha256_Oneshot(&dev, cIn, 16, cOut, 32), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Sha256_Start(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Sha256_Process(&dev, cIn, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Sha256_Finalize(&dev, cOut, 32), WHAL_SUCCESS);
}

static whal_Error MockHmacSha1Oneshot(whal_HmacSha1 *d, const void *k, size_t kSz, const void *in, size_t inSz, void *dig, size_t digSz) { (void)d; (void)k; (void)kSz; (void)in; (void)inSz; (void)dig; (void)digSz; return WHAL_SUCCESS; }
static whal_Error MockHmacSha1Start(whal_HmacSha1 *d, const void *k, size_t kSz) { (void)d; (void)k; (void)kSz; return WHAL_SUCCESS; }
static whal_Error MockHmacSha1Process(whal_HmacSha1 *d, const void *in, size_t inSz) { (void)d; (void)in; (void)inSz; return WHAL_SUCCESS; }
static whal_Error MockHmacSha1Finalize(whal_HmacSha1 *d, void *dig, size_t digSz) { (void)d; (void)dig; (void)digSz; return WHAL_SUCCESS; }

static const whal_HmacSha1Driver mockHmacSha1Driver = {
    .Oneshot = MockHmacSha1Oneshot,
    .Start = MockHmacSha1Start,
    .Process = MockHmacSha1Process,
    .Finalize = MockHmacSha1Finalize,
};

static void Test_HmacSha1_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_HmacSha1_Oneshot(NULL, cIn, 16, cIn, 16, cOut, 32), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_HmacSha1_Start(NULL, cIn, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_HmacSha1_Process(NULL, cIn, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_HmacSha1_Finalize(NULL, cOut, 32), WHAL_EINVAL);
}

static void Test_HmacSha1_NullDriver(void)
{
    whal_HmacSha1 dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_HmacSha1_Oneshot(&dev, cIn, 16, cIn, 16, cOut, 32), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_HmacSha1_Start(&dev, cIn, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_HmacSha1_Process(&dev, cIn, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_HmacSha1_Finalize(&dev, cOut, 32), WHAL_ENOTSUP);
}

static void Test_HmacSha1_ValidDispatch(void)
{
    whal_HmacSha1 dev = { .driver = &mockHmacSha1Driver };
    WHAL_ASSERT_EQ(whal_HmacSha1_Oneshot(&dev, cIn, 16, cIn, 16, cOut, 32), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_HmacSha1_Start(&dev, cIn, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_HmacSha1_Process(&dev, cIn, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_HmacSha1_Finalize(&dev, cOut, 32), WHAL_SUCCESS);
}

static whal_Error MockHmacSha224Oneshot(whal_HmacSha224 *d, const void *k, size_t kSz, const void *in, size_t inSz, void *dig, size_t digSz) { (void)d; (void)k; (void)kSz; (void)in; (void)inSz; (void)dig; (void)digSz; return WHAL_SUCCESS; }
static whal_Error MockHmacSha224Start(whal_HmacSha224 *d, const void *k, size_t kSz) { (void)d; (void)k; (void)kSz; return WHAL_SUCCESS; }
static whal_Error MockHmacSha224Process(whal_HmacSha224 *d, const void *in, size_t inSz) { (void)d; (void)in; (void)inSz; return WHAL_SUCCESS; }
static whal_Error MockHmacSha224Finalize(whal_HmacSha224 *d, void *dig, size_t digSz) { (void)d; (void)dig; (void)digSz; return WHAL_SUCCESS; }

static const whal_HmacSha224Driver mockHmacSha224Driver = {
    .Oneshot = MockHmacSha224Oneshot,
    .Start = MockHmacSha224Start,
    .Process = MockHmacSha224Process,
    .Finalize = MockHmacSha224Finalize,
};

static void Test_HmacSha224_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_HmacSha224_Oneshot(NULL, cIn, 16, cIn, 16, cOut, 32), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_HmacSha224_Start(NULL, cIn, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_HmacSha224_Process(NULL, cIn, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_HmacSha224_Finalize(NULL, cOut, 32), WHAL_EINVAL);
}

static void Test_HmacSha224_NullDriver(void)
{
    whal_HmacSha224 dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_HmacSha224_Oneshot(&dev, cIn, 16, cIn, 16, cOut, 32), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_HmacSha224_Start(&dev, cIn, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_HmacSha224_Process(&dev, cIn, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_HmacSha224_Finalize(&dev, cOut, 32), WHAL_ENOTSUP);
}

static void Test_HmacSha224_ValidDispatch(void)
{
    whal_HmacSha224 dev = { .driver = &mockHmacSha224Driver };
    WHAL_ASSERT_EQ(whal_HmacSha224_Oneshot(&dev, cIn, 16, cIn, 16, cOut, 32), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_HmacSha224_Start(&dev, cIn, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_HmacSha224_Process(&dev, cIn, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_HmacSha224_Finalize(&dev, cOut, 32), WHAL_SUCCESS);
}

static whal_Error MockHmacSha256Oneshot(whal_HmacSha256 *d, const void *k, size_t kSz, const void *in, size_t inSz, void *dig, size_t digSz) { (void)d; (void)k; (void)kSz; (void)in; (void)inSz; (void)dig; (void)digSz; return WHAL_SUCCESS; }
static whal_Error MockHmacSha256Start(whal_HmacSha256 *d, const void *k, size_t kSz) { (void)d; (void)k; (void)kSz; return WHAL_SUCCESS; }
static whal_Error MockHmacSha256Process(whal_HmacSha256 *d, const void *in, size_t inSz) { (void)d; (void)in; (void)inSz; return WHAL_SUCCESS; }
static whal_Error MockHmacSha256Finalize(whal_HmacSha256 *d, void *dig, size_t digSz) { (void)d; (void)dig; (void)digSz; return WHAL_SUCCESS; }

static const whal_HmacSha256Driver mockHmacSha256Driver = {
    .Oneshot = MockHmacSha256Oneshot,
    .Start = MockHmacSha256Start,
    .Process = MockHmacSha256Process,
    .Finalize = MockHmacSha256Finalize,
};

static void Test_HmacSha256_NullDev(void)
{
    WHAL_ASSERT_EQ(whal_HmacSha256_Oneshot(NULL, cIn, 16, cIn, 16, cOut, 32), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_HmacSha256_Start(NULL, cIn, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_HmacSha256_Process(NULL, cIn, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_HmacSha256_Finalize(NULL, cOut, 32), WHAL_EINVAL);
}

static void Test_HmacSha256_NullDriver(void)
{
    whal_HmacSha256 dev = { .driver = NULL };
    WHAL_ASSERT_EQ(whal_HmacSha256_Oneshot(&dev, cIn, 16, cIn, 16, cOut, 32), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_HmacSha256_Start(&dev, cIn, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_HmacSha256_Process(&dev, cIn, 16), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_HmacSha256_Finalize(&dev, cOut, 32), WHAL_ENOTSUP);
}

static void Test_HmacSha256_ValidDispatch(void)
{
    whal_HmacSha256 dev = { .driver = &mockHmacSha256Driver };
    WHAL_ASSERT_EQ(whal_HmacSha256_Oneshot(&dev, cIn, 16, cIn, 16, cOut, 32), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_HmacSha256_Start(&dev, cIn, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_HmacSha256_Process(&dev, cIn, 16), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_HmacSha256_Finalize(&dev, cOut, 32), WHAL_SUCCESS);
}

void whal_Test_Dispatch(void)
{
    WHAL_TEST_SUITE_START("dispatch");
    WHAL_TEST(Test_Gpio_NullDev);
    WHAL_TEST(Test_Gpio_NullDriver);
    WHAL_TEST(Test_Gpio_ValidDispatch);
    WHAL_TEST(Test_Uart_NullDev);
    WHAL_TEST(Test_Uart_NullDriver);
    WHAL_TEST(Test_Uart_NullAsyncVtable);
    WHAL_TEST(Test_Uart_ValidDispatch);
    WHAL_TEST(Test_Flash_NullDev);
    WHAL_TEST(Test_Flash_NullDriver);
    WHAL_TEST(Test_Flash_ValidDispatch);
    WHAL_TEST(Test_Timer_NullDev);
    WHAL_TEST(Test_Timer_NullDriver);
    WHAL_TEST(Test_Timer_ValidDispatch);
    WHAL_TEST(Test_Rng_NullDev);
    WHAL_TEST(Test_Rng_NullDriver);
    WHAL_TEST(Test_Rng_ValidDispatch);
    WHAL_TEST(Test_Spi_NullDev);
    WHAL_TEST(Test_Spi_NullDriver);
    WHAL_TEST(Test_Spi_ValidDispatch);
    WHAL_TEST(Test_SpiMem_NullDev);
    WHAL_TEST(Test_SpiMem_NullArgs);
    WHAL_TEST(Test_SpiMem_NullDriver);
    WHAL_TEST(Test_SpiMem_ValidDispatch);
    WHAL_TEST(Test_Block_NullDev);
    WHAL_TEST(Test_Block_NullDriver);
    WHAL_TEST(Test_Block_ValidDispatch);
    WHAL_TEST(Test_Pwm_NullDev);
    WHAL_TEST(Test_Pwm_NullDriver);
    WHAL_TEST(Test_Pwm_BadWave);
    WHAL_TEST(Test_Pwm_ValidDispatch);
    WHAL_TEST(Test_Display_NullDev);
    WHAL_TEST(Test_Display_NullDriver);
    WHAL_TEST(Test_Display_BadArgs);
    WHAL_TEST(Test_Display_ValidDispatch);
    WHAL_TEST(Test_Crypto_NullDev);
    WHAL_TEST(Test_Crypto_NullDriver);
    WHAL_TEST(Test_Crypto_ValidDispatch);
    WHAL_TEST(Test_AesEcb_NullDev);
    WHAL_TEST(Test_AesEcb_NullDriver);
    WHAL_TEST(Test_AesEcb_NoStop);
    WHAL_TEST(Test_AesEcb_ValidDispatch);
    WHAL_TEST(Test_AesCbc_NullDev);
    WHAL_TEST(Test_AesCbc_NullDriver);
    WHAL_TEST(Test_AesCbc_NoStop);
    WHAL_TEST(Test_AesCbc_ValidDispatch);
    WHAL_TEST(Test_AesCtr_NullDev);
    WHAL_TEST(Test_AesCtr_NullDriver);
    WHAL_TEST(Test_AesCtr_NoStop);
    WHAL_TEST(Test_AesCtr_ValidDispatch);
    WHAL_TEST(Test_AesGcm_NullDev);
    WHAL_TEST(Test_AesGcm_NullDriver);
    WHAL_TEST(Test_AesGcm_NoFinalize);
    WHAL_TEST(Test_AesGcm_ValidDispatch);
    WHAL_TEST(Test_AesGmac_NullDev);
    WHAL_TEST(Test_AesGmac_NullDriver);
    WHAL_TEST(Test_AesGmac_NoOneshot);
    WHAL_TEST(Test_AesGmac_ValidDispatch);
    WHAL_TEST(Test_AesCcm_NullDev);
    WHAL_TEST(Test_AesCcm_NullDriver);
    WHAL_TEST(Test_AesCcm_NoFinalize);
    WHAL_TEST(Test_AesCcm_ValidDispatch);
    WHAL_TEST(Test_Sha1_NullDev);
    WHAL_TEST(Test_Sha1_NullDriver);
    WHAL_TEST(Test_Sha1_ValidDispatch);
    WHAL_TEST(Test_Sha224_NullDev);
    WHAL_TEST(Test_Sha224_NullDriver);
    WHAL_TEST(Test_Sha224_ValidDispatch);
    WHAL_TEST(Test_Sha256_NullDev);
    WHAL_TEST(Test_Sha256_NullDriver);
    WHAL_TEST(Test_Sha256_ValidDispatch);
    WHAL_TEST(Test_HmacSha1_NullDev);
    WHAL_TEST(Test_HmacSha1_NullDriver);
    WHAL_TEST(Test_HmacSha1_ValidDispatch);
    WHAL_TEST(Test_HmacSha224_NullDev);
    WHAL_TEST(Test_HmacSha224_NullDriver);
    WHAL_TEST(Test_HmacSha224_ValidDispatch);
    WHAL_TEST(Test_HmacSha256_NullDev);
    WHAL_TEST(Test_HmacSha256_NullDriver);
    WHAL_TEST(Test_HmacSha256_ValidDispatch);
    WHAL_TEST_SUITE_END();
}
