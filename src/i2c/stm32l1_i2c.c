/* stm32l1_i2c.c
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
#ifdef WHAL_CFG_STM32L1_I2C_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides whal_Stm32l1_I2c_Dev singleton */
#endif
#include <wolfHAL/i2c/stm32l1_i2c.h>
#include <wolfHAL/i2c/i2c.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/**
 * STM32L1 I2C (V1) Register Definitions
 *
 * The V1 I2C peripheral uses CR1/CR2/DR/SR1/SR2/CCR/TRISE registers.
 * This driver operates in controller mode with polling, implementing the
 * master transmitter and receiver sequences from RM0038 Section 26.3.3.
 */

/* Register offsets */
#define I2C_CR1   0x00
#define I2C_CR2   0x04
#define I2C_DR    0x10
#define I2C_SR1   0x14
#define I2C_SR2   0x18
#define I2C_CCR   0x1C
#define I2C_TRISE 0x20

/* CR1 bits */
#define CR1_PE_Pos        0
#define CR1_PE_Msk        (1UL << CR1_PE_Pos)
#define CR1_START_Pos     8
#define CR1_START_Msk     (1UL << CR1_START_Pos)
#define CR1_STOP_Pos      9
#define CR1_STOP_Msk      (1UL << CR1_STOP_Pos)
#define CR1_ACK_Pos       10
#define CR1_ACK_Msk       (1UL << CR1_ACK_Pos)
#define CR1_POS_Pos       11
#define CR1_POS_Msk       (1UL << CR1_POS_Pos)
#define CR1_SWRST_Pos     15
#define CR1_SWRST_Msk     (1UL << CR1_SWRST_Pos)

/* CR2 bits */
#define CR2_FREQ_Pos      0
#define CR2_FREQ_Msk      (WHAL_BITMASK(6) << CR2_FREQ_Pos)

/* SR1 bits */
#define SR1_SB_Pos        0
#define SR1_SB_Msk        (1UL << SR1_SB_Pos)
#define SR1_ADDR_Pos      1
#define SR1_ADDR_Msk      (1UL << SR1_ADDR_Pos)
#define SR1_BTF_Pos       2
#define SR1_BTF_Msk       (1UL << SR1_BTF_Pos)
#define SR1_ADD10_Pos     3
#define SR1_ADD10_Msk     (1UL << SR1_ADD10_Pos)
#define SR1_RXNE_Pos      6
#define SR1_RXNE_Msk      (1UL << SR1_RXNE_Pos)
#define SR1_TXE_Pos       7
#define SR1_TXE_Msk       (1UL << SR1_TXE_Pos)
#define SR1_BERR_Pos      8
#define SR1_BERR_Msk      (1UL << SR1_BERR_Pos)
#define SR1_ARLO_Pos      9
#define SR1_ARLO_Msk      (1UL << SR1_ARLO_Pos)
#define SR1_AF_Pos        10
#define SR1_AF_Msk        (1UL << SR1_AF_Pos)

#define SR1_ERR_Msk       (SR1_BERR_Msk | SR1_ARLO_Msk | SR1_AF_Msk)

/* CCR bits */
#define CCR_CCR_Msk       (WHAL_BITMASK(12))
#define CCR_FS_Pos        15
#define CCR_FS_Msk        (1UL << CCR_FS_Pos)

/* TRISE bits */
#define TRISE_Msk         (WHAL_BITMASK(6))

#if defined(WHAL_CFG_STM32L1_I2C_DIRECT_API_MAPPING)
#define whal_Stm32l1_I2c_Init     whal_I2c_Init
#define whal_Stm32l1_I2c_Deinit   whal_I2c_Deinit
#define whal_Stm32l1_I2c_StartCom whal_I2c_StartCom
#define whal_Stm32l1_I2c_EndCom   whal_I2c_EndCom
#define whal_Stm32l1_I2c_Transfer whal_I2c_Transfer
#endif

#ifdef WHAL_CFG_STM32L1_I2C_SINGLE_INSTANCE
const whal_I2c whal_Stm32l1_I2c_Dev = WHAL_CFG_STM32L1_I2C_DEV;
#endif

static whal_Error Stm32l1_I2c_WaitSR1(size_t base, uint32_t flag,
                                       whal_Timeout *timeout)
{
    WHAL_TIMEOUT_START(timeout);

    while (1) {
        uint32_t sr1 = whal_Reg_Read(base, I2C_SR1);

        if (sr1 & flag)
            return WHAL_SUCCESS;

        if (sr1 & SR1_ERR_Msk) {
            whal_Reg_Update(base, I2C_SR1, SR1_ERR_Msk, 0);
            whal_Reg_Update(base, I2C_CR1, CR1_STOP_Msk, CR1_STOP_Msk);
            return WHAL_EHARDWARE;
        }

        if (WHAL_TIMEOUT_EXPIRED(timeout))
            return WHAL_ETIMEOUT;
    }
}

static void Stm32l1_I2c_ClearAddr(size_t base)
{
    (void)whal_Reg_Read(base, I2C_SR1);
    (void)whal_Reg_Read(base, I2C_SR2);
}

static whal_Error Stm32l1_I2c_SendStart(size_t base, uint16_t addr,
                                         uint8_t addrSz, uint8_t isRead,
                                         whal_Timeout *timeout)
{
    whal_Error err;
    uint8_t header;

    whal_Reg_Update(base, I2C_CR1, CR1_START_Msk, CR1_START_Msk);

    err = Stm32l1_I2c_WaitSR1(base, SR1_SB_Msk, timeout);
    if (err)
        return err;

    if (addrSz == 7) {
        whal_Reg_Write(base, I2C_DR,
                       (uint8_t)((addr << 1) | (isRead ? 1 : 0)));

        return Stm32l1_I2c_WaitSR1(base, SR1_ADDR_Msk, timeout);
    }

    /*
     * 10-bit addressing per RM0038 §26.3.3:
     * header byte is 0b11110<A9><A8><R/W>. The master always begins with W,
     * sends the low address byte after ADD10, then (for reads) issues a
     * repeated start with the same header byte plus R=1.
     */
    header = (uint8_t)(0xF0 | (((addr >> 8) & 0x03) << 1));

    whal_Reg_Write(base, I2C_DR, header);

    err = Stm32l1_I2c_WaitSR1(base, SR1_ADD10_Msk, timeout);
    if (err)
        return err;

    whal_Reg_Write(base, I2C_DR, (uint8_t)(addr & 0xFF));

    err = Stm32l1_I2c_WaitSR1(base, SR1_ADDR_Msk, timeout);
    if (err)
        return err;

    if (!isRead)
        return WHAL_SUCCESS;

    Stm32l1_I2c_ClearAddr(base);

    whal_Reg_Update(base, I2C_CR1, CR1_START_Msk, CR1_START_Msk);

    err = Stm32l1_I2c_WaitSR1(base, SR1_SB_Msk, timeout);
    if (err)
        return err;

    whal_Reg_Write(base, I2C_DR, (uint8_t)(header | 0x01));

    return Stm32l1_I2c_WaitSR1(base, SR1_ADDR_Msk, timeout);
}

static whal_Error Stm32l1_I2c_MasterWrite(size_t base, uint8_t *buf, size_t len,
                                           uint8_t stop, whal_Timeout *timeout)
{
    whal_Error err;

    for (size_t i = 0; i < len; i++) {
        err = Stm32l1_I2c_WaitSR1(base, SR1_TXE_Msk, timeout);
        if (err)
            return err;

        whal_Reg_Write(base, I2C_DR, buf[i]);
    }

    err = Stm32l1_I2c_WaitSR1(base, SR1_BTF_Msk, timeout);
    if (err)
        return err;

    if (stop)
        whal_Reg_Update(base, I2C_CR1, CR1_STOP_Msk, CR1_STOP_Msk);

    return WHAL_SUCCESS;
}

/**
 * Master receive with ADDR not yet cleared.
 *
 * Implements the 1-byte, 2-byte, and N>2-byte receive procedures from
 * RM0038 Section 26.3.3, including the ACK/POS manipulation that must
 * happen before the ADDR flag is cleared.
 */
static whal_Error Stm32l1_I2c_MasterRead(size_t base, uint8_t *buf, size_t len,
                                          uint8_t stop, whal_Timeout *timeout)
{
    whal_Error err;

    if (len == 1) {
        whal_Reg_Update(base, I2C_CR1, CR1_ACK_Msk, 0);
        Stm32l1_I2c_ClearAddr(base);

        if (stop)
            whal_Reg_Update(base, I2C_CR1, CR1_STOP_Msk, CR1_STOP_Msk);

        err = Stm32l1_I2c_WaitSR1(base, SR1_RXNE_Msk, timeout);
        if (err)
            return err;

        buf[0] = (uint8_t)whal_Reg_Read(base, I2C_DR);
    }
    else if (len == 2) {
        whal_Reg_Update(base, I2C_CR1, CR1_ACK_Msk | CR1_POS_Msk, CR1_POS_Msk);
        Stm32l1_I2c_ClearAddr(base);

        err = Stm32l1_I2c_WaitSR1(base, SR1_BTF_Msk, timeout);
        if (err)
            return err;

        if (stop)
            whal_Reg_Update(base, I2C_CR1, CR1_STOP_Msk, CR1_STOP_Msk);

        buf[0] = (uint8_t)whal_Reg_Read(base, I2C_DR);
        buf[1] = (uint8_t)whal_Reg_Read(base, I2C_DR);
    }
    else {
        whal_Reg_Update(base, I2C_CR1, CR1_ACK_Msk, CR1_ACK_Msk);
        Stm32l1_I2c_ClearAddr(base);

        for (size_t i = 0; i < len - 3; i++) {
            err = Stm32l1_I2c_WaitSR1(base, SR1_RXNE_Msk, timeout);
            if (err)
                return err;

            buf[i] = (uint8_t)whal_Reg_Read(base, I2C_DR);
        }

        err = Stm32l1_I2c_WaitSR1(base, SR1_BTF_Msk, timeout);
        if (err)
            return err;

        whal_Reg_Update(base, I2C_CR1, CR1_ACK_Msk, 0);
        buf[len - 3] = (uint8_t)whal_Reg_Read(base, I2C_DR);

        err = Stm32l1_I2c_WaitSR1(base, SR1_BTF_Msk, timeout);
        if (err)
            return err;

        if (stop)
            whal_Reg_Update(base, I2C_CR1, CR1_STOP_Msk, CR1_STOP_Msk);

        buf[len - 2] = (uint8_t)whal_Reg_Read(base, I2C_DR);
        buf[len - 1] = (uint8_t)whal_Reg_Read(base, I2C_DR);
    }

    whal_Reg_Update(base, I2C_CR1, CR1_POS_Msk, 0);

    return WHAL_SUCCESS;
}

static uint32_t Stm32l1_I2c_CalcCcr(uint32_t pclk, uint32_t freq)
{
    uint32_t ccr;

    if (freq <= 100000) {
        ccr = pclk / (2 * freq);
        if (ccr < 4)
            ccr = 4;
        return ccr & CCR_CCR_Msk;
    }

    ccr = pclk / (3 * freq);
    if (ccr < 1)
        ccr = 1;
    return CCR_FS_Msk | (ccr & CCR_CCR_Msk);
}

static uint32_t Stm32l1_I2c_CalcTrise(uint32_t pclk, uint32_t freq)
{
    uint32_t pclkMhz = pclk / 1000000;

    if (freq <= 100000)
        return (pclkMhz + 1) & TRISE_Msk;

    return ((pclkMhz * 300 / 1000) + 1) & TRISE_Msk;
}

whal_Error whal_Stm32l1_I2c_Init(whal_I2c *i2cDev)
{
#ifdef WHAL_CFG_STM32L1_I2C_SINGLE_INSTANCE
    size_t base = whal_Stm32l1_I2c_Dev.base;
    (void)i2cDev;
#else
    size_t base;

    if (!i2cDev || !i2cDev->cfg)
        return WHAL_EINVAL;

    base = i2cDev->base;
#endif

    whal_Reg_Update(base, I2C_CR1, CR1_PE_Msk, 0);
    whal_Reg_Update(base, I2C_CR1, CR1_SWRST_Msk, CR1_SWRST_Msk);
    whal_Reg_Update(base, I2C_CR1, CR1_SWRST_Msk, 0);
    whal_Reg_Update(base, I2C_CR1, CR1_PE_Msk, CR1_PE_Msk);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32l1_I2c_Deinit(whal_I2c *i2cDev)
{
#ifdef WHAL_CFG_STM32L1_I2C_SINGLE_INSTANCE
    size_t base = whal_Stm32l1_I2c_Dev.base;
    (void)i2cDev;
#else
    size_t base;

    if (!i2cDev || !i2cDev->cfg)
        return WHAL_EINVAL;

    base = i2cDev->base;
#endif

    whal_Reg_Update(base, I2C_CR1, CR1_PE_Msk, 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32l1_I2c_StartCom(whal_I2c *i2cDev, whal_I2c_ComCfg *comCfg)
{
    uint32_t freqMhz;
#ifdef WHAL_CFG_STM32L1_I2C_SINGLE_INSTANCE
    whal_Stm32l1_I2c_Cfg *cfg =
        (whal_Stm32l1_I2c_Cfg *)whal_Stm32l1_I2c_Dev.cfg;
    size_t base = whal_Stm32l1_I2c_Dev.base;
    (void)i2cDev;

    if (!comCfg)
        return WHAL_EINVAL;
#else
    whal_Stm32l1_I2c_Cfg *cfg;
    size_t base;

    if (!i2cDev || !i2cDev->cfg || !comCfg)
        return WHAL_EINVAL;
#endif

    if ((comCfg->addrSz != 7 && comCfg->addrSz != 10) || comCfg->freq == 0)
        return WHAL_EINVAL;

#ifndef WHAL_CFG_STM32L1_I2C_SINGLE_INSTANCE
    cfg = (whal_Stm32l1_I2c_Cfg *)i2cDev->cfg;
    base = i2cDev->base;
#endif

    whal_Reg_Update(base, I2C_CR1, CR1_PE_Msk, 0);

    freqMhz = cfg->pclk / 1000000;
    whal_Reg_Update(base, I2C_CR2, CR2_FREQ_Msk,
                    whal_SetBits(CR2_FREQ_Msk, CR2_FREQ_Pos, freqMhz));

    whal_Reg_Write(base, I2C_CCR, Stm32l1_I2c_CalcCcr(cfg->pclk, comCfg->freq));
    whal_Reg_Write(base, I2C_TRISE,
                   Stm32l1_I2c_CalcTrise(cfg->pclk, comCfg->freq));

    whal_Reg_Update(base, I2C_CR1, CR1_PE_Msk, CR1_PE_Msk);

    cfg->_addr = comCfg->addr;
    cfg->_addrSz = comCfg->addrSz;

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32l1_I2c_EndCom(whal_I2c *i2cDev)
{
#ifdef WHAL_CFG_STM32L1_I2C_SINGLE_INSTANCE
    whal_Stm32l1_I2c_Cfg *cfg =
        (whal_Stm32l1_I2c_Cfg *)whal_Stm32l1_I2c_Dev.cfg;
    (void)i2cDev;
#else
    whal_Stm32l1_I2c_Cfg *cfg;

    if (!i2cDev || !i2cDev->cfg)
        return WHAL_EINVAL;

    cfg = (whal_Stm32l1_I2c_Cfg *)i2cDev->cfg;
#endif
    cfg->_addr = 0;
    cfg->_addrSz = 0;

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32l1_I2c_Transfer(whal_I2c *i2cDev, whal_I2c_Msg *msgs,
                                     size_t numMsgs)
{
    whal_Error err;
#ifdef WHAL_CFG_STM32L1_I2C_SINGLE_INSTANCE
    whal_Stm32l1_I2c_Cfg *cfg =
        (whal_Stm32l1_I2c_Cfg *)whal_Stm32l1_I2c_Dev.cfg;
    size_t base = whal_Stm32l1_I2c_Dev.base;
    (void)i2cDev;

    if (!msgs || numMsgs == 0)
        return WHAL_EINVAL;
#else
    whal_Stm32l1_I2c_Cfg *cfg;
    size_t base;

    if (!i2cDev || !i2cDev->cfg || !msgs || numMsgs == 0)
        return WHAL_EINVAL;

    cfg = (whal_Stm32l1_I2c_Cfg *)i2cDev->cfg;
    base = i2cDev->base;
#endif

    for (size_t i = 0; i < numMsgs; i++) {
        uint8_t isRead = (msgs[i].flags & WHAL_I2C_MSG_READ) ? 1 : 0;
        uint8_t doStart = (msgs[i].flags & WHAL_I2C_MSG_START) ? 1 : 0;
        uint8_t doStop = (msgs[i].flags & WHAL_I2C_MSG_STOP) ? 1 : 0;
        uint8_t *buf = (uint8_t *)msgs[i].data;
        size_t len = msgs[i].dataSz;

        if (len == 0)
            return WHAL_EINVAL;

        if (doStart) {
            err = Stm32l1_I2c_SendStart(base, cfg->_addr, cfg->_addrSz,
                                         isRead, cfg->timeout);
            if (err)
                return err;
        }

        if (isRead) {
            err = Stm32l1_I2c_MasterRead(base, buf, len, doStop, cfg->timeout);
        }
        else {
            if (doStart)
                Stm32l1_I2c_ClearAddr(base);

            err = Stm32l1_I2c_MasterWrite(base, buf, len, doStop, cfg->timeout);
        }

        if (err)
            return err;
    }

    return WHAL_SUCCESS;
}

#if !defined(WHAL_CFG_STM32L1_I2C_DIRECT_API_MAPPING)
const whal_I2cDriver whal_Stm32l1_I2c_Driver = {
    .Init = whal_Stm32l1_I2c_Init,
    .Deinit = whal_Stm32l1_I2c_Deinit,
    .StartCom = whal_Stm32l1_I2c_StartCom,
    .EndCom = whal_Stm32l1_I2c_EndCom,
    .Transfer = whal_Stm32l1_I2c_Transfer,
};
#endif /* !WHAL_CFG_STM32L1_I2C_DIRECT_API_MAPPING */
