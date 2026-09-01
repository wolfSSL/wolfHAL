/* stm32wb_i2c.c
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
#ifdef WHAL_CFG_STM32WB_I2C_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides whal_Stm32wb_I2c_Dev singleton (possibly via platform alias macro) */
#endif
#include <wolfHAL/i2c/stm32wb_i2c.h>
#include <wolfHAL/i2c/i2c.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/*
 * STM32WB I2C Register Definitions
 *
 * The I2C peripheral provides controller and target mode communication.
 * This driver operates in controller mode with polling, using software
 * end mode (AUTOEND=0) to support multi-message transfers with repeated
 * START conditions.
 */

/* Control Register 1 */
#define I2C_CR1_REG  0x00
#define I2C_CR1_PE_Pos       0                                          /* Peripheral enable */
#define I2C_CR1_PE_Msk       (1UL << I2C_CR1_PE_Pos)

#define I2C_CR1_ANFOFF_Pos   12                                         /* Analog noise filter OFF */
#define I2C_CR1_ANFOFF_Msk   (1UL << I2C_CR1_ANFOFF_Pos)

#define I2C_CR1_DNF_Pos      8                                          /* Digital noise filter */
#define I2C_CR1_DNF_Msk      (WHAL_BITMASK(4) << I2C_CR1_DNF_Pos)

/* Control Register 2 */
#define I2C_CR2_REG  0x04
#define I2C_CR2_SADD_Pos     0                                          /* Target address */
#define I2C_CR2_SADD_Msk     (WHAL_BITMASK(10) << I2C_CR2_SADD_Pos)

#define I2C_CR2_RD_WRN_Pos   10                                         /* Transfer direction */
#define I2C_CR2_RD_WRN_Msk   (1UL << I2C_CR2_RD_WRN_Pos)

#define I2C_CR2_ADD10_Pos    11                                          /* 10-bit addressing mode */
#define I2C_CR2_ADD10_Msk    (1UL << I2C_CR2_ADD10_Pos)

#define I2C_CR2_START_Pos    13                                          /* START generation */
#define I2C_CR2_START_Msk    (1UL << I2C_CR2_START_Pos)

#define I2C_CR2_STOP_Pos     14                                          /* STOP generation */
#define I2C_CR2_STOP_Msk     (1UL << I2C_CR2_STOP_Pos)

#define I2C_CR2_NBYTES_Pos   16                                          /* Number of bytes */
#define I2C_CR2_NBYTES_Msk   (WHAL_BITMASK(8) << I2C_CR2_NBYTES_Pos)

#define I2C_CR2_RELOAD_Pos   24                                          /* NBYTES reload mode */
#define I2C_CR2_RELOAD_Msk   (1UL << I2C_CR2_RELOAD_Pos)

#define I2C_CR2_AUTOEND_Pos  25                                          /* Automatic end mode */
#define I2C_CR2_AUTOEND_Msk  (1UL << I2C_CR2_AUTOEND_Pos)

/* Timing Register */
#define I2C_TIMINGR_REG  0x10

/* Interrupt and Status Register */
#define I2C_ISR_REG  0x18
#define I2C_ISR_TXE_Pos      0                                           /* Transmit data register empty */
#define I2C_ISR_TXE_Msk      (1UL << I2C_ISR_TXE_Pos)

#define I2C_ISR_TXIS_Pos     1                                           /* Transmit interrupt status */
#define I2C_ISR_TXIS_Msk     (1UL << I2C_ISR_TXIS_Pos)

#define I2C_ISR_RXNE_Pos     2                                           /* Receive data register not empty */
#define I2C_ISR_RXNE_Msk     (1UL << I2C_ISR_RXNE_Pos)

#define I2C_ISR_NACKF_Pos    4                                           /* Not acknowledge received */
#define I2C_ISR_NACKF_Msk    (1UL << I2C_ISR_NACKF_Pos)

#define I2C_ISR_STOPF_Pos    5                                           /* STOP detection */
#define I2C_ISR_STOPF_Msk    (1UL << I2C_ISR_STOPF_Pos)

#define I2C_ISR_TC_Pos       6                                           /* Transfer complete */
#define I2C_ISR_TC_Msk       (1UL << I2C_ISR_TC_Pos)

#define I2C_ISR_TCR_Pos      7                                           /* Transfer complete reload */
#define I2C_ISR_TCR_Msk      (1UL << I2C_ISR_TCR_Pos)

#define I2C_ISR_BERR_Pos     8                                           /* Bus error */
#define I2C_ISR_BERR_Msk     (1UL << I2C_ISR_BERR_Pos)

#define I2C_ISR_ARLO_Pos     9                                           /* Arbitration lost */
#define I2C_ISR_ARLO_Msk     (1UL << I2C_ISR_ARLO_Pos)

#define I2C_ISR_BUSY_Pos     15                                          /* Bus busy */
#define I2C_ISR_BUSY_Msk     (1UL << I2C_ISR_BUSY_Pos)

/* Interrupt Clear Register */
#define I2C_ICR_REG  0x1C
#define I2C_ICR_NACKCF_Pos   4                                           /* NACK clear flag */
#define I2C_ICR_NACKCF_Msk   (1UL << I2C_ICR_NACKCF_Pos)

#define I2C_ICR_STOPCF_Pos   5                                           /* STOP clear flag */
#define I2C_ICR_STOPCF_Msk   (1UL << I2C_ICR_STOPCF_Pos)

#define I2C_ICR_BERRCF_Pos   8                                           /* Bus error clear flag */
#define I2C_ICR_BERRCF_Msk   (1UL << I2C_ICR_BERRCF_Pos)

#define I2C_ICR_ARLOCF_Pos   9                                           /* Arbitration lost clear flag */
#define I2C_ICR_ARLOCF_Msk   (1UL << I2C_ICR_ARLOCF_Pos)

/* Receive Data Register */
#define I2C_RXDR_REG  0x24

/* Transmit Data Register */
#define I2C_TXDR_REG  0x28

/* I2C_TIMINGR bit positions */
#define I2C_TIMINGR_SCLL_Pos   0
#define I2C_TIMINGR_SCLH_Pos   8
#define I2C_TIMINGR_SDADEL_Pos 16
#define I2C_TIMINGR_SCLDEL_Pos 20
#define I2C_TIMINGR_PRESC_Pos  28

/* I2C spec minimum SCL timing values in nanoseconds */
#define I2C_SM_TLOW_NS    4700   /* Standard mode tLOW min */
#define I2C_SM_THIGH_NS   4000   /* Standard mode tHIGH min */
#define I2C_FM_TLOW_NS    1300   /* Fast mode tLOW min */
#define I2C_FM_THIGH_NS    600   /* Fast mode tHIGH min */
#define I2C_FMP_TLOW_NS    500   /* Fast mode plus tLOW min */
#define I2C_FMP_THIGH_NS   260   /* Fast mode plus tHIGH min */

#ifdef WHAL_CFG_STM32WB_I2C_DIRECT_API_MAPPING
#define whal_Stm32wb_I2c_Init     whal_I2c_Init
#define whal_Stm32wb_I2c_Deinit   whal_I2c_Deinit
#define whal_Stm32wb_I2c_StartCom whal_I2c_StartCom
#define whal_Stm32wb_I2c_EndCom   whal_I2c_EndCom
#define whal_Stm32wb_I2c_Transfer whal_I2c_Transfer
#endif /* WHAL_CFG_I2C_API_MAPPING */

#ifdef WHAL_CFG_STM32WB_I2C_SINGLE_INSTANCE
const whal_I2c whal_Stm32wb_I2c_Dev = WHAL_CFG_STM32WB_I2C_DEV;
#endif

static uint32_t Stm32wb_I2c_CalcTimingr(uint32_t pclk, uint32_t freq)
{
    uint32_t tLowNs, tHighNs;
    uint32_t presc, scll, sclh, sdadel, scldel;
    uint32_t tPrescNs;

    if (freq <= 100000) {
        tLowNs  = I2C_SM_TLOW_NS;
        tHighNs = I2C_SM_THIGH_NS;
        sdadel  = 2;
        scldel  = 4;
    } else if (freq <= 400000) {
        tLowNs  = I2C_FM_TLOW_NS;
        tHighNs = I2C_FM_THIGH_NS;
        sdadel  = 1;
        scldel  = 3;
    } else {
        tLowNs  = I2C_FMP_TLOW_NS;
        tHighNs = I2C_FMP_THIGH_NS;
        sdadel  = 0;
        scldel  = 1;
    }

    for (presc = 0; presc < 16; presc++) {
        tPrescNs = ((presc + 1) * 1000) / (pclk / 1000000);
        if (tPrescNs == 0)
            continue;

        scll = tLowNs / tPrescNs;
        sclh = tHighNs / tPrescNs;

        if (scll > 0) scll--;
        if (sclh > 0) sclh--;

        if (scll <= 255 && sclh <= 255)
            break;
    }

    if (presc >= 16) {
        presc = 15;
        scll = 255;
        sclh = 255;
    }

    return (presc  << I2C_TIMINGR_PRESC_Pos)  |
           (scldel << I2C_TIMINGR_SCLDEL_Pos) |
           (sdadel << I2C_TIMINGR_SDADEL_Pos) |
           (sclh   << I2C_TIMINGR_SCLH_Pos)   |
           (scll   << I2C_TIMINGR_SCLL_Pos);
}

static whal_Error Stm32wb_I2c_CheckErrors(size_t base)
{
    size_t isr = whal_Reg_Read(base, I2C_ISR_REG);

    if (isr & I2C_ISR_NACKF_Msk) {
        whal_Reg_Write(base, I2C_ICR_REG, I2C_ICR_NACKCF_Msk);
        /* Generate STOP after NACK */
        whal_Reg_Update(base, I2C_CR2_REG, I2C_CR2_STOP_Msk,
                        whal_SetBits(I2C_CR2_STOP_Msk, I2C_CR2_STOP_Pos, 1));
        return WHAL_EHARDWARE;
    }

    if (isr & I2C_ISR_BERR_Msk) {
        whal_Reg_Write(base, I2C_ICR_REG, I2C_ICR_BERRCF_Msk);
        return WHAL_EHARDWARE;
    }

    if (isr & I2C_ISR_ARLO_Msk) {
        whal_Reg_Write(base, I2C_ICR_REG, I2C_ICR_ARLOCF_Msk);
        return WHAL_EHARDWARE;
    }

    return WHAL_SUCCESS;
}

static whal_Error Stm32wb_I2c_WaitFlag(size_t base, size_t mask, size_t value,
                                       whal_Timeout *timeout)
{
    whal_Error err;

    WHAL_TIMEOUT_START(timeout);

    while (1) {
        size_t isr = whal_Reg_Read(base, I2C_ISR_REG);

        if ((isr & mask) == value)
            return WHAL_SUCCESS;

        err = Stm32wb_I2c_CheckErrors(base);
        if (err)
            return err;

        if (WHAL_TIMEOUT_EXPIRED(timeout))
            return WHAL_ETIMEOUT;
    }
}

/*
 * Transfer up to 255 bytes in a single hardware operation.
 * Caller must ensure nbytes <= 255.
 */
/* Transfer control bits mask — everything TransferChunk touches in CR2 */
#define I2C_CR2_XFER_Msk  (I2C_CR2_RD_WRN_Msk | I2C_CR2_NBYTES_Msk | \
                            I2C_CR2_START_Msk | I2C_CR2_RELOAD_Msk)

static whal_Error Stm32wb_I2c_TransferChunk(size_t base, uint8_t *buf,
                                            size_t nbytes, uint8_t isRead,
                                            uint8_t start, uint8_t reload,
                                            whal_Timeout *timeout)
{
    uint32_t cr2 = 0;
    whal_Error err;

    cr2 |= whal_SetBits(I2C_CR2_RD_WRN_Msk, I2C_CR2_RD_WRN_Pos, isRead);
    cr2 |= whal_SetBits(I2C_CR2_NBYTES_Msk, I2C_CR2_NBYTES_Pos, nbytes);

    if (start)
        cr2 |= whal_SetBits(I2C_CR2_START_Msk, I2C_CR2_START_Pos, 1);

    if (reload)
        cr2 |= whal_SetBits(I2C_CR2_RELOAD_Msk, I2C_CR2_RELOAD_Pos, 1);

    /* Update only transfer control bits, preserving SADD + ADD10 from StartCom */
    whal_Reg_Update(base, I2C_CR2_REG, I2C_CR2_XFER_Msk, cr2);

    for (size_t i = 0; i < nbytes; i++) {
        if (isRead) {
            err = Stm32wb_I2c_WaitFlag(base, I2C_ISR_RXNE_Msk,
                                       I2C_ISR_RXNE_Msk, timeout);
            if (err)
                return err;

            if (buf)
                buf[i] = (uint8_t)whal_Reg_Read(base, I2C_RXDR_REG);
            else
                (void)whal_Reg_Read(base, I2C_RXDR_REG);
        } else {
            err = Stm32wb_I2c_WaitFlag(base, I2C_ISR_TXIS_Msk,
                                       I2C_ISR_TXIS_Msk, timeout);
            if (err)
                return err;

            whal_Reg_Write(base, I2C_TXDR_REG, buf ? buf[i] : 0xFF);
        }
    }

    if (reload) {
        /* Wait for TCR before next chunk */
        err = Stm32wb_I2c_WaitFlag(base, I2C_ISR_TCR_Msk,
                                   I2C_ISR_TCR_Msk, timeout);
        if (err)
            return err;
    }

    return WHAL_SUCCESS;
}

/*
 * Transfer a single I2C message on the bus.
 *
 * Handles messages of any size by splitting into <=255 byte chunks using
 * the hardware RELOAD mechanism. The SADD and ADD10 bits in CR2 are
 * configured by StartCom and preserved across writes.
 *
 * The caller controls bus conditions explicitly via message flags:
 *   WHAL_I2C_MSG_START — generate START (or repeated START)
 *   WHAL_I2C_MSG_STOP  — generate STOP after this message
 *   WHAL_I2C_MSG_READ  — master read
 *   WHAL_I2C_MSG_WRITE — master write
 *
 * When STOP is not set, the function waits for TC (transfer complete) which
 * stretches SCL low, holding the bus for the next message's START.
 */
/*
 * @param reloadLast  If true, use RELOAD on the last chunk so the next
 *                    message can continue the same transfer without
 *                    re-addressing. If false, use TC so the next message
 *                    can issue START for a direction change.
 */
static whal_Error Stm32wb_I2c_TransferMsg(size_t base, whal_I2c_Msg *msg,
                                          uint8_t reloadLast,
                                          whal_Timeout *timeout)
{
    uint8_t *buf = (uint8_t *)msg->data;
    uint8_t isRead = (msg->flags & WHAL_I2C_MSG_READ) ? 1 : 0;
    uint8_t doStart = (msg->flags & WHAL_I2C_MSG_START) ? 1 : 0;
    uint8_t doStop = (msg->flags & WHAL_I2C_MSG_STOP) ? 1 : 0;
    size_t remaining = msg->dataSz;
    whal_Error err;

    if (msg->dataSz == 0)
        return WHAL_EINVAL;

    /* Split into <=255 byte RELOAD chunks */
    while (remaining > 255) {
        err = Stm32wb_I2c_TransferChunk(base, buf, 255, isRead,
                                        doStart, 1, timeout);
        if (err)
            return err;

        doStart = 0;
        if (buf)
            buf += 255;
        remaining -= 255;
    }

    /* Last chunk: RELOAD if the next message continues the same transfer
     * (no START), otherwise no RELOAD so hardware sets TC.
     * Note: reloadLast and doStop are mutually exclusive — the caller
     * (Transfer) never sets reloadLast when the message has STOP. */
    err = Stm32wb_I2c_TransferChunk(base, buf, remaining, isRead,
                                    doStart, reloadLast, timeout);
    if (err)
        return err;

    if (doStop) {
        /* Wait for TC, then issue STOP */
        err = Stm32wb_I2c_WaitFlag(base, I2C_ISR_TC_Msk,
                                   I2C_ISR_TC_Msk, timeout);
        if (err)
            return err;

        whal_Reg_Update(base, I2C_CR2_REG, I2C_CR2_STOP_Msk,
                        whal_SetBits(I2C_CR2_STOP_Msk, I2C_CR2_STOP_Pos, 1));

        err = Stm32wb_I2c_WaitFlag(base, I2C_ISR_STOPF_Msk,
                                   I2C_ISR_STOPF_Msk, timeout);
        if (err)
            return err;

        whal_Reg_Write(base, I2C_ICR_REG, I2C_ICR_STOPCF_Msk);
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_I2c_Init(whal_I2c *i2cDev)
{
#ifdef WHAL_CFG_STM32WB_I2C_SINGLE_INSTANCE
    size_t base = whal_Stm32wb_I2c_Dev.base;
    (void)i2cDev;
#else
    size_t base;

    if (!i2cDev || !i2cDev->cfg) {
        return WHAL_EINVAL;
    }

    base = i2cDev->base;
#endif

    /* Disable PE before configuring */
    whal_Reg_Update(base, I2C_CR1_REG, I2C_CR1_PE_Msk,
                    whal_SetBits(I2C_CR1_PE_Msk, I2C_CR1_PE_Pos, 0));

    /* Enable analog noise filter (ANFOFF=0), disable digital filter (DNF=0) */
    whal_Reg_Update(base, I2C_CR1_REG,
                    I2C_CR1_ANFOFF_Msk | I2C_CR1_DNF_Msk,
                    whal_SetBits(I2C_CR1_ANFOFF_Msk, I2C_CR1_ANFOFF_Pos, 0) |
                    whal_SetBits(I2C_CR1_DNF_Msk, I2C_CR1_DNF_Pos, 0));

    /* Enable peripheral */
    whal_Reg_Update(base, I2C_CR1_REG, I2C_CR1_PE_Msk,
                    whal_SetBits(I2C_CR1_PE_Msk, I2C_CR1_PE_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_I2c_Deinit(whal_I2c *i2cDev)
{
#ifdef WHAL_CFG_STM32WB_I2C_SINGLE_INSTANCE
    size_t base = whal_Stm32wb_I2c_Dev.base;
    (void)i2cDev;
#else
    size_t base;

    if (!i2cDev || !i2cDev->cfg) {
        return WHAL_EINVAL;
    }

    base = i2cDev->base;
#endif

    /* Disable peripheral */
    whal_Reg_Update(base, I2C_CR1_REG, I2C_CR1_PE_Msk,
                    whal_SetBits(I2C_CR1_PE_Msk, I2C_CR1_PE_Pos, 0));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_I2c_StartCom(whal_I2c *i2cDev, whal_I2c_ComCfg *comCfg)
{
    uint32_t cr2 = 0;
#ifdef WHAL_CFG_STM32WB_I2C_SINGLE_INSTANCE
    whal_Stm32wb_I2c_Cfg *cfg =
        (whal_Stm32wb_I2c_Cfg *)whal_Stm32wb_I2c_Dev.cfg;
    size_t base = whal_Stm32wb_I2c_Dev.base;
    (void)i2cDev;

    if (!comCfg) {
        return WHAL_EINVAL;
    }
#else
    size_t base;
    whal_Stm32wb_I2c_Cfg *cfg;

    if (!i2cDev || !i2cDev->cfg || !comCfg) {
        return WHAL_EINVAL;
    }
#endif

    if ((comCfg->addrSz != 7 && comCfg->addrSz != 10) || comCfg->freq == 0) {
        return WHAL_EINVAL;
    }

#ifndef WHAL_CFG_STM32WB_I2C_SINGLE_INSTANCE
    base = i2cDev->base;
    cfg = (whal_Stm32wb_I2c_Cfg *)i2cDev->cfg;
#endif

    /* Disable PE to configure timing */
    whal_Reg_Update(base, I2C_CR1_REG, I2C_CR1_PE_Msk,
                    whal_SetBits(I2C_CR1_PE_Msk, I2C_CR1_PE_Pos, 0));

    /* Compute and write timing register */
    whal_Reg_Write(base, I2C_TIMINGR_REG,
                   Stm32wb_I2c_CalcTimingr(cfg->pclk, comCfg->freq));

    /* Re-enable PE */
    whal_Reg_Update(base, I2C_CR1_REG, I2C_CR1_PE_Msk,
                    whal_SetBits(I2C_CR1_PE_Msk, I2C_CR1_PE_Pos, 1));

    /* Configure target address and addressing mode in CR2 */
    if (comCfg->addrSz == 10) {
        cr2 |= whal_SetBits(I2C_CR2_ADD10_Msk, I2C_CR2_ADD10_Pos, 1);
        cr2 |= whal_SetBits(I2C_CR2_SADD_Msk, I2C_CR2_SADD_Pos,
                             comCfg->addr);
    } else {
        cr2 |= whal_SetBits(I2C_CR2_SADD_Msk, I2C_CR2_SADD_Pos,
                             (uint32_t)comCfg->addr << 1);
    }

    whal_Reg_Update(base, I2C_CR2_REG,
                    I2C_CR2_SADD_Msk | I2C_CR2_ADD10_Msk, cr2);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_I2c_EndCom(whal_I2c *i2cDev)
{
#ifdef WHAL_CFG_STM32WB_I2C_SINGLE_INSTANCE
    size_t base = whal_Stm32wb_I2c_Dev.base;
    (void)i2cDev;
#else
    size_t base;

    if (!i2cDev || !i2cDev->cfg) {
        return WHAL_EINVAL;
    }

    base = i2cDev->base;
#endif

    /* Clear target address and addressing mode */
    whal_Reg_Update(base, I2C_CR2_REG,
                    I2C_CR2_SADD_Msk | I2C_CR2_ADD10_Msk, 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_I2c_Transfer(whal_I2c *i2cDev, whal_I2c_Msg *msgs,
                                     size_t numMsgs)
{
    whal_Error err;
#ifdef WHAL_CFG_STM32WB_I2C_SINGLE_INSTANCE
    whal_Stm32wb_I2c_Cfg *cfg =
        (whal_Stm32wb_I2c_Cfg *)whal_Stm32wb_I2c_Dev.cfg;
    size_t base = whal_Stm32wb_I2c_Dev.base;
    (void)i2cDev;

    if (!msgs || numMsgs == 0) {
        return WHAL_EINVAL;
    }
#else
    size_t base;
    whal_Stm32wb_I2c_Cfg *cfg;

    if (!i2cDev || !i2cDev->cfg || !msgs || numMsgs == 0) {
        return WHAL_EINVAL;
    }

    base = i2cDev->base;
    cfg = (whal_Stm32wb_I2c_Cfg *)i2cDev->cfg;
#endif

    for (size_t i = 0; i < numMsgs; i++) {
        /*
         * Use RELOAD on the last chunk if the current message has no STOP
         * and the next message has no START (i.e. it continues the same
         * transfer without re-addressing). Otherwise use TC so the next
         * message's START can generate a repeated START.
         */
        uint8_t reloadLast = 0;
        if (!(msgs[i].flags & WHAL_I2C_MSG_STOP) && i + 1 < numMsgs &&
            !(msgs[i + 1].flags & WHAL_I2C_MSG_START)) {
            reloadLast = 1;
        }

        err = Stm32wb_I2c_TransferMsg(base, &msgs[i], reloadLast,
                                      cfg->timeout);
        if (err)
            return err;
    }

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_STM32WB_I2C_DIRECT_API_MAPPING
const whal_I2cDriver whal_Stm32wb_I2c_Driver = {
    .Init = whal_Stm32wb_I2c_Init,
    .Deinit = whal_Stm32wb_I2c_Deinit,
    .StartCom = whal_Stm32wb_I2c_StartCom,
    .EndCom = whal_Stm32wb_I2c_EndCom,
    .Transfer = whal_Stm32wb_I2c_Transfer,
};
#endif /* !WHAL_CFG_I2C_API_MAPPING */
