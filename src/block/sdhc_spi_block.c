/* sdhc_spi_block.c
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
#ifdef WHAL_CFG_SDHC_SPI_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides whal_SdhcSpi_Dev singleton */
#endif
#include <wolfHAL/block/sdhc_spi_block.h>
#include <wolfHAL/block/block.h>
#include <wolfHAL/spi/spi.h>
#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/error.h>
#include <wolfHAL/timeout.h>

/* SD SPI Commands */
#define CMD0   0x40  /* GO_IDLE_STATE */
#define CMD8   0x48  /* SEND_IF_COND */
#define CMD12  0x4C  /* STOP_TRANSMISSION */
#define CMD17  0x51  /* READ_SINGLE_BLOCK */
#define CMD18  0x52  /* READ_MULTIPLE_BLOCK */
#define CMD24  0x58  /* WRITE_BLOCK */
#define CMD25  0x59  /* WRITE_MULTIPLE_BLOCK */
#define CMD32  0x60  /* ERASE_WR_BLK_START */
#define CMD33  0x61  /* ERASE_WR_BLK_END */
#define CMD38  0x66  /* ERASE */
#define CMD55  0x77  /* APP_CMD */
#define CMD58  0x7A  /* READ_OCR */
#define ACMD41 0x69  /* SD_SEND_OP_COND */

/* R1 Response Bits */
#define R1_IDLE    0x01
#define R1_ERRORS  0x7C

/* Data Tokens */
#define TOKEN_START_BLOCK       0xFE
#define TOKEN_START_BLOCK_MULTI 0xFC
#define TOKEN_STOP_TRAN         0xFD

/* Data Response */
#define DATA_RESP_MASK     0x1F
#define DATA_RESP_ACCEPTED 0x05

#define DUMMY 0xFF

#ifdef WHAL_CFG_SDHC_SPI_BLOCK_DIRECT_API_MAPPING
#define whal_SdhcSpi_Init   whal_Block_Init
#define whal_SdhcSpi_Deinit whal_Block_Deinit
#define whal_SdhcSpi_Read   whal_Block_Read
#define whal_SdhcSpi_Write  whal_Block_Write
#define whal_SdhcSpi_Erase  whal_Block_Erase
#endif /* WHAL_CFG_SDHC_SPI_BLOCK_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_SDHC_SPI_SINGLE_INSTANCE
const whal_Block whal_SdhcSpi_Dev = WHAL_CFG_SDHC_SPI_DEV;
#endif

static whal_Error SdhcSpi_CsAssert(whal_SdhcSpi_Cfg *cfg)
{
    uint8_t dummy = DUMMY;
    whal_Error err;

    err = whal_Gpio_Set(cfg->gpioDev, cfg->csPin, 0);
    if (err)
        return err;

    /* Dummy byte gives card time to recognize CS transition */
    return whal_Spi_SendRecv(cfg->spiDev, &dummy, 1, NULL, 0);
}

static whal_Error SdhcSpi_CsDeassert(whal_SdhcSpi_Cfg *cfg)
{
    return whal_Gpio_Set(cfg->gpioDev, cfg->csPin, 1);
}

/* Poll for a response byte (up to 8 attempts, skips 0xFF) */
static whal_Error SdhcSpi_RecvResp(whal_SdhcSpi_Cfg *cfg, uint8_t *resp)
{
    whal_Error err;
    int i;
    uint8_t tmp;

    for (i = 0; i < 8; i++) {
        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, &tmp, 1);
        if (err)
            return err;
        if (tmp != DUMMY) {
            *resp = tmp;
            return WHAL_SUCCESS;
        }
    }

    return WHAL_ETIMEOUT;
}

/* Send a 6-byte command frame */
static whal_Error SdhcSpi_SendCmd(whal_SdhcSpi_Cfg *cfg,
                                   uint8_t cmd, uint32_t arg,
                                   uint8_t crc)
{
    uint8_t frame[6];

    frame[0] = cmd;
    frame[1] = (uint8_t)(arg >> 24);
    frame[2] = (uint8_t)(arg >> 16);
    frame[3] = (uint8_t)(arg >> 8);
    frame[4] = (uint8_t)(arg);
    frame[5] = crc;

    return whal_Spi_SendRecv(cfg->spiDev, frame, 6, NULL, 0);
}

/* Poll until card is no longer busy (MISO goes high) */
static whal_Error SdhcSpi_WaitReady(whal_SdhcSpi_Cfg *cfg)
{
    uint8_t resp;
    whal_Error err;

    WHAL_TIMEOUT_START(cfg->timeout);
    do {
        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, &resp, 1);
        if (err)
            return err;
        if (resp == DUMMY)
            return WHAL_SUCCESS;
        if (WHAL_TIMEOUT_EXPIRED(cfg->timeout))
            return WHAL_ETIMEOUT;
    } while (1);
}

/* Poll for data start token before a read */
static whal_Error SdhcSpi_WaitDataToken(whal_SdhcSpi_Cfg *cfg, uint8_t *token)
{
    uint8_t resp;
    whal_Error err;

    WHAL_TIMEOUT_START(cfg->timeout);
    do {
        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, &resp, 1);
        if (err)
            return err;
        if (resp != DUMMY) {
            *token = resp;
            return WHAL_SUCCESS;
        }
        if (WHAL_TIMEOUT_EXPIRED(cfg->timeout))
            return WHAL_ETIMEOUT;
    } while (1);
}

whal_Error whal_SdhcSpi_Init(whal_Block *blockDev)
{
    whal_SdhcSpi_Cfg *cfg;
    whal_Spi_ComCfg slow;
    whal_Error err;
    uint8_t r1;
    uint8_t buf[4];
    uint8_t dummy = DUMMY;
    int i;

#ifdef WHAL_CFG_SDHC_SPI_SINGLE_INSTANCE
    cfg = (whal_SdhcSpi_Cfg *)whal_SdhcSpi_Dev.cfg;
    (void)blockDev;
#else
    if (!blockDev || !blockDev->cfg)
        return WHAL_EINVAL;

    cfg = (whal_SdhcSpi_Cfg *)blockDev->cfg;
#endif

    if (!cfg->spiDev || !cfg->spiComCfg || !cfg->gpioDev)
        return WHAL_EINVAL;

    slow = *cfg->spiComCfg;
    slow.freq = 400000;

    /* Power-up: CS high, 80+ clock pulses */
    err = SdhcSpi_CsDeassert(cfg);
    if (err)
        return err;
    err = whal_Spi_StartCom(cfg->spiDev, &slow);
    if (err)
        return err;
    for (i = 0; i < 10; i++) {
        err = whal_Spi_SendRecv(cfg->spiDev, &dummy, 1, NULL, 0);
        if (err)
            goto cleanup;
    }

    /* CMD0: Enter SPI mode */
    err = SdhcSpi_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = SdhcSpi_SendCmd(cfg, CMD0, 0x00000000, 0x95);
    if (!err)
        err = SdhcSpi_RecvResp(cfg, &r1);
    SdhcSpi_CsDeassert(cfg);
    if (err)
        goto cleanup;
    if ((r1 & R1_ERRORS) || !(r1 & R1_IDLE)) {
        err = WHAL_EHARDWARE;
        goto cleanup;
    }

    /*
     * CMD8: SEND_IF_COND — check for SDv2.
     * Arg 0x000001AA: voltage range 2.7-3.6V (0x01) + check pattern (0xAA).
     * Response is R7: R1 byte followed by 4 trailing bytes:
     *   buf[0]: command version
     *   buf[1]: reserved
     *   buf[2]: accepted voltage (expect 0x01)
     *   buf[3]: check pattern echo (expect 0xAA)
     */
    err = SdhcSpi_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = SdhcSpi_SendCmd(cfg, CMD8, 0x000001AA, 0x87);
    if (!err)
        err = SdhcSpi_RecvResp(cfg, &r1);
    if (!err && (r1 & R1_IDLE))
        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, buf, 4);
    SdhcSpi_CsDeassert(cfg);
    if (err)
        goto cleanup;
    if (!(r1 & R1_IDLE) || buf[2] != 0x01 || buf[3] != 0xAA) {
        err = WHAL_EHARDWARE;
        goto cleanup;
    }

    /* ACMD41: Initialize card */
    WHAL_TIMEOUT_START(cfg->timeout);
    do {
        err = SdhcSpi_CsAssert(cfg);
        if (err)
            goto cleanup;
        err = SdhcSpi_SendCmd(cfg, CMD55, 0x00000000, 0x01);
        if (!err)
            err = SdhcSpi_RecvResp(cfg, &r1);
        SdhcSpi_CsDeassert(cfg);
        if (err)
            goto cleanup;

        err = SdhcSpi_CsAssert(cfg);
        if (err)
            goto cleanup;
        err = SdhcSpi_SendCmd(cfg, ACMD41, 0x40000000, 0x01);
        if (!err)
            err = SdhcSpi_RecvResp(cfg, &r1);
        SdhcSpi_CsDeassert(cfg);
        if (err)
            goto cleanup;

        if (r1 == 0x00)
            break;
        if (WHAL_TIMEOUT_EXPIRED(cfg->timeout)) {
            err = WHAL_ETIMEOUT;
            goto cleanup;
        }
    } while (1);

    /* CMD58: Read OCR, verify SDHC */
    err = SdhcSpi_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = SdhcSpi_SendCmd(cfg, CMD58, 0x00000000, 0x01);
    if (!err)
        err = SdhcSpi_RecvResp(cfg, &r1);
    if (!err && r1 == 0x00)
        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, buf, 4);
    SdhcSpi_CsDeassert(cfg);
    if (err)
        goto cleanup;
    if (r1 != 0x00 || !(buf[0] & 0x40)) {
        err = WHAL_EHARDWARE;
        goto cleanup;
    }

cleanup:
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SdhcSpi_Deinit(whal_Block *blockDev)
{
    whal_SdhcSpi_Cfg *cfg;

#ifdef WHAL_CFG_SDHC_SPI_SINGLE_INSTANCE
    cfg = (whal_SdhcSpi_Cfg *)whal_SdhcSpi_Dev.cfg;
    (void)blockDev;
#else
    if (!blockDev || !blockDev->cfg)
        return WHAL_EINVAL;

    cfg = (whal_SdhcSpi_Cfg *)blockDev->cfg;
#endif

    SdhcSpi_CsDeassert(cfg);
    return WHAL_SUCCESS;
}

whal_Error whal_SdhcSpi_Read(whal_Block *blockDev, uint32_t block,
                              void *data, uint32_t blockCount)
{
    uint8_t *dataBuf = (uint8_t *)data;
    whal_SdhcSpi_Cfg *cfg;
    whal_Error err;
    uint8_t r1;
    uint8_t token;
    uint8_t crc[2];
    uint8_t dummy = DUMMY;
    uint32_t i;

#ifdef WHAL_CFG_SDHC_SPI_SINGLE_INSTANCE
    cfg = (whal_SdhcSpi_Cfg *)whal_SdhcSpi_Dev.cfg;
    (void)blockDev;

    if (!data || blockCount == 0)
        return WHAL_EINVAL;
#else
    if (!blockDev || !blockDev->cfg || !data || blockCount == 0)
        return WHAL_EINVAL;

    cfg = (whal_SdhcSpi_Cfg *)blockDev->cfg;
#endif

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    err = SdhcSpi_CsAssert(cfg);
    if (err)
        goto cleanup;

    /* Send read command */
    err = SdhcSpi_SendCmd(cfg,
                          (blockCount == 1) ? CMD17 : CMD18,
                          block, 0x01);
    if (!err)
        err = SdhcSpi_RecvResp(cfg, &r1);
    if (err || r1 != 0x00) {
        if (!err) err = WHAL_EHARDWARE;
        goto cleanup;
    }

    /* Read blocks */
    for (i = 0; i < blockCount; i++) {
        err = SdhcSpi_WaitDataToken(cfg, &token);
        if (err || token != TOKEN_START_BLOCK) {
            if (!err) err = WHAL_EHARDWARE;
            break;
        }

        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0,
                                dataBuf + (i * WHAL_SDHC_SPI_BLOCK_SZ),
                                WHAL_SDHC_SPI_BLOCK_SZ);
        if (err)
            break;

        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, crc, 2);
        if (err)
            break;
    }

    /* Stop multi-block read */
    if (blockCount > 1) {
        SdhcSpi_SendCmd(cfg, CMD12, 0x00000000, 0x01);
        SdhcSpi_RecvResp(cfg, &r1);
        whal_Spi_SendRecv(cfg->spiDev, &dummy, 1, NULL, 0);
        SdhcSpi_WaitReady(cfg);
    }

cleanup:
    SdhcSpi_CsDeassert(cfg);
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SdhcSpi_Write(whal_Block *blockDev, uint32_t block,
                               const void *data, uint32_t blockCount)
{
    const uint8_t *dataBuf = (const uint8_t *)data;
    whal_SdhcSpi_Cfg *cfg;
    whal_Error err;
    uint8_t r1;
    uint8_t resp;
    uint8_t token;
    uint8_t crc[2] = {DUMMY, DUMMY};
    uint8_t dummy = DUMMY;
    uint32_t i;

#ifdef WHAL_CFG_SDHC_SPI_SINGLE_INSTANCE
    cfg = (whal_SdhcSpi_Cfg *)whal_SdhcSpi_Dev.cfg;
    (void)blockDev;

    if (!data || blockCount == 0)
        return WHAL_EINVAL;
#else
    if (!blockDev || !blockDev->cfg || !data || blockCount == 0)
        return WHAL_EINVAL;

    cfg = (whal_SdhcSpi_Cfg *)blockDev->cfg;
#endif

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    err = SdhcSpi_CsAssert(cfg);
    if (err)
        goto cleanup;

    /* Send write command */
    err = SdhcSpi_SendCmd(cfg,
                          (blockCount == 1) ? CMD24 : CMD25,
                          block, 0x01);
    if (!err)
        err = SdhcSpi_RecvResp(cfg, &r1);
    if (err || r1 != 0x00) {
        if (!err) err = WHAL_EHARDWARE;
        goto cleanup;
    }

    /* Write blocks */
    for (i = 0; i < blockCount; i++) {
        err = whal_Spi_SendRecv(cfg->spiDev, &dummy, 1, NULL, 0);
        if (err)
            break;
        token = (blockCount == 1) ? TOKEN_START_BLOCK : TOKEN_START_BLOCK_MULTI;
        err = whal_Spi_SendRecv(cfg->spiDev, &token, 1, NULL, 0);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev,
                      dataBuf + (i * WHAL_SDHC_SPI_BLOCK_SZ),
                      WHAL_SDHC_SPI_BLOCK_SZ, NULL, 0);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, crc, 2, NULL, 0);
        if (err)
            break;

        err = SdhcSpi_RecvResp(cfg, &resp);
        if (err || (resp & DATA_RESP_MASK) != DATA_RESP_ACCEPTED) {
            if (!err) err = WHAL_EHARDWARE;
            break;
        }

        err = SdhcSpi_WaitReady(cfg);
        if (err)
            break;
    }

    /* Stop multi-block write */
    if (blockCount > 1) {
        token = TOKEN_STOP_TRAN;
        whal_Spi_SendRecv(cfg->spiDev, &token, 1, NULL, 0);
        whal_Spi_SendRecv(cfg->spiDev, &dummy, 1, NULL, 0);
        SdhcSpi_WaitReady(cfg);
    }

cleanup:
    SdhcSpi_CsDeassert(cfg);
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SdhcSpi_Erase(whal_Block *blockDev, uint32_t block,
                               uint32_t blockCount)
{
    whal_SdhcSpi_Cfg *cfg;
    whal_Error err;
    uint8_t r1;
    uint32_t endBlock;

#ifdef WHAL_CFG_SDHC_SPI_SINGLE_INSTANCE
    cfg = (whal_SdhcSpi_Cfg *)whal_SdhcSpi_Dev.cfg;
    (void)blockDev;

    if (blockCount == 0)
        return WHAL_EINVAL;
#else
    if (!blockDev || !blockDev->cfg || blockCount == 0)
        return WHAL_EINVAL;

    cfg = (whal_SdhcSpi_Cfg *)blockDev->cfg;
#endif

    if (blockCount - 1u > UINT32_MAX - block)
        return WHAL_EINVAL;
    endBlock = block + blockCount - 1;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    /* CMD32: Set erase start block */
    err = SdhcSpi_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = SdhcSpi_SendCmd(cfg, CMD32, block, 0x01);
    if (!err)
        err = SdhcSpi_RecvResp(cfg, &r1);
    SdhcSpi_CsDeassert(cfg);
    if (err || r1 != 0x00) {
        if (!err) err = WHAL_EHARDWARE;
        goto cleanup;
    }

    /* CMD33: Set erase end block */
    err = SdhcSpi_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = SdhcSpi_SendCmd(cfg, CMD33, endBlock, 0x01);
    if (!err)
        err = SdhcSpi_RecvResp(cfg, &r1);
    SdhcSpi_CsDeassert(cfg);
    if (err || r1 != 0x00) {
        if (!err) err = WHAL_EHARDWARE;
        goto cleanup;
    }

    /* CMD38: Execute erase */
    err = SdhcSpi_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = SdhcSpi_SendCmd(cfg, CMD38, 0x00000000, 0x01);
    if (!err)
        err = SdhcSpi_RecvResp(cfg, &r1);
    if (!err && r1 == 0x00)
        err = SdhcSpi_WaitReady(cfg);
    SdhcSpi_CsDeassert(cfg);
    if (!err && r1 != 0x00)
        err = WHAL_EHARDWARE;

cleanup:
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

#ifndef WHAL_CFG_SDHC_SPI_BLOCK_DIRECT_API_MAPPING
const whal_BlockDriver whal_SdhcSpi_Driver = {
    .Init = whal_SdhcSpi_Init,
    .Deinit = whal_SdhcSpi_Deinit,
    .Read = whal_SdhcSpi_Read,
    .Write = whal_SdhcSpi_Write,
    .Erase = whal_SdhcSpi_Erase,
};
#endif /* !WHAL_CFG_SDHC_SPI_BLOCK_DIRECT_API_MAPPING */
