/* stm32wba_hash.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32WBA_HASH*_DEV initializers */
#include <wolfHAL/crypto/stm32wba_hash.h>
#include <wolfHAL/crypto/crypto.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/endian.h>

const whal_Crypto     whal_Stm32wba_Hash_Dev       = WHAL_CFG_STM32WBA_HASH_DEV;
const whal_Sha1       whal_Stm32wba_Sha1_Dev       = WHAL_CFG_STM32WBA_HASH_SHA1_DEV;
const whal_Sha224     whal_Stm32wba_Sha224_Dev     = WHAL_CFG_STM32WBA_HASH_SHA224_DEV;
const whal_Sha256     whal_Stm32wba_Sha256_Dev     = WHAL_CFG_STM32WBA_HASH_SHA256_DEV;
const whal_HmacSha1   whal_Stm32wba_HmacSha1_Dev   = WHAL_CFG_STM32WBA_HASH_HMAC_SHA1_DEV;
const whal_HmacSha224 whal_Stm32wba_HmacSha224_Dev = WHAL_CFG_STM32WBA_HASH_HMAC_SHA224_DEV;
const whal_HmacSha256 whal_Stm32wba_HmacSha256_Dev = WHAL_CFG_STM32WBA_HASH_HMAC_SHA256_DEV;

/* Control Register */
#define HASH_CR_REG           0x00

#define HASH_CR_INIT_Pos      2
#define HASH_CR_INIT_Msk      (1UL << HASH_CR_INIT_Pos)

#define HASH_CR_DATATYPE_Pos  4
#define HASH_CR_DATATYPE_Msk  (3UL << HASH_CR_DATATYPE_Pos)

#define HASH_CR_MODE_Pos      6
#define HASH_CR_MODE_Msk      (1UL << HASH_CR_MODE_Pos)

#define HASH_CR_LKEY_Pos      16
#define HASH_CR_LKEY_Msk      (1UL << HASH_CR_LKEY_Pos)

#define HASH_CR_ALGO_Pos      17
#define HASH_CR_ALGO_Msk      (3UL << HASH_CR_ALGO_Pos)

/* Data Input Register */
#define HASH_DIN_REG          0x04

/* Start Register */
#define HASH_STR_REG          0x08

#define HASH_STR_NBLW_Pos     0
#define HASH_STR_NBLW_Msk     (0x1FUL << HASH_STR_NBLW_Pos)

#define HASH_STR_DCAL_Pos     8
#define HASH_STR_DCAL_Msk     (1UL << HASH_STR_DCAL_Pos)

/* Status Register */
#define HASH_SR_REG           0x24

#define HASH_SR_BUSY_Pos      3
#define HASH_SR_BUSY_Msk      (1UL << HASH_SR_BUSY_Pos)

/* Digest Result Registers */
#define HASH_HR_BASE_REG      0x310

/* Algorithm encoding: ALGO[1:0] at bits 18:17 */
#define HASH_ALGO_SHA1        0
#define HASH_ALGO_SHA224      2
#define HASH_ALGO_SHA256      3

#define HASH_MODE_HASH        0
#define HASH_MODE_HMAC        1

/* Per-driver HMAC state surviving Start→Finalize. Singleton drivers so static. */
static whal_Stm32wba_HmacSha1_State   g_hmacSha1State;
static whal_Stm32wba_HmacSha224_State g_hmacSha224State;
static whal_Stm32wba_HmacSha256_State g_hmacSha256State;

/* Hash peripheral is single-instance, so one shared 0-3 byte staging
 * buffer is enough — only one streaming session can be live at a time.
 * Process accumulates leftover bytes here until the next call brings the
 * count to 4 (push as a full word) or Finalize drains them as the
 * genuine last partial word. */
static uint8_t g_streamBuf[4];
static size_t  g_streamBufBytes;

static uint32_t AlgoBits(size_t algo)
{
    return whal_SetBits(HASH_CR_ALGO_Msk, HASH_CR_ALGO_Pos, algo);
}

static whal_Error WaitForReady(size_t base, whal_Timeout *timeout)
{
    return whal_Reg_ReadPoll(base, HASH_SR_REG, HASH_SR_BUSY_Msk, 0, timeout);
}

/* Reset the stream staging buffer; called from each Start. */
static void StreamReset(void)
{
    g_streamBufBytes = 0;
}

/* Accumulate Process input. Tops up the staging buffer to a full word if
 * it had leftovers, pushes full 32-bit DIN writes, and stashes the
 * trailing 0-3 bytes back into the staging buffer for the next call. */
static void StreamWrite(size_t base, const uint8_t *in, size_t inSz)
{
    if (g_streamBufBytes > 0) {
        while (g_streamBufBytes < 4 && inSz > 0) {
            g_streamBuf[g_streamBufBytes++] = *in++;
            inSz--;
        }
        if (g_streamBufBytes == 4) {
            whal_Reg_Write(base, HASH_DIN_REG, whal_LoadBe32(g_streamBuf));
            g_streamBufBytes = 0;
        }
    }

    while (inSz >= 4) {
        whal_Reg_Write(base, HASH_DIN_REG, whal_LoadBe32(in));
        in += 4;
        inSz -= 4;
    }

    while (inSz > 0) {
        g_streamBuf[g_streamBufBytes++] = *in++;
        inSz--;
    }
}

/* Push inSz bytes immediately before issuing DCAL: full words first, then
 * if any trailing bytes remain, set NBLW and push one partial word. */
static void WriteTail(size_t base, const uint8_t *in, size_t inSz)
{
    while (inSz >= 4) {
        whal_Reg_Write(base, HASH_DIN_REG, whal_LoadBe32(in));
        in += 4;
        inSz -= 4;
    }

    if (inSz > 0) {
        whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk,
                        whal_SetBits(HASH_STR_NBLW_Msk, HASH_STR_NBLW_Pos,
                                     inSz * 8));
        whal_Reg_Write(base, HASH_DIN_REG, whal_LoadBe32Partial(in, inSz));
    }
}

/* Drain any buffered 0-3 trailing bytes as the final partial word right
 * before DCAL; sets NBLW correctly via WriteTail. */
static void StreamDrain(size_t base)
{
    WriteTail(base, g_streamBuf, g_streamBufBytes);
    g_streamBufBytes = 0;
}

static void ReadDigest(size_t base, uint8_t *digest, size_t digestSz)
{
    size_t words = digestSz / 4;
    size_t i;

    for (i = 0; i < words; i++)
        whal_StoreBe32(digest + i * 4,
                       whal_Reg_Read(base, HASH_HR_BASE_REG + i * 4));
}



/* ---- Direct API mapping ---- */

#ifdef WHAL_CFG_STM32WBA_HASH_INIT_DIRECT_API_MAPPING
#define whal_Stm32wba_Hash_Init          whal_Crypto_Init
#define whal_Stm32wba_Hash_Deinit        whal_Crypto_Deinit
#endif

#ifdef WHAL_CFG_STM32WBA_HASH_SHA1_DIRECT_API_MAPPING
#define whal_Stm32wba_Sha1_Oneshot       whal_Sha1_Oneshot
#define whal_Stm32wba_Sha1_Start         whal_Sha1_Start
#define whal_Stm32wba_Sha1_Process       whal_Sha1_Process
#define whal_Stm32wba_Sha1_Finalize      whal_Sha1_Finalize
#endif

#ifdef WHAL_CFG_STM32WBA_HASH_SHA224_DIRECT_API_MAPPING
#define whal_Stm32wba_Sha224_Oneshot     whal_Sha224_Oneshot
#define whal_Stm32wba_Sha224_Start       whal_Sha224_Start
#define whal_Stm32wba_Sha224_Process     whal_Sha224_Process
#define whal_Stm32wba_Sha224_Finalize    whal_Sha224_Finalize
#endif

#ifdef WHAL_CFG_STM32WBA_HASH_SHA256_DIRECT_API_MAPPING
#define whal_Stm32wba_Sha256_Oneshot     whal_Sha256_Oneshot
#define whal_Stm32wba_Sha256_Start       whal_Sha256_Start
#define whal_Stm32wba_Sha256_Process     whal_Sha256_Process
#define whal_Stm32wba_Sha256_Finalize    whal_Sha256_Finalize
#endif

#ifdef WHAL_CFG_STM32WBA_HASH_HMAC_SHA1_DIRECT_API_MAPPING
#define whal_Stm32wba_HmacSha1_Oneshot   whal_HmacSha1_Oneshot
#define whal_Stm32wba_HmacSha1_Start     whal_HmacSha1_Start
#define whal_Stm32wba_HmacSha1_Process   whal_HmacSha1_Process
#define whal_Stm32wba_HmacSha1_Finalize  whal_HmacSha1_Finalize
#endif

#ifdef WHAL_CFG_STM32WBA_HASH_HMAC_SHA224_DIRECT_API_MAPPING
#define whal_Stm32wba_HmacSha224_Oneshot  whal_HmacSha224_Oneshot
#define whal_Stm32wba_HmacSha224_Start    whal_HmacSha224_Start
#define whal_Stm32wba_HmacSha224_Process  whal_HmacSha224_Process
#define whal_Stm32wba_HmacSha224_Finalize whal_HmacSha224_Finalize
#endif

#ifdef WHAL_CFG_STM32WBA_HASH_HMAC_SHA256_DIRECT_API_MAPPING
#define whal_Stm32wba_HmacSha256_Oneshot  whal_HmacSha256_Oneshot
#define whal_Stm32wba_HmacSha256_Start    whal_HmacSha256_Start
#define whal_Stm32wba_HmacSha256_Process  whal_HmacSha256_Process
#define whal_Stm32wba_HmacSha256_Finalize whal_HmacSha256_Finalize
#endif


/* ---- Init / Deinit ---- */

whal_Error whal_Stm32wba_Hash_Init(whal_Crypto *dev)
{
    (void)dev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Hash_Deinit(whal_Crypto *dev)
{
    (void)dev;
    return WHAL_SUCCESS;
}

const whal_CryptoDriver whal_Stm32wba_Hash_CryptoDriver = {
    .Init = whal_Stm32wba_Hash_Init,
    .Deinit = whal_Stm32wba_Hash_Deinit,
};


/* ---- SHA-1 ---- */

whal_Error whal_Stm32wba_Sha1_Oneshot(whal_Sha1 *dev,
                                      const void *in, size_t inSz,
                                      void *digest, size_t digestSz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    whal_Error err;
    (void)dev;

    if (!digest || digestSz != 20)
        return WHAL_EINVAL;

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);

    whal_Reg_Update(base, HASH_CR_REG,
                    HASH_CR_ALGO_Msk | HASH_CR_DATATYPE_Msk |
                    HASH_CR_MODE_Msk | HASH_CR_LKEY_Msk | HASH_CR_INIT_Msk,
                    AlgoBits(HASH_ALGO_SHA1) | HASH_CR_INIT_Msk);

    if (inSz > 0) {
        if (!in)
            return WHAL_EINVAL;
        WriteTail(base, (const uint8_t *)in, inSz);
    }

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    ReadDigest(base, (uint8_t *)digest, digestSz);
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Sha1_Start(whal_Sha1 *dev)
{
    size_t base = whal_Stm32wba_Hash_Dev.base;
    (void)dev;

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);

    whal_Reg_Update(base, HASH_CR_REG,
                    HASH_CR_ALGO_Msk | HASH_CR_DATATYPE_Msk |
                    HASH_CR_MODE_Msk | HASH_CR_LKEY_Msk | HASH_CR_INIT_Msk,
                    AlgoBits(HASH_ALGO_SHA1) | HASH_CR_INIT_Msk);

    StreamReset();
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Sha1_Process(whal_Sha1 *dev,
                                      const void *in, size_t inSz)
{
    (void)dev;
    if (inSz == 0)
        return WHAL_SUCCESS;
    if (!in)
        return WHAL_EINVAL;
    StreamWrite(whal_Stm32wba_Hash_Dev.base, (const uint8_t *)in, inSz);
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Sha1_Finalize(whal_Sha1 *dev,
                                       void *digest, size_t digestSz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    whal_Error err;
    (void)dev;

    if (!digest || digestSz != 20)
        return WHAL_EINVAL;

    StreamDrain(base);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    ReadDigest(base, (uint8_t *)digest, digestSz);
    return WHAL_SUCCESS;
}

const whal_Sha1Driver whal_Stm32wba_Hash_Sha1Driver = {
    .Oneshot = whal_Stm32wba_Sha1_Oneshot,
    .Start = whal_Stm32wba_Sha1_Start,
    .Process = whal_Stm32wba_Sha1_Process,
    .Finalize = whal_Stm32wba_Sha1_Finalize,
};


/* ---- SHA-224 ---- */

whal_Error whal_Stm32wba_Sha224_Oneshot(whal_Sha224 *dev,
                                        const void *in, size_t inSz,
                                        void *digest, size_t digestSz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    whal_Error err;
    (void)dev;

    if (!digest || digestSz != 28)
        return WHAL_EINVAL;

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);

    whal_Reg_Update(base, HASH_CR_REG,
                    HASH_CR_ALGO_Msk | HASH_CR_DATATYPE_Msk |
                    HASH_CR_MODE_Msk | HASH_CR_LKEY_Msk | HASH_CR_INIT_Msk,
                    AlgoBits(HASH_ALGO_SHA224) | HASH_CR_INIT_Msk);

    if (inSz > 0) {
        if (!in)
            return WHAL_EINVAL;
        WriteTail(base, (const uint8_t *)in, inSz);
    }

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    ReadDigest(base, (uint8_t *)digest, digestSz);
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Sha224_Start(whal_Sha224 *dev)
{
    size_t base = whal_Stm32wba_Hash_Dev.base;
    (void)dev;

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);

    whal_Reg_Update(base, HASH_CR_REG,
                    HASH_CR_ALGO_Msk | HASH_CR_DATATYPE_Msk |
                    HASH_CR_MODE_Msk | HASH_CR_LKEY_Msk | HASH_CR_INIT_Msk,
                    AlgoBits(HASH_ALGO_SHA224) | HASH_CR_INIT_Msk);

    StreamReset();
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Sha224_Process(whal_Sha224 *dev,
                                        const void *in, size_t inSz)
{
    (void)dev;
    if (inSz == 0)
        return WHAL_SUCCESS;
    if (!in)
        return WHAL_EINVAL;
    StreamWrite(whal_Stm32wba_Hash_Dev.base, (const uint8_t *)in, inSz);
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Sha224_Finalize(whal_Sha224 *dev,
                                         void *digest, size_t digestSz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    whal_Error err;
    (void)dev;

    if (!digest || digestSz != 28)
        return WHAL_EINVAL;

    StreamDrain(base);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    ReadDigest(base, (uint8_t *)digest, digestSz);
    return WHAL_SUCCESS;
}

const whal_Sha224Driver whal_Stm32wba_Hash_Sha224Driver = {
    .Oneshot = whal_Stm32wba_Sha224_Oneshot,
    .Start = whal_Stm32wba_Sha224_Start,
    .Process = whal_Stm32wba_Sha224_Process,
    .Finalize = whal_Stm32wba_Sha224_Finalize,
};


/* ---- SHA-256 ---- */

whal_Error whal_Stm32wba_Sha256_Oneshot(whal_Sha256 *dev,
                                        const void *in, size_t inSz,
                                        void *digest, size_t digestSz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    whal_Error err;
    (void)dev;

    if (!digest || digestSz != 32)
        return WHAL_EINVAL;

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);

    whal_Reg_Update(base, HASH_CR_REG,
                    HASH_CR_ALGO_Msk | HASH_CR_DATATYPE_Msk |
                    HASH_CR_MODE_Msk | HASH_CR_LKEY_Msk | HASH_CR_INIT_Msk,
                    AlgoBits(HASH_ALGO_SHA256) | HASH_CR_INIT_Msk);

    if (inSz > 0) {
        if (!in)
            return WHAL_EINVAL;
        WriteTail(base, (const uint8_t *)in, inSz);
    }

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    ReadDigest(base, (uint8_t *)digest, digestSz);
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Sha256_Start(whal_Sha256 *dev)
{
    size_t base = whal_Stm32wba_Hash_Dev.base;
    (void)dev;

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);

    whal_Reg_Update(base, HASH_CR_REG,
                    HASH_CR_ALGO_Msk | HASH_CR_DATATYPE_Msk |
                    HASH_CR_MODE_Msk | HASH_CR_LKEY_Msk | HASH_CR_INIT_Msk,
                    AlgoBits(HASH_ALGO_SHA256) | HASH_CR_INIT_Msk);

    StreamReset();
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Sha256_Process(whal_Sha256 *dev,
                                        const void *in, size_t inSz)
{
    (void)dev;
    if (inSz == 0)
        return WHAL_SUCCESS;
    if (!in)
        return WHAL_EINVAL;
    StreamWrite(whal_Stm32wba_Hash_Dev.base, (const uint8_t *)in, inSz);
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Sha256_Finalize(whal_Sha256 *dev,
                                         void *digest, size_t digestSz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    whal_Error err;
    (void)dev;

    if (!digest || digestSz != 32)
        return WHAL_EINVAL;

    StreamDrain(base);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    ReadDigest(base, (uint8_t *)digest, digestSz);
    return WHAL_SUCCESS;
}

const whal_Sha256Driver whal_Stm32wba_Hash_Sha256Driver = {
    .Oneshot = whal_Stm32wba_Sha256_Oneshot,
    .Start = whal_Stm32wba_Sha256_Start,
    .Process = whal_Stm32wba_Sha256_Process,
    .Finalize = whal_Stm32wba_Sha256_Finalize,
};


/* ---- HMAC-SHA-1 ---- */

whal_Error whal_Stm32wba_HmacSha1_Oneshot(whal_HmacSha1 *dev,
                                          const void *key, size_t keySz,
                                          const void *in, size_t inSz,
                                          void *digest, size_t digestSz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    size_t lkey;
    whal_Error err;
    (void)dev;

    if (!key || !digest || digestSz != 20)
        return WHAL_EINVAL;

    lkey = (keySz > 64) ? 1 : 0;

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);

    whal_Reg_Update(base, HASH_CR_REG,
                    HASH_CR_ALGO_Msk | HASH_CR_DATATYPE_Msk |
                    HASH_CR_MODE_Msk | HASH_CR_LKEY_Msk | HASH_CR_INIT_Msk,
                    AlgoBits(HASH_ALGO_SHA1) |
                    whal_SetBits(HASH_CR_MODE_Msk, HASH_CR_MODE_Pos,
                                 HASH_MODE_HMAC) |
                    whal_SetBits(HASH_CR_LKEY_Msk, HASH_CR_LKEY_Pos, lkey) |
                    HASH_CR_INIT_Msk);

    /* Inner key */
    WriteTail(base, (const uint8_t *)key, keySz);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    /* Message */
    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);
    if (inSz > 0) {
        if (!in)
            return WHAL_EINVAL;
        WriteTail(base, (const uint8_t *)in, inSz);
    }

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    /* Outer key */
    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);
    WriteTail(base, (const uint8_t *)key, keySz);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    ReadDigest(base, (uint8_t *)digest, digestSz);
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_HmacSha1_Start(whal_HmacSha1 *dev,
                                         const void *key, size_t keySz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    size_t lkey;
    whal_Error err;
    (void)dev;

    if (!key)
        return WHAL_EINVAL;

    lkey = (keySz > 64) ? 1 : 0;

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);

    whal_Reg_Update(base, HASH_CR_REG,
                    HASH_CR_ALGO_Msk | HASH_CR_DATATYPE_Msk |
                    HASH_CR_MODE_Msk | HASH_CR_LKEY_Msk | HASH_CR_INIT_Msk,
                    AlgoBits(HASH_ALGO_SHA1) |
                    whal_SetBits(HASH_CR_MODE_Msk, HASH_CR_MODE_Pos,
                                 HASH_MODE_HMAC) |
                    whal_SetBits(HASH_CR_LKEY_Msk, HASH_CR_LKEY_Pos, lkey) |
                    HASH_CR_INIT_Msk);

    /* Inner key */
    WriteTail(base, (const uint8_t *)key, keySz);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    /* Store key for outer-key phase in Finalize */
    g_hmacSha1State.key = key;
    g_hmacSha1State.keySz = keySz;

    StreamReset();
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_HmacSha1_Process(whal_HmacSha1 *dev,
                                           const void *in, size_t inSz)
{
    (void)dev;
    if (inSz == 0)
        return WHAL_SUCCESS;
    if (!in)
        return WHAL_EINVAL;
    StreamWrite(whal_Stm32wba_Hash_Dev.base, (const uint8_t *)in, inSz);
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_HmacSha1_Finalize(whal_HmacSha1 *dev,
                                            void *digest, size_t digestSz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    whal_Error err;
    (void)dev;

    if (!digest || digestSz != 20)
        return WHAL_EINVAL;

    /* Drain any buffered tail bytes, then DCAL to close message phase. */
    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);
    StreamDrain(base);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    /* Outer key */
    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);
    WriteTail(base, (const uint8_t *)g_hmacSha1State.key, g_hmacSha1State.keySz);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    ReadDigest(base, (uint8_t *)digest, digestSz);
    return WHAL_SUCCESS;
}

const whal_HmacSha1Driver whal_Stm32wba_Hash_HmacSha1Driver = {
    .Oneshot = whal_Stm32wba_HmacSha1_Oneshot,
    .Start = whal_Stm32wba_HmacSha1_Start,
    .Process = whal_Stm32wba_HmacSha1_Process,
    .Finalize = whal_Stm32wba_HmacSha1_Finalize,
};


/* ---- HMAC-SHA-224 ---- */

whal_Error whal_Stm32wba_HmacSha224_Oneshot(whal_HmacSha224 *dev,
                                            const void *key, size_t keySz,
                                            const void *in, size_t inSz,
                                            void *digest, size_t digestSz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    size_t lkey;
    whal_Error err;
    (void)dev;

    if (!key || !digest || digestSz != 28)
        return WHAL_EINVAL;

    lkey = (keySz > 64) ? 1 : 0;

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);

    whal_Reg_Update(base, HASH_CR_REG,
                    HASH_CR_ALGO_Msk | HASH_CR_DATATYPE_Msk |
                    HASH_CR_MODE_Msk | HASH_CR_LKEY_Msk | HASH_CR_INIT_Msk,
                    AlgoBits(HASH_ALGO_SHA224) |
                    whal_SetBits(HASH_CR_MODE_Msk, HASH_CR_MODE_Pos,
                                 HASH_MODE_HMAC) |
                    whal_SetBits(HASH_CR_LKEY_Msk, HASH_CR_LKEY_Pos, lkey) |
                    HASH_CR_INIT_Msk);

    /* Inner key */
    WriteTail(base, (const uint8_t *)key, keySz);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    /* Message */
    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);
    if (inSz > 0) {
        if (!in)
            return WHAL_EINVAL;
        WriteTail(base, (const uint8_t *)in, inSz);
    }

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    /* Outer key */
    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);
    WriteTail(base, (const uint8_t *)key, keySz);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    ReadDigest(base, (uint8_t *)digest, digestSz);
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_HmacSha224_Start(whal_HmacSha224 *dev,
                                           const void *key, size_t keySz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    size_t lkey;
    whal_Error err;
    (void)dev;

    if (!key)
        return WHAL_EINVAL;

    lkey = (keySz > 64) ? 1 : 0;

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);

    whal_Reg_Update(base, HASH_CR_REG,
                    HASH_CR_ALGO_Msk | HASH_CR_DATATYPE_Msk |
                    HASH_CR_MODE_Msk | HASH_CR_LKEY_Msk | HASH_CR_INIT_Msk,
                    AlgoBits(HASH_ALGO_SHA224) |
                    whal_SetBits(HASH_CR_MODE_Msk, HASH_CR_MODE_Pos,
                                 HASH_MODE_HMAC) |
                    whal_SetBits(HASH_CR_LKEY_Msk, HASH_CR_LKEY_Pos, lkey) |
                    HASH_CR_INIT_Msk);

    /* Inner key */
    WriteTail(base, (const uint8_t *)key, keySz);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    /* Store key for outer-key phase in Finalize */
    g_hmacSha224State.key = key;
    g_hmacSha224State.keySz = keySz;

    StreamReset();
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_HmacSha224_Process(whal_HmacSha224 *dev,
                                             const void *in, size_t inSz)
{
    (void)dev;
    if (inSz == 0)
        return WHAL_SUCCESS;
    if (!in)
        return WHAL_EINVAL;
    StreamWrite(whal_Stm32wba_Hash_Dev.base, (const uint8_t *)in, inSz);
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_HmacSha224_Finalize(whal_HmacSha224 *dev,
                                              void *digest, size_t digestSz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    whal_Error err;
    (void)dev;

    if (!digest || digestSz != 28)
        return WHAL_EINVAL;

    /* Drain any buffered tail bytes, then DCAL to close message phase. */
    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);
    StreamDrain(base);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    /* Outer key */
    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);
    WriteTail(base, (const uint8_t *)g_hmacSha224State.key, g_hmacSha224State.keySz);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    ReadDigest(base, (uint8_t *)digest, digestSz);
    return WHAL_SUCCESS;
}

const whal_HmacSha224Driver whal_Stm32wba_Hash_HmacSha224Driver = {
    .Oneshot = whal_Stm32wba_HmacSha224_Oneshot,
    .Start = whal_Stm32wba_HmacSha224_Start,
    .Process = whal_Stm32wba_HmacSha224_Process,
    .Finalize = whal_Stm32wba_HmacSha224_Finalize,
};


/* ---- HMAC-SHA-256 ---- */

whal_Error whal_Stm32wba_HmacSha256_Oneshot(whal_HmacSha256 *dev,
                                            const void *key, size_t keySz,
                                            const void *in, size_t inSz,
                                            void *digest, size_t digestSz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    size_t lkey;
    whal_Error err;
    (void)dev;

    if (!key || !digest || digestSz != 32)
        return WHAL_EINVAL;

    lkey = (keySz > 64) ? 1 : 0;

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);

    whal_Reg_Update(base, HASH_CR_REG,
                    HASH_CR_ALGO_Msk | HASH_CR_DATATYPE_Msk |
                    HASH_CR_MODE_Msk | HASH_CR_LKEY_Msk | HASH_CR_INIT_Msk,
                    AlgoBits(HASH_ALGO_SHA256) |
                    whal_SetBits(HASH_CR_MODE_Msk, HASH_CR_MODE_Pos,
                                 HASH_MODE_HMAC) |
                    whal_SetBits(HASH_CR_LKEY_Msk, HASH_CR_LKEY_Pos, lkey) |
                    HASH_CR_INIT_Msk);

    /* Inner key */
    WriteTail(base, (const uint8_t *)key, keySz);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    /* Message */
    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);
    if (inSz > 0) {
        if (!in)
            return WHAL_EINVAL;
        WriteTail(base, (const uint8_t *)in, inSz);
    }

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    /* Outer key */
    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);
    WriteTail(base, (const uint8_t *)key, keySz);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    ReadDigest(base, (uint8_t *)digest, digestSz);
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_HmacSha256_Start(whal_HmacSha256 *dev,
                                           const void *key, size_t keySz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    size_t lkey;
    whal_Error err;
    (void)dev;

    if (!key)
        return WHAL_EINVAL;

    lkey = (keySz > 64) ? 1 : 0;

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);

    whal_Reg_Update(base, HASH_CR_REG,
                    HASH_CR_ALGO_Msk | HASH_CR_DATATYPE_Msk |
                    HASH_CR_MODE_Msk | HASH_CR_LKEY_Msk | HASH_CR_INIT_Msk,
                    AlgoBits(HASH_ALGO_SHA256) |
                    whal_SetBits(HASH_CR_MODE_Msk, HASH_CR_MODE_Pos,
                                 HASH_MODE_HMAC) |
                    whal_SetBits(HASH_CR_LKEY_Msk, HASH_CR_LKEY_Pos, lkey) |
                    HASH_CR_INIT_Msk);

    /* Inner key */
    WriteTail(base, (const uint8_t *)key, keySz);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    /* Store key for outer-key phase in Finalize */
    g_hmacSha256State.key = key;
    g_hmacSha256State.keySz = keySz;

    StreamReset();
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_HmacSha256_Process(whal_HmacSha256 *dev,
                                             const void *in, size_t inSz)
{
    (void)dev;
    if (inSz == 0)
        return WHAL_SUCCESS;
    if (!in)
        return WHAL_EINVAL;
    StreamWrite(whal_Stm32wba_Hash_Dev.base, (const uint8_t *)in, inSz);
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_HmacSha256_Finalize(whal_HmacSha256 *dev,
                                              void *digest, size_t digestSz)
{
    const whal_Stm32wba_Hash_Cfg *cfg =
        (const whal_Stm32wba_Hash_Cfg *)whal_Stm32wba_Hash_Dev.cfg;
    size_t base = whal_Stm32wba_Hash_Dev.base;
    whal_Error err;
    (void)dev;

    if (!digest || digestSz != 32)
        return WHAL_EINVAL;

    /* Drain any buffered tail bytes, then DCAL to close message phase. */
    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);
    StreamDrain(base);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    /* Outer key */
    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_NBLW_Msk, 0);
    WriteTail(base, (const uint8_t *)g_hmacSha256State.key, g_hmacSha256State.keySz);

    whal_Reg_Update(base, HASH_STR_REG, HASH_STR_DCAL_Msk,
                    HASH_STR_DCAL_Msk);

    err = WaitForReady(base, cfg->timeout);
    if (err)
        return err;

    ReadDigest(base, (uint8_t *)digest, digestSz);
    return WHAL_SUCCESS;
}

const whal_HmacSha256Driver whal_Stm32wba_Hash_HmacSha256Driver = {
    .Oneshot = whal_Stm32wba_HmacSha256_Oneshot,
    .Start = whal_Stm32wba_HmacSha256_Start,
    .Process = whal_Stm32wba_HmacSha256_Process,
    .Finalize = whal_Stm32wba_HmacSha256_Finalize,
};
