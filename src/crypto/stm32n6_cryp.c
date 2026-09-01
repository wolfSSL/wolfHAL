/* stm32n6_cryp.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32N6_CRYP*_DEV initializers */
#include <wolfHAL/crypto/stm32n6_cryp.h>
#include <wolfHAL/crypto/crypto.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/endian.h>

const whal_Crypto  whal_Stm32n6_Cryp_Dev      = WHAL_CFG_STM32N6_CRYP_DEV;
const whal_AesEcb  whal_Stm32n6_CrypEcb_Dev   = WHAL_CFG_STM32N6_CRYP_ECB_DEV;
const whal_AesCbc  whal_Stm32n6_CrypCbc_Dev   = WHAL_CFG_STM32N6_CRYP_CBC_DEV;
const whal_AesCtr  whal_Stm32n6_CrypCtr_Dev   = WHAL_CFG_STM32N6_CRYP_CTR_DEV;
const whal_AesGcm  whal_Stm32n6_CrypGcm_Dev   = WHAL_CFG_STM32N6_CRYP_GCM_DEV;
const whal_AesGmac whal_Stm32n6_CrypGmac_Dev  = WHAL_CFG_STM32N6_CRYP_GMAC_DEV;
const whal_AesCcm  whal_Stm32n6_CrypCcm_Dev   = WHAL_CFG_STM32N6_CRYP_CCM_DEV;

/* Control Register (CRYP_CR) */
#define CRYP_CR_REG            0x00
#define CRYP_CR_ALGODIR_Pos    2
#define CRYP_CR_ALGODIR_Msk    (1UL << CRYP_CR_ALGODIR_Pos)
#define CRYP_CR_ALGOMODE_LO_Pos 3
#define CRYP_CR_ALGOMODE_LO_Msk (7UL << CRYP_CR_ALGOMODE_LO_Pos)
#define CRYP_CR_DATATYPE_Pos   6
#define CRYP_CR_DATATYPE_Msk   (3UL << CRYP_CR_DATATYPE_Pos)
#define CRYP_CR_KEYSIZE_Pos    8
#define CRYP_CR_KEYSIZE_Msk    (3UL << CRYP_CR_KEYSIZE_Pos)
#define CRYP_CR_FFLUSH_Pos     14
#define CRYP_CR_FFLUSH_Msk     (1UL << CRYP_CR_FFLUSH_Pos)
#define CRYP_CR_CRYPEN_Pos     15
#define CRYP_CR_CRYPEN_Msk     (1UL << CRYP_CR_CRYPEN_Pos)
#define CRYP_CR_GCM_CCMPH_Pos  16
#define CRYP_CR_GCM_CCMPH_Msk  (3UL << CRYP_CR_GCM_CCMPH_Pos)
#define CRYP_CR_ALGOMODE_HI_Pos 19
#define CRYP_CR_ALGOMODE_HI_Msk (1UL << CRYP_CR_ALGOMODE_HI_Pos)
#define CRYP_CR_NPBLB_Pos      20
#define CRYP_CR_NPBLB_Msk      (0xFUL << CRYP_CR_NPBLB_Pos)
#define CRYP_CR_KMOD_Pos       24
#define CRYP_CR_KMOD_Msk       (3UL << CRYP_CR_KMOD_Pos)
#define CRYP_CR_IPRST_Pos      31
#define CRYP_CR_IPRST_Msk      (1UL << CRYP_CR_IPRST_Pos)

#define CRYP_CR_ALGOMODE_Msk \
    (CRYP_CR_ALGOMODE_LO_Msk | CRYP_CR_ALGOMODE_HI_Msk)

/* Pack a 4-bit ALGOMODE into the split CR bitfields (bits 5:3 + bit 19). */
#define CRYP_CR_ALGOMODE(v) \
    ((((uint32_t)(v) & 0x7U) << CRYP_CR_ALGOMODE_LO_Pos) | \
     ((((uint32_t)(v) >> 3) & 0x1U) << CRYP_CR_ALGOMODE_HI_Pos))

/* ALGOMODE values */
#define CRYP_ALGOMODE_AES_ECB     0x4
#define CRYP_ALGOMODE_AES_CBC     0x5
#define CRYP_ALGOMODE_AES_CTR     0x6
#define CRYP_ALGOMODE_AES_KEYPREP 0x7
#define CRYP_ALGOMODE_AES_GCM     0x8
#define CRYP_ALGOMODE_AES_CCM     0x9

#define CRYP_ALGODIR_ENCRYPT 0
#define CRYP_ALGODIR_DECRYPT 1

#define CRYP_KEYSIZE_128 0
#define CRYP_KEYSIZE_192 1
#define CRYP_KEYSIZE_256 2

#define CRYP_GCM_CCMPH_INIT    0
#define CRYP_GCM_CCMPH_HEADER  1
#define CRYP_GCM_CCMPH_PAYLOAD 2
#define CRYP_GCM_CCMPH_FINAL   3

/* Status Register (CRYP_SR) */
#define CRYP_SR_REG            0x04
#define CRYP_SR_IFEM_Msk       (1UL << 0)
#define CRYP_SR_IFNF_Msk       (1UL << 1)
#define CRYP_SR_OFNE_Msk       (1UL << 2)
#define CRYP_SR_OFFU_Msk       (1UL << 3)
#define CRYP_SR_BUSY_Msk       (1UL << 4)
#define CRYP_SR_KERF_Msk       (1UL << 6)
#define CRYP_SR_KEYVALID_Msk   (1UL << 7)

/* Data Registers */
#define CRYP_DINR_REG          0x08
#define CRYP_DOUTR_REG         0x0C

/* Key Registers (write-only): K0LR..K3RR at 0x20..0x3C */
#define CRYP_K0LR_REG          0x20
#define CRYP_K0RR_REG          0x24
#define CRYP_K1LR_REG          0x28
#define CRYP_K1RR_REG          0x2C
#define CRYP_K2LR_REG          0x30
#define CRYP_K2RR_REG          0x34
#define CRYP_K3LR_REG          0x38
#define CRYP_K3RR_REG          0x3C

/* Initialization Vector Registers: IV0LR..IV1RR at 0x40..0x4C */
#define CRYP_IV0LR_REG         0x40
#define CRYP_IV0RR_REG         0x44
#define CRYP_IV1LR_REG         0x48
#define CRYP_IV1RR_REG         0x4C

/* Per-driver state surviving Start→Finalize. Singleton drivers so static. */
static whal_Stm32n6_AesGcm_State g_aesGcmState;
static whal_Stm32n6_AesCcm_State g_aesCcmState;

static whal_Error WaitKeyValid(size_t base, whal_Timeout *timeout)
{
    return whal_Reg_ReadPoll(base, CRYP_SR_REG, CRYP_SR_KEYVALID_Msk,
                             CRYP_SR_KEYVALID_Msk, timeout);
}

static whal_Error WaitBusyClear(size_t base, whal_Timeout *timeout)
{
    return whal_Reg_ReadPoll(base, CRYP_SR_REG, CRYP_SR_BUSY_Msk, 0, timeout);
}

static whal_Error WaitCrypEnClear(size_t base, whal_Timeout *timeout)
{
    return whal_Reg_ReadPoll(base, CRYP_CR_REG, CRYP_CR_CRYPEN_Msk, 0, timeout);
}

static whal_Error WaitOutputReady(size_t base, whal_Timeout *timeout)
{
    return whal_Reg_ReadPoll(base, CRYP_SR_REG, CRYP_SR_OFNE_Msk,
                             CRYP_SR_OFNE_Msk, timeout);
}

static void DisableAndFlush(size_t base)
{
    whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_CRYPEN_Msk, 0);
    whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_FFLUSH_Msk, CRYP_CR_FFLUSH_Msk);
}

static void Enable(size_t base)
{
    whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_CRYPEN_Msk, CRYP_CR_CRYPEN_Msk);
}

static void Disable(size_t base)
{
    whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_CRYPEN_Msk, 0);
}

/*
 * Write the AES key into CRYP_KxL/R registers in the order required by the
 * hardware key scheduler: high-order 32-bit words first, descending into the
 * low 32 bits (K3RR last). The key is read big-endian from the input buffer.
 */
static void WriteKey(size_t base, const uint8_t *key, size_t keySz)
{
    const uint8_t *k = key;

    if (keySz == 32) {
        whal_Reg_Write(base, CRYP_K0LR_REG, whal_LoadBe32(k));
        whal_Reg_Write(base, CRYP_K0RR_REG, whal_LoadBe32(k + 4));
        whal_Reg_Write(base, CRYP_K1LR_REG, whal_LoadBe32(k + 8));
        whal_Reg_Write(base, CRYP_K1RR_REG, whal_LoadBe32(k + 12));
        k += 16;
    } else if (keySz == 24) {
        whal_Reg_Write(base, CRYP_K1LR_REG, whal_LoadBe32(k));
        whal_Reg_Write(base, CRYP_K1RR_REG, whal_LoadBe32(k + 4));
        k += 8;
    }
    whal_Reg_Write(base, CRYP_K2LR_REG, whal_LoadBe32(k));
    whal_Reg_Write(base, CRYP_K2RR_REG, whal_LoadBe32(k + 4));
    whal_Reg_Write(base, CRYP_K3LR_REG, whal_LoadBe32(k + 8));
    whal_Reg_Write(base, CRYP_K3RR_REG, whal_LoadBe32(k + 12));
}

static void WriteIv16(size_t base, const uint8_t *iv)
{
    whal_Reg_Write(base, CRYP_IV0LR_REG, whal_LoadBe32(iv));
    whal_Reg_Write(base, CRYP_IV0RR_REG, whal_LoadBe32(iv + 4));
    whal_Reg_Write(base, CRYP_IV1LR_REG, whal_LoadBe32(iv + 8));
    whal_Reg_Write(base, CRYP_IV1RR_REG, whal_LoadBe32(iv + 12));
}

static void ZeroKeyIv(size_t base)
{
    whal_Reg_Write(base, CRYP_K0LR_REG, 0);
    whal_Reg_Write(base, CRYP_K0RR_REG, 0);
    whal_Reg_Write(base, CRYP_K1LR_REG, 0);
    whal_Reg_Write(base, CRYP_K1RR_REG, 0);
    whal_Reg_Write(base, CRYP_K2LR_REG, 0);
    whal_Reg_Write(base, CRYP_K2RR_REG, 0);
    whal_Reg_Write(base, CRYP_K3LR_REG, 0);
    whal_Reg_Write(base, CRYP_K3RR_REG, 0);
    whal_Reg_Write(base, CRYP_IV0LR_REG, 0);
    whal_Reg_Write(base, CRYP_IV0RR_REG, 0);
    whal_Reg_Write(base, CRYP_IV1LR_REG, 0);
    whal_Reg_Write(base, CRYP_IV1RR_REG, 0);
}

static void WriteBlock(size_t base, const uint8_t *in)
{
    whal_Reg_Write(base, CRYP_DINR_REG, whal_LoadBe32(in));
    whal_Reg_Write(base, CRYP_DINR_REG, whal_LoadBe32(in + 4));
    whal_Reg_Write(base, CRYP_DINR_REG, whal_LoadBe32(in + 8));
    whal_Reg_Write(base, CRYP_DINR_REG, whal_LoadBe32(in + 12));
}

static void ReadBlock(size_t base, uint8_t *out)
{
    whal_StoreBe32(out,      whal_Reg_Read(base, CRYP_DOUTR_REG));
    whal_StoreBe32(out + 4,  whal_Reg_Read(base, CRYP_DOUTR_REG));
    whal_StoreBe32(out + 8,  whal_Reg_Read(base, CRYP_DOUTR_REG));
    whal_StoreBe32(out + 12, whal_Reg_Read(base, CRYP_DOUTR_REG));
}

/*
 * Configure CR for an AES operation: clears every mode-related field and sets
 * the requested ALGOMODE/ALGODIR/KEYSIZE/KMOD/PHASE/NPBLB. CRYPEN/FFLUSH/IPRST
 * are left untouched.
 */
static void ConfigureMode(size_t base, uint32_t algoMode, uint32_t algoDir,
                          uint32_t keySize, uint32_t phase, uint32_t npblb,
                          uint32_t kmod)
{
    uint32_t mask = CRYP_CR_ALGOMODE_Msk | CRYP_CR_ALGODIR_Msk |
                    CRYP_CR_KEYSIZE_Msk | CRYP_CR_DATATYPE_Msk |
                    CRYP_CR_GCM_CCMPH_Msk | CRYP_CR_NPBLB_Msk |
                    CRYP_CR_KMOD_Msk;
    uint32_t value = CRYP_CR_ALGOMODE(algoMode) |
                     whal_SetBits(CRYP_CR_ALGODIR_Msk, CRYP_CR_ALGODIR_Pos,
                                  algoDir) |
                     whal_SetBits(CRYP_CR_KEYSIZE_Msk, CRYP_CR_KEYSIZE_Pos,
                                  keySize) |
                     whal_SetBits(CRYP_CR_GCM_CCMPH_Msk, CRYP_CR_GCM_CCMPH_Pos,
                                  phase) |
                     whal_SetBits(CRYP_CR_NPBLB_Msk, CRYP_CR_NPBLB_Pos, npblb) |
                     whal_SetBits(CRYP_CR_KMOD_Msk, CRYP_CR_KMOD_Pos, kmod);

    whal_Reg_Update(base, CRYP_CR_REG, mask, value);
}

static whal_Error KeySizeBits(size_t keySz, uint32_t *out)
{
    if (keySz == 16) {
        *out = CRYP_KEYSIZE_128;
    } else if (keySz == 24) {
        *out = CRYP_KEYSIZE_192;
    } else if (keySz == 32) {
        *out = CRYP_KEYSIZE_256;
    } else {
        return WHAL_ENOTSUP;
    }
    return WHAL_SUCCESS;
}

/*
 * Run the ECB/CBC decryption key-preparation pass. Required before performing
 * an ECB or CBC decryption: the hardware computes the last round key, then
 * auto-clears CRYPEN. Caller must hold the timeout and have keys staged.
 */
static whal_Error PrepareDecryptionKey(size_t base, const uint8_t *key,
                                       size_t keySz, uint32_t keySizeBits,
                                       whal_Timeout *timeout)
{
    whal_Error err;

    DisableAndFlush(base);
    ConfigureMode(base, CRYP_ALGOMODE_AES_KEYPREP, CRYP_ALGODIR_ENCRYPT,
                  keySizeBits, 0, 0, 0);
    WriteKey(base, key, keySz);
    err = WaitKeyValid(base, timeout);
    if (err)
        return err;
    Enable(base);
    return WaitCrypEnClear(base, timeout);
}

static whal_Error Process_BlockCipher(const uint8_t *in, uint8_t *out, size_t sz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    whal_Error err;
    size_t i;

    if (sz == 0)
        return WHAL_SUCCESS;

    if (!in || !out || (sz & 0xF) != 0) {
        Disable(base);
        ZeroKeyIv(base);
        return WHAL_EINVAL;
    }

    for (i = 0; i < sz; i += 16) {
        WriteBlock(base, in + i);
        err = WaitOutputReady(base, cfg->timeout);
        if (err) {
            Disable(base);
            ZeroKeyIv(base);
            return err;
        }
        ReadBlock(base, out + i);
    }

    return WHAL_SUCCESS;
}


/* ---- Direct API mapping ---- */

#ifdef WHAL_CFG_STM32N6_CRYP_INIT_DIRECT_API_MAPPING
#define whal_Stm32n6_Cryp_Init        whal_Crypto_Init
#define whal_Stm32n6_Cryp_Deinit      whal_Crypto_Deinit
#endif
#ifdef WHAL_CFG_STM32N6_CRYP_ECB_DIRECT_API_MAPPING
#define whal_Stm32n6_CrypAesEcb_Oneshot  whal_AesEcb_Oneshot
#define whal_Stm32n6_CrypAesEcb_Start    whal_AesEcb_Start
#define whal_Stm32n6_CrypAesEcb_Process  whal_AesEcb_Process
#endif
#ifdef WHAL_CFG_STM32N6_CRYP_CBC_DIRECT_API_MAPPING
#define whal_Stm32n6_CrypAesCbc_Oneshot  whal_AesCbc_Oneshot
#define whal_Stm32n6_CrypAesCbc_Start    whal_AesCbc_Start
#define whal_Stm32n6_CrypAesCbc_Process  whal_AesCbc_Process
#endif
#ifdef WHAL_CFG_STM32N6_CRYP_CTR_DIRECT_API_MAPPING
#define whal_Stm32n6_CrypAesCtr_Oneshot  whal_AesCtr_Oneshot
#define whal_Stm32n6_CrypAesCtr_Start    whal_AesCtr_Start
#define whal_Stm32n6_CrypAesCtr_Process  whal_AesCtr_Process
#endif
#ifdef WHAL_CFG_STM32N6_CRYP_GCM_DIRECT_API_MAPPING
#define whal_Stm32n6_CrypAesGcm_Oneshot  whal_AesGcm_Oneshot
#define whal_Stm32n6_CrypAesGcm_Start    whal_AesGcm_Start
#define whal_Stm32n6_CrypAesGcm_Process  whal_AesGcm_Process
#define whal_Stm32n6_CrypAesGcm_Finalize whal_AesGcm_Finalize
#endif
#ifdef WHAL_CFG_STM32N6_CRYP_GMAC_DIRECT_API_MAPPING
#define whal_Stm32n6_CrypAesGmac_Oneshot whal_AesGmac_Oneshot
#endif
#ifdef WHAL_CFG_STM32N6_CRYP_CCM_DIRECT_API_MAPPING
#define whal_Stm32n6_CrypAesCcm_Oneshot  whal_AesCcm_Oneshot
#define whal_Stm32n6_CrypAesCcm_Start    whal_AesCcm_Start
#define whal_Stm32n6_CrypAesCcm_Process  whal_AesCcm_Process
#define whal_Stm32n6_CrypAesCcm_Finalize whal_AesCcm_Finalize
#endif


/* ---- Init / Deinit ---- */

whal_Error whal_Stm32n6_Cryp_Init(whal_Crypto *dev)
{
    (void)dev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32n6_Cryp_Deinit(whal_Crypto *dev)
{
    (void)dev;
    Disable(whal_Stm32n6_Cryp_Dev.base);
    ZeroKeyIv(whal_Stm32n6_Cryp_Dev.base);
    return WHAL_SUCCESS;
}

const whal_CryptoDriver whal_Stm32n6_Cryp_CryptoDriver = {
    .Init = whal_Stm32n6_Cryp_Init,
    .Deinit = whal_Stm32n6_Cryp_Deinit,
};


/* ---- AES-ECB ---- */

whal_Error whal_Stm32n6_CrypAesEcb_Oneshot(whal_AesEcb *dev,
                                           whal_Crypto_Dir dir,
                                           const void *key, size_t keySz,
                                           const void *in, void *out,
                                           size_t sz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint32_t keySizeBits;
    whal_Error err;
    (void)dev;

    if (!key)
        return WHAL_EINVAL;

    err = KeySizeBits(keySz, &keySizeBits);
    if (err)
        return err;

    if (dir == WHAL_CRYPTO_DECRYPT) {
        err = PrepareDecryptionKey(base, key, keySz, keySizeBits,
                                   cfg->timeout);
        if (err)
            goto cleanup;
        ConfigureMode(base, CRYP_ALGOMODE_AES_ECB, CRYP_ALGODIR_DECRYPT,
                      keySizeBits, 0, 0, 0);
        Enable(base);
    } else {
        DisableAndFlush(base);
        ConfigureMode(base, CRYP_ALGOMODE_AES_ECB, CRYP_ALGODIR_ENCRYPT,
                      keySizeBits, 0, 0, 0);
        WriteKey(base, key, keySz);
        err = WaitKeyValid(base, cfg->timeout);
        if (err)
            goto cleanup;
        Enable(base);
    }

    err = Process_BlockCipher(in, out, sz);
cleanup:
    Disable(base);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32n6_CrypAesEcb_Start(whal_AesEcb *dev,
                                         whal_Crypto_Dir dir,
                                         const void *key, size_t keySz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint32_t keySizeBits;
    whal_Error err;
    (void)dev;

    if (!key)
        return WHAL_EINVAL;

    err = KeySizeBits(keySz, &keySizeBits);
    if (err)
        return err;

    if (dir == WHAL_CRYPTO_DECRYPT) {
        err = PrepareDecryptionKey(base, key, keySz, keySizeBits,
                                   cfg->timeout);
        if (err)
            goto cleanup;
        ConfigureMode(base, CRYP_ALGOMODE_AES_ECB, CRYP_ALGODIR_DECRYPT,
                      keySizeBits, 0, 0, 0);
        Enable(base);
    } else {
        DisableAndFlush(base);
        ConfigureMode(base, CRYP_ALGOMODE_AES_ECB, CRYP_ALGODIR_ENCRYPT,
                      keySizeBits, 0, 0, 0);
        WriteKey(base, key, keySz);
        err = WaitKeyValid(base, cfg->timeout);
        if (err)
            goto cleanup;
        Enable(base);
    }

    return WHAL_SUCCESS;

cleanup:
    Disable(base);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32n6_CrypAesEcb_Process(whal_AesEcb *dev,
                                           const void *in, void *out,
                                           size_t sz)
{
    (void)dev;
    return Process_BlockCipher(in, out, sz);
}

const whal_AesEcbDriver whal_Stm32n6_Cryp_EcbDriver = {
    .Oneshot = whal_Stm32n6_CrypAesEcb_Oneshot,
    .Start = whal_Stm32n6_CrypAesEcb_Start,
    .Process = whal_Stm32n6_CrypAesEcb_Process,
};


/* ---- AES-CBC ---- */

whal_Error whal_Stm32n6_CrypAesCbc_Oneshot(whal_AesCbc *dev,
                                           whal_Crypto_Dir dir,
                                           const void *key, size_t keySz,
                                           const void *iv,
                                           const void *in, void *out,
                                           size_t sz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint32_t keySizeBits;
    whal_Error err;
    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;

    err = KeySizeBits(keySz, &keySizeBits);
    if (err)
        return err;

    if (dir == WHAL_CRYPTO_DECRYPT) {
        err = PrepareDecryptionKey(base, key, keySz, keySizeBits,
                                   cfg->timeout);
        if (err)
            goto cleanup;
        ConfigureMode(base, CRYP_ALGOMODE_AES_CBC, CRYP_ALGODIR_DECRYPT,
                      keySizeBits, 0, 0, 0);
        WriteIv16(base, (const uint8_t *)iv);
        Enable(base);
    } else {
        DisableAndFlush(base);
        ConfigureMode(base, CRYP_ALGOMODE_AES_CBC, CRYP_ALGODIR_ENCRYPT,
                      keySizeBits, 0, 0, 0);
        WriteIv16(base, (const uint8_t *)iv);
        WriteKey(base, key, keySz);
        err = WaitKeyValid(base, cfg->timeout);
        if (err)
            goto cleanup;
        Enable(base);
    }

    err = Process_BlockCipher(in, out, sz);
cleanup:
    Disable(base);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32n6_CrypAesCbc_Start(whal_AesCbc *dev,
                                         whal_Crypto_Dir dir,
                                         const void *key, size_t keySz,
                                         const void *iv)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint32_t keySizeBits;
    whal_Error err;
    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;

    err = KeySizeBits(keySz, &keySizeBits);
    if (err)
        return err;

    if (dir == WHAL_CRYPTO_DECRYPT) {
        err = PrepareDecryptionKey(base, key, keySz, keySizeBits,
                                   cfg->timeout);
        if (err)
            goto cleanup;
        ConfigureMode(base, CRYP_ALGOMODE_AES_CBC, CRYP_ALGODIR_DECRYPT,
                      keySizeBits, 0, 0, 0);
        WriteIv16(base, (const uint8_t *)iv);
        Enable(base);
    } else {
        DisableAndFlush(base);
        ConfigureMode(base, CRYP_ALGOMODE_AES_CBC, CRYP_ALGODIR_ENCRYPT,
                      keySizeBits, 0, 0, 0);
        WriteIv16(base, (const uint8_t *)iv);
        WriteKey(base, key, keySz);
        err = WaitKeyValid(base, cfg->timeout);
        if (err)
            goto cleanup;
        Enable(base);
    }

    return WHAL_SUCCESS;

cleanup:
    Disable(base);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32n6_CrypAesCbc_Process(whal_AesCbc *dev,
                                           const void *in, void *out,
                                           size_t sz)
{
    (void)dev;
    return Process_BlockCipher(in, out, sz);
}

const whal_AesCbcDriver whal_Stm32n6_Cryp_CbcDriver = {
    .Oneshot = whal_Stm32n6_CrypAesCbc_Oneshot,
    .Start = whal_Stm32n6_CrypAesCbc_Start,
    .Process = whal_Stm32n6_CrypAesCbc_Process,
};


/* ---- AES-CTR ---- */

whal_Error whal_Stm32n6_CrypAesCtr_Oneshot(whal_AesCtr *dev,
                                           whal_Crypto_Dir dir,
                                           const void *key, size_t keySz,
                                           const void *iv,
                                           const void *in, void *out,
                                           size_t sz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint32_t keySizeBits;
    uint32_t algoDir;
    whal_Error err;
    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;

    err = KeySizeBits(keySz, &keySizeBits);
    if (err)
        return err;

    algoDir = (dir == WHAL_CRYPTO_ENCRYPT) ? CRYP_ALGODIR_ENCRYPT
                                           : CRYP_ALGODIR_DECRYPT;

    DisableAndFlush(base);
    ConfigureMode(base, CRYP_ALGOMODE_AES_CTR, algoDir, keySizeBits, 0, 0, 0);
    WriteIv16(base, (const uint8_t *)iv);
    WriteKey(base, key, keySz);
    err = WaitKeyValid(base, cfg->timeout);
    if (err)
        goto cleanup;
    Enable(base);

    err = Process_BlockCipher(in, out, sz);
cleanup:
    Disable(base);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32n6_CrypAesCtr_Start(whal_AesCtr *dev,
                                         whal_Crypto_Dir dir,
                                         const void *key, size_t keySz,
                                         const void *iv)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint32_t keySizeBits;
    uint32_t algoDir;
    whal_Error err;
    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;

    err = KeySizeBits(keySz, &keySizeBits);
    if (err)
        return err;

    algoDir = (dir == WHAL_CRYPTO_ENCRYPT) ? CRYP_ALGODIR_ENCRYPT
                                           : CRYP_ALGODIR_DECRYPT;

    DisableAndFlush(base);
    ConfigureMode(base, CRYP_ALGOMODE_AES_CTR, algoDir, keySizeBits, 0, 0, 0);
    WriteIv16(base, (const uint8_t *)iv);
    WriteKey(base, key, keySz);
    err = WaitKeyValid(base, cfg->timeout);
    if (err)
        goto cleanup;
    Enable(base);

    return WHAL_SUCCESS;

cleanup:
    Disable(base);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32n6_CrypAesCtr_Process(whal_AesCtr *dev,
                                           const void *in, void *out,
                                           size_t sz)
{
    (void)dev;
    return Process_BlockCipher(in, out, sz);
}

const whal_AesCtrDriver whal_Stm32n6_Cryp_CtrDriver = {
    .Oneshot = whal_Stm32n6_CrypAesCtr_Oneshot,
    .Start = whal_Stm32n6_CrypAesCtr_Start,
    .Process = whal_Stm32n6_CrypAesCtr_Process,
};


/* ---- GCM helpers ---- */

/*
 * Run GCM init phase: configure CR for GCM, load IV (12 bytes + counter=2)
 * and key, then enable CRYP and wait for the hash subkey computation to
 * complete (CRYPEN auto-clears).
 */
static whal_Error GcmInit(const uint8_t *key, size_t keySz,
                          uint32_t keySizeBits, uint32_t algoDir,
                          const uint8_t *iv12)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    whal_Error err;

    DisableAndFlush(base);
    ConfigureMode(base, CRYP_ALGOMODE_AES_GCM, algoDir, keySizeBits,
                  CRYP_GCM_CCMPH_INIT, 0, 0);
    whal_Reg_Write(base, CRYP_IV0LR_REG, whal_LoadBe32(iv12));
    whal_Reg_Write(base, CRYP_IV0RR_REG, whal_LoadBe32(iv12 + 4));
    whal_Reg_Write(base, CRYP_IV1LR_REG, whal_LoadBe32(iv12 + 8));
    whal_Reg_Write(base, CRYP_IV1RR_REG, 0x00000002UL);
    WriteKey(base, key, keySz);
    err = WaitKeyValid(base, cfg->timeout);
    if (err)
        return err;
    Enable(base);
    return WaitCrypEnClear(base, cfg->timeout);
}

/*
 * Feed AAD blocks during the header phase. The peripheral consumes header
 * data without producing output; the last partial block must be zero-padded.
 */
static whal_Error GcmHeaderPhase(const uint8_t *aad, size_t aadSz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    whal_Error err;
    size_t i;

    if (aadSz == 0)
        return WHAL_SUCCESS;

    whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_GCM_CCMPH_Msk,
                    whal_SetBits(CRYP_CR_GCM_CCMPH_Msk,
                                 CRYP_CR_GCM_CCMPH_Pos,
                                 CRYP_GCM_CCMPH_HEADER));
    Enable(base);

    for (i = 0; i < aadSz; i += 16) {
        size_t remain = aadSz - i;
        if (remain >= 16) {
            WriteBlock(base, aad + i);
        } else {
            uint8_t pad[16] = {0};
            size_t j;
            for (j = 0; j < remain; j++)
                pad[j] = aad[i + j];
            WriteBlock(base, pad);
        }
    }

    err = WaitBusyClear(base, cfg->timeout);
    if (err)
        return err;
    return WHAL_SUCCESS;
}


/* ---- AES-GCM ---- */

whal_Error whal_Stm32n6_CrypAesGcm_Oneshot(whal_AesGcm *dev,
                                           whal_Crypto_Dir dir,
                                           const void *key, size_t keySz,
                                           const void *iv, size_t ivSz,
                                           const void *aad, size_t aadSz,
                                           const void *in, void *out,
                                           size_t sz,
                                           void *tag, size_t tagSz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint32_t keySizeBits;
    uint32_t algoDir;
    uint8_t tagBuf[16];
    uint64_t aadBits;
    uint64_t payloadBits;
    whal_Error err;
    size_t i;
    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;
    if (ivSz != 12)
        return WHAL_ENOTSUP;
    if (aadSz > 0 && !aad)
        return WHAL_EINVAL;
    if (sz > 0 && (!in || !out))
        return WHAL_EINVAL;
    if (!tag || tagSz == 0 || tagSz > 16)
        return WHAL_EINVAL;

    err = KeySizeBits(keySz, &keySizeBits);
    if (err)
        return err;

    algoDir = (dir == WHAL_CRYPTO_ENCRYPT) ? CRYP_ALGODIR_ENCRYPT
                                           : CRYP_ALGODIR_DECRYPT;

    /* Init phase */
    err = GcmInit((const uint8_t *)key, keySz,
                  keySizeBits, algoDir, (const uint8_t *)iv);
    if (err)
        goto cleanup;

    /* Header phase */
    err = GcmHeaderPhase((const uint8_t *)aad, aadSz);
    if (err)
        goto cleanup;

    /* Payload phase */
    if (sz > 0) {
        Disable(base);
        whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_GCM_CCMPH_Msk,
                        whal_SetBits(CRYP_CR_GCM_CCMPH_Msk,
                                     CRYP_CR_GCM_CCMPH_Pos,
                                     CRYP_GCM_CCMPH_PAYLOAD));
        Enable(base);

        for (i = 0; i < sz; i += 16) {
            const uint8_t *inPtr = (const uint8_t *)in + i;
            uint8_t *outPtr = (uint8_t *)out + i;
            size_t remain = sz - i;
            uint8_t blockIn[16] = {0};
            uint8_t blockOut[16];
            size_t j;

            if (remain >= 16) {
                WriteBlock(base, inPtr);
            } else {
                if (dir == WHAL_CRYPTO_ENCRYPT) {
                    whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_NPBLB_Msk,
                                    whal_SetBits(CRYP_CR_NPBLB_Msk,
                                                 CRYP_CR_NPBLB_Pos,
                                                 16 - remain));
                }
                for (j = 0; j < remain; j++)
                    blockIn[j] = inPtr[j];
                WriteBlock(base, blockIn);
            }

            err = WaitOutputReady(base, cfg->timeout);
            if (err)
                goto cleanup;

            if (remain >= 16) {
                ReadBlock(base, outPtr);
            } else {
                ReadBlock(base, blockOut);
                for (j = 0; j < remain; j++)
                    outPtr[j] = blockOut[j];
            }
        }
    }

    /* Final phase */
    Disable(base);
    whal_Reg_Update(base, CRYP_CR_REG,
                    CRYP_CR_GCM_CCMPH_Msk | CRYP_CR_ALGODIR_Msk,
                    whal_SetBits(CRYP_CR_GCM_CCMPH_Msk,
                                 CRYP_CR_GCM_CCMPH_Pos,
                                 CRYP_GCM_CCMPH_FINAL));
    Enable(base);

    aadBits = (uint64_t)aadSz * 8;
    payloadBits = (uint64_t)sz * 8;
    whal_Reg_Write(base, CRYP_DINR_REG, (uint32_t)(aadBits >> 32));
    whal_Reg_Write(base, CRYP_DINR_REG, (uint32_t)aadBits);
    whal_Reg_Write(base, CRYP_DINR_REG, (uint32_t)(payloadBits >> 32));
    whal_Reg_Write(base, CRYP_DINR_REG, (uint32_t)payloadBits);

    err = WaitOutputReady(base, cfg->timeout);
    if (err)
        goto cleanup;

    ReadBlock(base, tagBuf);
    for (i = 0; i < tagSz; i++)
        ((uint8_t *)tag)[i] = tagBuf[i];

cleanup:
    Disable(base);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32n6_CrypAesGcm_Start(whal_AesGcm *dev,
                                         whal_Crypto_Dir dir,
                                         const void *key, size_t keySz,
                                         const void *iv, size_t ivSz,
                                         const void *aad, size_t aadSz)
{
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint32_t keySizeBits;
    uint32_t algoDir;
    whal_Error err;
    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;
    if (ivSz != 12)
        return WHAL_ENOTSUP;
    if (aadSz > 0 && !aad)
        return WHAL_EINVAL;

    err = KeySizeBits(keySz, &keySizeBits);
    if (err)
        return err;

    algoDir = (dir == WHAL_CRYPTO_ENCRYPT) ? CRYP_ALGODIR_ENCRYPT
                                           : CRYP_ALGODIR_DECRYPT;

    /* Init phase */
    err = GcmInit((const uint8_t *)key, keySz,
                  keySizeBits, algoDir, (const uint8_t *)iv);
    if (err)
        goto cleanup;

    /* Header phase */
    err = GcmHeaderPhase((const uint8_t *)aad, aadSz);
    if (err)
        goto cleanup;

    /* Transition to payload phase */
    Disable(base);
    whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_GCM_CCMPH_Msk,
                    whal_SetBits(CRYP_CR_GCM_CCMPH_Msk,
                                 CRYP_CR_GCM_CCMPH_Pos,
                                 CRYP_GCM_CCMPH_PAYLOAD));
    Enable(base);

    g_aesGcmState.aadSz = aadSz;
    g_aesGcmState.dataSz = 0;

    return WHAL_SUCCESS;

cleanup:
    Disable(base);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32n6_CrypAesGcm_Process(whal_AesGcm *dev,
                                           const void *in, void *out,
                                           size_t sz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint32_t algoDir;
    size_t i;
    whal_Error err;
    (void)dev;

    if (sz == 0)
        return WHAL_SUCCESS;

    if (!in || !out) {
        Disable(base);
        ZeroKeyIv(base);
        return WHAL_EINVAL;
    }

    algoDir = whal_GetBits(CRYP_CR_ALGODIR_Msk, CRYP_CR_ALGODIR_Pos,
                           whal_Reg_Read(base, CRYP_CR_REG));

    for (i = 0; i < sz; i += 16) {
        const uint8_t *inPtr = (const uint8_t *)in + i;
        uint8_t *outPtr = (uint8_t *)out + i;
        size_t remain = sz - i;
        uint8_t block[16] = {0};
        size_t j;

        if (remain >= 16) {
            WriteBlock(base, inPtr);
        } else {
            if (algoDir == CRYP_ALGODIR_ENCRYPT) {
                whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_NPBLB_Msk,
                                whal_SetBits(CRYP_CR_NPBLB_Msk,
                                             CRYP_CR_NPBLB_Pos,
                                             16 - remain));
            }
            for (j = 0; j < remain; j++)
                block[j] = inPtr[j];
            WriteBlock(base, block);
        }

        err = WaitOutputReady(base, cfg->timeout);
        if (err) {
            Disable(base);
            ZeroKeyIv(base);
            return err;
        }

        if (remain >= 16) {
            ReadBlock(base, outPtr);
        } else {
            ReadBlock(base, block);
            for (j = 0; j < remain; j++)
                outPtr[j] = block[j];
        }
    }

    g_aesGcmState.dataSz += sz;

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32n6_CrypAesGcm_Finalize(whal_AesGcm *dev,
                                            void *tag, size_t tagSz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint8_t tagBuf[16];
    uint64_t aadBits;
    uint64_t payloadBits;
    size_t i;
    whal_Error err;
    (void)dev;

    if (!tag || tagSz == 0 || tagSz > 16) {
        Disable(base);
        ZeroKeyIv(base);
        return WHAL_EINVAL;
    }

    /* Final phase */
    Disable(base);
    whal_Reg_Update(base, CRYP_CR_REG,
                    CRYP_CR_GCM_CCMPH_Msk | CRYP_CR_ALGODIR_Msk,
                    whal_SetBits(CRYP_CR_GCM_CCMPH_Msk,
                                 CRYP_CR_GCM_CCMPH_Pos,
                                 CRYP_GCM_CCMPH_FINAL));
    Enable(base);

    aadBits = (uint64_t)g_aesGcmState.aadSz * 8;
    payloadBits = (uint64_t)g_aesGcmState.dataSz * 8;
    whal_Reg_Write(base, CRYP_DINR_REG, (uint32_t)(aadBits >> 32));
    whal_Reg_Write(base, CRYP_DINR_REG, (uint32_t)aadBits);
    whal_Reg_Write(base, CRYP_DINR_REG, (uint32_t)(payloadBits >> 32));
    whal_Reg_Write(base, CRYP_DINR_REG, (uint32_t)payloadBits);

    err = WaitOutputReady(base, cfg->timeout);
    if (err) {
        Disable(base);
        ZeroKeyIv(base);
        return err;
    }

    ReadBlock(base, tagBuf);
    for (i = 0; i < tagSz; i++)
        ((uint8_t *)tag)[i] = tagBuf[i];

    Disable(base);
    ZeroKeyIv(base);
    return WHAL_SUCCESS;
}

const whal_AesGcmDriver whal_Stm32n6_Cryp_GcmDriver = {
    .Oneshot = whal_Stm32n6_CrypAesGcm_Oneshot,
    .Start = whal_Stm32n6_CrypAesGcm_Start,
    .Process = whal_Stm32n6_CrypAesGcm_Process,
    .Finalize = whal_Stm32n6_CrypAesGcm_Finalize,
};


/* ---- AES-GMAC ---- */

whal_Error whal_Stm32n6_CrypAesGmac_Oneshot(whal_AesGmac *dev,
                                            const void *key, size_t keySz,
                                            const void *iv, size_t ivSz,
                                            const void *aad, size_t aadSz,
                                            void *tag, size_t tagSz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint32_t keySizeBits;
    uint8_t tagBuf[16];
    uint64_t aadBits;
    whal_Error err;
    size_t i;
    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;
    if (ivSz != 12)
        return WHAL_ENOTSUP;
    if (aadSz > 0 && !aad)
        return WHAL_EINVAL;
    if (!tag || tagSz == 0 || tagSz > 16)
        return WHAL_EINVAL;

    err = KeySizeBits(keySz, &keySizeBits);
    if (err)
        return err;

    /* Init phase */
    err = GcmInit((const uint8_t *)key, keySz,
                  keySizeBits, CRYP_ALGODIR_ENCRYPT,
                  (const uint8_t *)iv);
    if (err)
        goto cleanup;

    /* Header phase */
    err = GcmHeaderPhase((const uint8_t *)aad, aadSz);
    if (err)
        goto cleanup;

    /* Final phase */
    Disable(base);
    whal_Reg_Update(base, CRYP_CR_REG,
                    CRYP_CR_GCM_CCMPH_Msk | CRYP_CR_ALGODIR_Msk,
                    whal_SetBits(CRYP_CR_GCM_CCMPH_Msk,
                                 CRYP_CR_GCM_CCMPH_Pos,
                                 CRYP_GCM_CCMPH_FINAL));
    Enable(base);

    aadBits = (uint64_t)aadSz * 8;
    whal_Reg_Write(base, CRYP_DINR_REG, (uint32_t)(aadBits >> 32));
    whal_Reg_Write(base, CRYP_DINR_REG, (uint32_t)aadBits);
    whal_Reg_Write(base, CRYP_DINR_REG, 0);
    whal_Reg_Write(base, CRYP_DINR_REG, 0);

    err = WaitOutputReady(base, cfg->timeout);
    if (err)
        goto cleanup;

    ReadBlock(base, tagBuf);
    for (i = 0; i < tagSz; i++)
        ((uint8_t *)tag)[i] = tagBuf[i];

cleanup:
    Disable(base);
    ZeroKeyIv(base);
    return err;
}

const whal_AesGmacDriver whal_Stm32n6_Cryp_GmacDriver = {
    .Oneshot = whal_Stm32n6_CrypAesGmac_Oneshot,
};


/* ---- AES-CCM ---- */

/*
 * Build the 16-byte CCM B0 first authentication block from the user nonce,
 * tag length, and message length per NIST SP 800-38C Appendix A.
 */
static void CcmBuildB0(const uint8_t *nonce, size_t nonceSz, size_t tagSz,
                       size_t msgSz, int hasAad, uint8_t *b0)
{
    size_t q = 15 - nonceSz;
    size_t i;
    size_t msg = msgSz;

    b0[0] = (uint8_t)((hasAad ? 0x40 : 0) |
                      (((tagSz - 2) / 2) << 3) |
                      (q - 1));
    for (i = 0; i < nonceSz; i++)
        b0[1 + i] = nonce[i];
    for (i = 0; i < q; i++) {
        b0[15 - i] = (uint8_t)(msg & 0xFF);
        msg >>= 8;
    }
}

whal_Error whal_Stm32n6_CrypAesCcm_Oneshot(whal_AesCcm *dev,
                                           whal_Crypto_Dir dir,
                                           const void *key, size_t keySz,
                                           const void *nonce, size_t nonceSz,
                                           const void *aad, size_t aadSz,
                                           const void *in, void *out,
                                           size_t sz,
                                           void *tag, size_t tagSz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint32_t keySizeBits;
    uint32_t algoDir;
    uint8_t b0[16];
    uint8_t ctr0[16];
    uint8_t ctr1[16];
    uint8_t tagBuf[16];
    size_t q;
    size_t i;
    whal_Error err;
    (void)dev;

    if (!key || !nonce)
        return WHAL_EINVAL;
    if (nonceSz < 7 || nonceSz > 13)
        return WHAL_EINVAL;
    if (tagSz < 4 || tagSz > 16 || (tagSz & 1) != 0)
        return WHAL_EINVAL;
    if (aadSz > 0 && !aad)
        return WHAL_EINVAL;
    if (sz > 0 && (!in || !out))
        return WHAL_EINVAL;
    if (!tag)
        return WHAL_EINVAL;

    err = KeySizeBits(keySz, &keySizeBits);
    if (err)
        return err;

    algoDir = (dir == WHAL_CRYPTO_ENCRYPT) ? CRYP_ALGODIR_ENCRYPT
                                           : CRYP_ALGODIR_DECRYPT;

    q = 15 - nonceSz;
    CcmBuildB0((const uint8_t *)nonce, nonceSz, tagSz,
               sz, aadSz > 0, b0);

    /* CTR0 = B0 with the top-5 flag bits cleared and the message-length
     * bytes (last q bytes) zeroed. Replayed in the final phase below. */
    for (i = 0; i < 16; i++)
        ctr0[i] = b0[i];
    ctr0[0] &= 0x07;
    for (i = 16 - q; i < 16; i++)
        ctr0[i] = 0;

    /* CTR1 = CTR0 with bit 0 set (counter = 1). Per RM0486 Table 421 this
     * is what the IV registers receive at init time. */
    for (i = 0; i < 16; i++)
        ctr1[i] = ctr0[i];
    ctr1[15] |= 0x01;

    /* Init phase */
    DisableAndFlush(base);
    ConfigureMode(base, CRYP_ALGOMODE_AES_CCM, algoDir, keySizeBits,
                  CRYP_GCM_CCMPH_INIT, 0, 0);
    WriteIv16(base, ctr1);
    WriteKey(base, key, keySz);
    err = WaitKeyValid(base, cfg->timeout);
    if (err)
        goto cleanup;
    Enable(base);

    /* Feed B0 to start the CBC-MAC; CRYPEN auto-clears when init completes. */
    WriteBlock(base, b0);
    err = WaitCrypEnClear(base, cfg->timeout);
    if (err)
        goto cleanup;

    /* Header phase (AAD) */
    if (aadSz > 0) {
        const uint8_t *aadPtr = (const uint8_t *)aad;
        uint8_t hdr[16] = {0};
        size_t hdrOff;
        size_t aadOff = 0;

        whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_GCM_CCMPH_Msk,
                        whal_SetBits(CRYP_CR_GCM_CCMPH_Msk,
                                     CRYP_CR_GCM_CCMPH_Pos,
                                     CRYP_GCM_CCMPH_HEADER));
        Enable(base);

        hdr[0] = (uint8_t)(aadSz >> 8);
        hdr[1] = (uint8_t)aadSz;
        hdrOff = 2;
        while (hdrOff < 16 && aadOff < aadSz)
            hdr[hdrOff++] = aadPtr[aadOff++];
        WriteBlock(base, hdr);

        while (aadOff < aadSz) {
            uint8_t blk[16] = {0};
            size_t j;
            for (j = 0; j < 16 && aadOff < aadSz; j++)
                blk[j] = aadPtr[aadOff++];
            WriteBlock(base, blk);
        }

        err = WaitBusyClear(base, cfg->timeout);
        if (err)
            goto cleanup;
    }

    /* Payload phase */
    if (sz > 0) {
        Disable(base);
        whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_GCM_CCMPH_Msk,
                        whal_SetBits(CRYP_CR_GCM_CCMPH_Msk,
                                     CRYP_CR_GCM_CCMPH_Pos,
                                     CRYP_GCM_CCMPH_PAYLOAD));
        Enable(base);

        for (i = 0; i < sz; i += 16) {
            const uint8_t *inPtr = (const uint8_t *)in + i;
            uint8_t *outPtr = (uint8_t *)out + i;
            size_t remain = sz - i;
            uint8_t blockIn[16] = {0};
            uint8_t blockOut[16];
            size_t j;

            if (remain >= 16) {
                WriteBlock(base, inPtr);
            } else {
                if (dir == WHAL_CRYPTO_DECRYPT) {
                    whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_NPBLB_Msk,
                                    whal_SetBits(CRYP_CR_NPBLB_Msk,
                                                 CRYP_CR_NPBLB_Pos,
                                                 16 - remain));
                }
                for (j = 0; j < remain; j++)
                    blockIn[j] = inPtr[j];
                WriteBlock(base, blockIn);
            }

            err = WaitOutputReady(base, cfg->timeout);
            if (err)
                goto cleanup;

            if (remain >= 16) {
                ReadBlock(base, outPtr);
            } else {
                ReadBlock(base, blockOut);
                for (j = 0; j < remain; j++)
                    outPtr[j] = blockOut[j];
            }
        }
    }

    /* Final phase */
    Disable(base);
    whal_Reg_Update(base, CRYP_CR_REG,
                    CRYP_CR_GCM_CCMPH_Msk | CRYP_CR_ALGODIR_Msk,
                    whal_SetBits(CRYP_CR_GCM_CCMPH_Msk,
                                 CRYP_CR_GCM_CCMPH_Pos,
                                 CRYP_GCM_CCMPH_FINAL));
    Enable(base);

    WriteBlock(base, ctr0);

    err = WaitOutputReady(base, cfg->timeout);
    if (err)
        goto cleanup;

    ReadBlock(base, tagBuf);
    for (i = 0; i < tagSz; i++)
        ((uint8_t *)tag)[i] = tagBuf[i];

cleanup:
    Disable(base);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32n6_CrypAesCcm_Start(whal_AesCcm *dev,
                                         whal_Crypto_Dir dir,
                                         const void *key, size_t keySz,
                                         const void *nonce, size_t nonceSz,
                                         const void *aad, size_t aadSz,
                                         size_t tagSz, size_t sz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint32_t keySizeBits;
    uint32_t algoDir;
    uint8_t b0[16];
    uint8_t ctr1[16];
    size_t q;
    size_t i;
    whal_Error err;
    (void)dev;

    if (!key || !nonce)
        return WHAL_EINVAL;
    if (nonceSz < 7 || nonceSz > 13)
        return WHAL_EINVAL;
    if (tagSz < 4 || tagSz > 16 || (tagSz & 1) != 0)
        return WHAL_EINVAL;
    if (aadSz > 0 && !aad)
        return WHAL_EINVAL;

    err = KeySizeBits(keySz, &keySizeBits);
    if (err)
        return err;

    algoDir = (dir == WHAL_CRYPTO_ENCRYPT) ? CRYP_ALGODIR_ENCRYPT
                                           : CRYP_ALGODIR_DECRYPT;

    q = 15 - nonceSz;
    CcmBuildB0((const uint8_t *)nonce, nonceSz, tagSz,
               sz, aadSz > 0, b0);

    for (i = 0; i < 16; i++)
        g_aesCcmState.ccmCtr0[i] = b0[i];
    g_aesCcmState.ccmCtr0[0] &= 0x07;
    for (i = 16 - q; i < 16; i++)
        g_aesCcmState.ccmCtr0[i] = 0;

    for (i = 0; i < 16; i++)
        ctr1[i] = g_aesCcmState.ccmCtr0[i];
    ctr1[15] |= 0x01;

    /* Init phase */
    DisableAndFlush(base);
    ConfigureMode(base, CRYP_ALGOMODE_AES_CCM, algoDir, keySizeBits,
                  CRYP_GCM_CCMPH_INIT, 0, 0);
    WriteIv16(base, ctr1);
    WriteKey(base, key, keySz);
    err = WaitKeyValid(base, cfg->timeout);
    if (err)
        goto cleanup;
    Enable(base);

    WriteBlock(base, b0);
    err = WaitCrypEnClear(base, cfg->timeout);
    if (err)
        goto cleanup;

    /* Header phase (AAD) */
    if (aadSz > 0) {
        const uint8_t *aadPtr = (const uint8_t *)aad;
        uint8_t hdr[16] = {0};
        size_t hdrOff;
        size_t aadOff = 0;

        whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_GCM_CCMPH_Msk,
                        whal_SetBits(CRYP_CR_GCM_CCMPH_Msk,
                                     CRYP_CR_GCM_CCMPH_Pos,
                                     CRYP_GCM_CCMPH_HEADER));
        Enable(base);

        hdr[0] = (uint8_t)(aadSz >> 8);
        hdr[1] = (uint8_t)aadSz;
        hdrOff = 2;
        while (hdrOff < 16 && aadOff < aadSz)
            hdr[hdrOff++] = aadPtr[aadOff++];
        WriteBlock(base, hdr);

        while (aadOff < aadSz) {
            uint8_t blk[16] = {0};
            size_t j;
            for (j = 0; j < 16 && aadOff < aadSz; j++)
                blk[j] = aadPtr[aadOff++];
            WriteBlock(base, blk);
        }

        err = WaitBusyClear(base, cfg->timeout);
        if (err)
            goto cleanup;
    }

    /* Transition to payload phase */
    Disable(base);
    whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_GCM_CCMPH_Msk,
                    whal_SetBits(CRYP_CR_GCM_CCMPH_Msk,
                                 CRYP_CR_GCM_CCMPH_Pos,
                                 CRYP_GCM_CCMPH_PAYLOAD));
    Enable(base);

    g_aesCcmState.aadSz = aadSz;
    g_aesCcmState.dataSz = 0;

    return WHAL_SUCCESS;

cleanup:
    Disable(base);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32n6_CrypAesCcm_Process(whal_AesCcm *dev,
                                           const void *in, void *out,
                                           size_t sz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint32_t algoDir;
    size_t i;
    whal_Error err;
    (void)dev;

    if (sz == 0)
        return WHAL_SUCCESS;

    if (!in || !out) {
        Disable(base);
        ZeroKeyIv(base);
        return WHAL_EINVAL;
    }

    algoDir = whal_GetBits(CRYP_CR_ALGODIR_Msk, CRYP_CR_ALGODIR_Pos,
                           whal_Reg_Read(base, CRYP_CR_REG));

    for (i = 0; i < sz; i += 16) {
        const uint8_t *inPtr = (const uint8_t *)in + i;
        uint8_t *outPtr = (uint8_t *)out + i;
        size_t remain = sz - i;
        uint8_t block[16] = {0};
        uint8_t blockOut[16];
        size_t j;

        if (remain >= 16) {
            WriteBlock(base, inPtr);
        } else {
            if (algoDir == CRYP_ALGODIR_DECRYPT) {
                whal_Reg_Update(base, CRYP_CR_REG, CRYP_CR_NPBLB_Msk,
                                whal_SetBits(CRYP_CR_NPBLB_Msk,
                                             CRYP_CR_NPBLB_Pos,
                                             16 - remain));
            }
            for (j = 0; j < remain; j++)
                block[j] = inPtr[j];
            WriteBlock(base, block);
        }

        err = WaitOutputReady(base, cfg->timeout);
        if (err) {
            Disable(base);
            ZeroKeyIv(base);
            return err;
        }

        if (remain >= 16) {
            ReadBlock(base, outPtr);
        } else {
            ReadBlock(base, blockOut);
            for (j = 0; j < remain; j++)
                outPtr[j] = blockOut[j];
        }
    }

    g_aesCcmState.dataSz += sz;

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32n6_CrypAesCcm_Finalize(whal_AesCcm *dev,
                                            void *tag, size_t tagSz)
{
    const whal_Stm32n6_Cryp_Cfg *cfg =
        (const whal_Stm32n6_Cryp_Cfg *)whal_Stm32n6_Cryp_Dev.cfg;
    size_t base = whal_Stm32n6_Cryp_Dev.base;
    uint8_t tagBuf[16];
    size_t i;
    whal_Error err;
    (void)dev;

    if (!tag || tagSz < 4 || tagSz > 16 || (tagSz & 1) != 0) {
        Disable(base);
        ZeroKeyIv(base);
        return WHAL_EINVAL;
    }

    /* Final phase */
    Disable(base);
    whal_Reg_Update(base, CRYP_CR_REG,
                    CRYP_CR_GCM_CCMPH_Msk | CRYP_CR_ALGODIR_Msk,
                    whal_SetBits(CRYP_CR_GCM_CCMPH_Msk,
                                 CRYP_CR_GCM_CCMPH_Pos,
                                 CRYP_GCM_CCMPH_FINAL));
    Enable(base);

    WriteBlock(base, g_aesCcmState.ccmCtr0);

    err = WaitOutputReady(base, cfg->timeout);
    if (err) {
        Disable(base);
        ZeroKeyIv(base);
        return err;
    }

    ReadBlock(base, tagBuf);
    for (i = 0; i < tagSz; i++)
        ((uint8_t *)tag)[i] = tagBuf[i];

    Disable(base);
    ZeroKeyIv(base);
    return WHAL_SUCCESS;
}

const whal_AesCcmDriver whal_Stm32n6_Cryp_CcmDriver = {
    .Oneshot = whal_Stm32n6_CrypAesCcm_Oneshot,
    .Start = whal_Stm32n6_CrypAesCcm_Start,
    .Process = whal_Stm32n6_CrypAesCcm_Process,
    .Finalize = whal_Stm32n6_CrypAesCcm_Finalize,
};
