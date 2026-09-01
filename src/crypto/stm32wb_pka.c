/* stm32wb_pka.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32WB_PKA_DEV initializer */
#include <wolfHAL/crypto/stm32wb_pka.h>
#include <wolfHAL/crypto/crypto.h>
#include <wolfHAL/crypto/pka.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/timeout.h>

/* Direct-API mapping for the math primitives: when the board defines
 * WHAL_CFG_STM32WB_PKA_DIRECT_API_MAPPING, the platform-prefixed function
 * names below get preprocessor-substituted to the generic whal_Pka_* names
 * declared in <wolfHAL/crypto/pka.h>, so the binary exports those names. */
#ifdef WHAL_CFG_STM32WB_PKA_DIRECT_API_MAPPING
#define whal_Stm32wb_Pka_ModExp    whal_Pka_ModExp
#define whal_Stm32wb_Pka_ModInv    whal_Pka_ModInv
#define whal_Stm32wb_Pka_ModReduce whal_Pka_ModReduce
#define whal_Stm32wb_Pka_IntMul    whal_Pka_IntMul
#define whal_Stm32wb_Pka_IntSub    whal_Pka_IntSub
#define whal_Stm32wb_Pka_RsaCrtExp whal_Pka_RsaCrtExp
#endif

const whal_Crypto whal_Stm32wb_Pka_Dev = WHAL_CFG_STM32WB_PKA_DEV;

/* ---- PKA register definitions ---- */

/* Control Register */
#define PKA_CR_REG           0x00
#define PKA_CR_EN_Pos        0
#define PKA_CR_EN_Msk        (1UL << PKA_CR_EN_Pos)
#define PKA_CR_START_Pos     1
#define PKA_CR_START_Msk     (1UL << PKA_CR_START_Pos)
#define PKA_CR_MODE_Pos      8
#define PKA_CR_MODE_Msk      (0x3FUL << PKA_CR_MODE_Pos)
#define PKA_CR_PROCENDIE_Pos 17
#define PKA_CR_PROCENDIE_Msk (1UL << PKA_CR_PROCENDIE_Pos)
#define PKA_CR_RAMERRIE_Pos  19
#define PKA_CR_RAMERRIE_Msk  (1UL << PKA_CR_RAMERRIE_Pos)
#define PKA_CR_ADDRERRIE_Pos 20
#define PKA_CR_ADDRERRIE_Msk (1UL << PKA_CR_ADDRERRIE_Pos)

/* Status Register */
#define PKA_SR_REG           0x04
#define PKA_SR_BUSY_Pos      16
#define PKA_SR_BUSY_Msk      (1UL << PKA_SR_BUSY_Pos)
#define PKA_SR_PROCENDF_Pos  17
#define PKA_SR_PROCENDF_Msk  (1UL << PKA_SR_PROCENDF_Pos)
#define PKA_SR_RAMERRF_Pos   19
#define PKA_SR_RAMERRF_Msk   (1UL << PKA_SR_RAMERRF_Pos)
#define PKA_SR_ADDRERRF_Pos  20
#define PKA_SR_ADDRERRF_Msk  (1UL << PKA_SR_ADDRERRF_Pos)

/* Clear Flag Register */
#define PKA_CLRFR_REG            0x08
#define PKA_CLRFR_PROCENDFC_Pos  17
#define PKA_CLRFR_PROCENDFC_Msk  (1UL << PKA_CLRFR_PROCENDFC_Pos)
#define PKA_CLRFR_RAMERRFC_Pos   19
#define PKA_CLRFR_RAMERRFC_Msk   (1UL << PKA_CLRFR_RAMERRFC_Pos)
#define PKA_CLRFR_ADDRERRFC_Pos  20
#define PKA_CLRFR_ADDRERRFC_Msk  (1UL << PKA_CLRFR_ADDRERRFC_Pos)

/* Operating modes (PKA_CR.MODE values) */
#define PKA_MODE_MODEXP_NORMAL       0x00
#define PKA_MODE_MONTGOMERY_PARAM    0x01
#define PKA_MODE_MODEXP_FAST         0x02
#define PKA_MODE_RSA_CRT_EXP         0x07
#define PKA_MODE_MODINV              0x08
#define PKA_MODE_INT_ADD             0x09
#define PKA_MODE_INT_SUB             0x0A
#define PKA_MODE_INT_MUL             0x0B
#define PKA_MODE_INT_CMP             0x0C
#define PKA_MODE_MOD_REDUCE          0x0D
#define PKA_MODE_MOD_ADD             0x0E
#define PKA_MODE_MOD_SUB             0x0F
#define PKA_MODE_MONTGOMERY_MUL      0x10
#define PKA_MODE_ECC_SCALAR_MUL      0x20
#define PKA_MODE_ECC_SCALAR_MUL_FAST 0x22
#define PKA_MODE_ECDSA_SIGN          0x24
#define PKA_MODE_ECDSA_VERIFY        0x26
#define PKA_MODE_ECC_POINT_CHECK     0x28

/* ---- PKA RAM operand offsets (from PKA base address) ---- */

/* MODE 0x00 — Modular exponentiation (normal) */
#define PKA_MODEXP_EXP_LEN        0x400
#define PKA_MODEXP_OP_LEN         0x404
#define PKA_MODEXP_A              0xA44
#define PKA_MODEXP_E              0xBD0
#define PKA_MODEXP_N              0xD5C
#define PKA_MODEXP_RESULT         0x724

/* MODE 0x01 — Montgomery parameter (R^2 mod n) */
#define PKA_MONT_PARAM_MOD_LEN    0x404
#define PKA_MONT_PARAM_N          0xD5C
#define PKA_MONT_PARAM_R2         0x594

/* MODE 0x02 — Modular exponentiation (fast, precomputed R^2) */
#define PKA_MODEXP_FAST_EXP_LEN   0x400
#define PKA_MODEXP_FAST_OP_LEN    0x404
#define PKA_MODEXP_FAST_A         0xA44
#define PKA_MODEXP_FAST_E         0xBD0
#define PKA_MODEXP_FAST_N         0xD5C
#define PKA_MODEXP_FAST_R2        0x594
#define PKA_MODEXP_FAST_RESULT    0x724

/* MODE 0x07 — RSA CRT exponentiation */
#define PKA_RSA_CRT_OP_LEN        0x404
#define PKA_RSA_CRT_DP            0x65C
#define PKA_RSA_CRT_DQ            0xBD0
#define PKA_RSA_CRT_QINV          0x7EC
#define PKA_RSA_CRT_P             0x97C
#define PKA_RSA_CRT_Q             0xD5C
#define PKA_RSA_CRT_A             0xEEC
#define PKA_RSA_CRT_RESULT        0x724

/* MODE 0x08 — Modular inversion */
#define PKA_MODINV_OP_LEN         0x404
#define PKA_MODINV_A              0x8B4
#define PKA_MODINV_N              0xA44
#define PKA_MODINV_RESULT         0xBD0

/* MODE 0x0A — Arithmetic subtraction */
#define PKA_INT_SUB_OP_LEN        0x404
#define PKA_INT_SUB_A             0x8B4
#define PKA_INT_SUB_B             0xA44
#define PKA_INT_SUB_RESULT        0xBD0

/* MODE 0x0B — Arithmetic multiplication */
#define PKA_INT_MUL_OP_LEN        0x404
#define PKA_INT_MUL_A             0x8B4
#define PKA_INT_MUL_B             0xA44
#define PKA_INT_MUL_RESULT        0xBD0

/* MODE 0x0D — Modular reduction */
#define PKA_MOD_REDUCE_OP_LEN     0x400
#define PKA_MOD_REDUCE_MOD_LEN    0x404
#define PKA_MOD_REDUCE_A          0x8B4
#define PKA_MOD_REDUCE_N          0xA44
#define PKA_MOD_REDUCE_RESULT     0xBD0

/* ---- Internal helpers ---- */

/* Write a big-endian integer into the PKA RAM at offset as a little-endian
 * 32-bit word array, with the mandatory trailing zero word. */
static void WriteOperand(size_t base, size_t offset,
                         const uint8_t *data, size_t dataSz)
{
    size_t numWords = (dataSz + 3) / 4;
    size_t i, b;

    for (i = 0; i < numWords; i++) {
        uint32_t word = 0;
        for (b = 0; b < 4; b++) {
            size_t bytePos = i * 4 + b;
            if (bytePos < dataSz)
                word |= (uint32_t)data[dataSz - 1 - bytePos] << (b * 8);
        }
        whal_Reg_Write(base, offset + i * 4, word);
    }
    whal_Reg_Write(base, offset + numWords * 4, 0);
}

/* Read a little-endian word array from PKA RAM into a big-endian byte array. */
static void ReadOperand(size_t base, size_t offset,
                        uint8_t *data, size_t dataSz)
{
    size_t numWords = (dataSz + 3) / 4;
    size_t i, b;

    for (i = 0; i < numWords; i++) {
        uint32_t word = whal_Reg_Read(base, offset + i * 4);
        for (b = 0; b < 4; b++) {
            size_t bytePos = i * 4 + b;
            if (bytePos < dataSz)
                data[dataSz - 1 - bytePos] = (uint8_t)(word >> (b * 8));
        }
    }
}

/* Zero the slot footprint of a prior WriteOperand/ReadOperand call: the
 * numWords data words plus the one trailing word. Called on every op's
 * success path so the PKA RAM never carries stale operand or result bytes
 * into the next operation. */
static void ZeroOperand(size_t base, size_t offset, size_t dataSz)
{
    size_t numWords = (dataSz + 3) / 4;
    size_t i;

    for (i = 0; i <= numWords; i++)
        whal_Reg_Write(base, offset + i * 4, 0);
}

static whal_Error WaitForProcEnd(size_t base, whal_Timeout *timeout)
{
    const uint32_t errFlags  = PKA_SR_ADDRERRF_Msk | PKA_SR_RAMERRF_Msk;
    const uint32_t doneFlags = errFlags | PKA_SR_PROCENDF_Msk;
    const uint32_t clrFlags  = PKA_CLRFR_ADDRERRFC_Msk |
                               PKA_CLRFR_RAMERRFC_Msk |
                               PKA_CLRFR_PROCENDFC_Msk;

    WHAL_TIMEOUT_START(timeout);

    while (1) {
        uint32_t sr = whal_Reg_Read(base, PKA_SR_REG);
        if (sr & doneFlags) {
            whal_Reg_Write(base, PKA_CLRFR_REG, clrFlags);
            return (sr & errFlags) ? WHAL_EHARDWARE : WHAL_SUCCESS;
        }
        if (WHAL_TIMEOUT_EXPIRED(timeout))
            return WHAL_ETIMEOUT;
    }
}

/* ---- Init / Deinit ---- */

whal_Error whal_Stm32wb_Pka_Init(whal_Crypto *dev)
{
    size_t base = whal_Stm32wb_Pka_Dev.base;
    (void)dev;
    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_EN_Msk, PKA_CR_EN_Msk);
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_Pka_Deinit(whal_Crypto *dev)
{
    size_t base = whal_Stm32wb_Pka_Dev.base;
    (void)dev;
    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_EN_Msk, 0);
    return WHAL_SUCCESS;
}

/* ---- PKA math primitives (whal_Stm32wb_Pka_*, aliased to whal_Pka_* with direct mapping) ----
 *
 * Each function maps to one PKA hardware opcode. Operands are big-endian
 * byte arrays. The PKA peripheral base and the polling timeout are pulled
 * from the whal_Stm32wb_Pka_Dev singleton.
 */

whal_Error whal_Stm32wb_Pka_ModExp(const uint8_t *A, size_t ASz,
                                   const uint8_t *e, size_t eSz,
                                   const uint8_t *N, size_t NSz,
                                   uint8_t *result, size_t resultSz)
{
    size_t base = whal_Stm32wb_Pka_Dev.base;
    const whal_Stm32wb_Pka_Cfg *cfg =
        (const whal_Stm32wb_Pka_Cfg *)whal_Stm32wb_Pka_Dev.cfg;
    whal_Error err;

    if (A == NULL || e == NULL || N == NULL || result == NULL ||
        ASz == 0 || eSz == 0 || NSz == 0 || resultSz == 0)
        return WHAL_EINVAL;
    if (NSz * 8 > 3136 || eSz * 8 > 3136 ||
        ASz != NSz || eSz != NSz || resultSz != NSz)
        return WHAL_ENOTSUP;

    whal_Reg_Write(base, PKA_MODEXP_EXP_LEN, (uint32_t)(eSz * 8));
    whal_Reg_Write(base, PKA_MODEXP_OP_LEN,  (uint32_t)(NSz * 8));
    WriteOperand(base, PKA_MODEXP_A, A, ASz);
    WriteOperand(base, PKA_MODEXP_E, e, eSz);
    WriteOperand(base, PKA_MODEXP_N, N, NSz);

    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_MODE_Msk,
                    (uint32_t)PKA_MODE_MODEXP_NORMAL << PKA_CR_MODE_Pos);
    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_START_Msk, PKA_CR_START_Msk);

    err = WaitForProcEnd(base, cfg->timeout);
    if (err == WHAL_SUCCESS)
        ReadOperand(base, PKA_MODEXP_RESULT, result, resultSz);

    whal_Reg_Write(base, PKA_MODEXP_EXP_LEN, 0);
    whal_Reg_Write(base, PKA_MODEXP_OP_LEN,  0);
    ZeroOperand(base, PKA_MODEXP_A,      ASz);
    ZeroOperand(base, PKA_MODEXP_E,      eSz);
    ZeroOperand(base, PKA_MODEXP_N,      NSz);
    ZeroOperand(base, PKA_MODEXP_RESULT, resultSz);
    return err;
}

whal_Error whal_Stm32wb_Pka_ModInv(const uint8_t *A, size_t ASz,
                                   const uint8_t *N, size_t NSz,
                                   uint8_t *result, size_t resultSz)
{
    size_t base = whal_Stm32wb_Pka_Dev.base;
    const whal_Stm32wb_Pka_Cfg *cfg =
        (const whal_Stm32wb_Pka_Cfg *)whal_Stm32wb_Pka_Dev.cfg;
    whal_Error err;

    if (A == NULL || N == NULL || result == NULL ||
        ASz == 0 || NSz == 0 || resultSz == 0)
        return WHAL_EINVAL;
    if (NSz * 8 > 3136 || ASz != NSz || resultSz != NSz)
        return WHAL_ENOTSUP;

    whal_Reg_Write(base, PKA_MODINV_OP_LEN, (uint32_t)(NSz * 8));
    WriteOperand(base, PKA_MODINV_A, A, ASz);
    WriteOperand(base, PKA_MODINV_N, N, NSz);

    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_MODE_Msk,
                    (uint32_t)PKA_MODE_MODINV << PKA_CR_MODE_Pos);
    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_START_Msk, PKA_CR_START_Msk);

    err = WaitForProcEnd(base, cfg->timeout);
    if (err == WHAL_SUCCESS)
        ReadOperand(base, PKA_MODINV_RESULT, result, resultSz);

    whal_Reg_Write(base, PKA_MODINV_OP_LEN, 0);
    ZeroOperand(base, PKA_MODINV_A,      ASz);
    ZeroOperand(base, PKA_MODINV_N,      NSz);
    ZeroOperand(base, PKA_MODINV_RESULT, resultSz);
    return err;
}

whal_Error whal_Stm32wb_Pka_ModReduce(const uint8_t *A, size_t ASz,
                                      const uint8_t *N, size_t NSz,
                                      uint8_t *result, size_t resultSz)
{
    size_t base = whal_Stm32wb_Pka_Dev.base;
    const whal_Stm32wb_Pka_Cfg *cfg =
        (const whal_Stm32wb_Pka_Cfg *)whal_Stm32wb_Pka_Dev.cfg;
    whal_Error err;

    if (A == NULL || N == NULL || result == NULL ||
        ASz == 0 || NSz == 0 || resultSz == 0)
        return WHAL_EINVAL;
    if (NSz * 8 > 3136 || ASz != NSz || resultSz != NSz)
        return WHAL_ENOTSUP;

    whal_Reg_Write(base, PKA_MOD_REDUCE_OP_LEN,  (uint32_t)(ASz * 8));
    whal_Reg_Write(base, PKA_MOD_REDUCE_MOD_LEN, (uint32_t)(NSz * 8));
    WriteOperand(base, PKA_MOD_REDUCE_A, A, ASz);
    WriteOperand(base, PKA_MOD_REDUCE_N, N, NSz);

    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_MODE_Msk,
                    (uint32_t)PKA_MODE_MOD_REDUCE << PKA_CR_MODE_Pos);
    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_START_Msk, PKA_CR_START_Msk);

    err = WaitForProcEnd(base, cfg->timeout);
    if (err == WHAL_SUCCESS)
        ReadOperand(base, PKA_MOD_REDUCE_RESULT, result, resultSz);

    whal_Reg_Write(base, PKA_MOD_REDUCE_OP_LEN,  0);
    whal_Reg_Write(base, PKA_MOD_REDUCE_MOD_LEN, 0);
    ZeroOperand(base, PKA_MOD_REDUCE_A,      ASz);
    ZeroOperand(base, PKA_MOD_REDUCE_N,      NSz);
    ZeroOperand(base, PKA_MOD_REDUCE_RESULT, resultSz);
    return err;
}

whal_Error whal_Stm32wb_Pka_IntMul(const uint8_t *A, size_t ASz,
                                   const uint8_t *B, size_t BSz,
                                   uint8_t *result, size_t resultSz)
{
    size_t base = whal_Stm32wb_Pka_Dev.base;
    const whal_Stm32wb_Pka_Cfg *cfg =
        (const whal_Stm32wb_Pka_Cfg *)whal_Stm32wb_Pka_Dev.cfg;
    whal_Error err;

    if (A == NULL || B == NULL || result == NULL ||
        ASz == 0 || BSz == 0 || resultSz == 0)
        return WHAL_EINVAL;
    if (ASz * 8 > 3136 || ASz != BSz || resultSz != 2 * ASz)
        return WHAL_ENOTSUP;

    whal_Reg_Write(base, PKA_INT_MUL_OP_LEN, (uint32_t)(ASz * 8));
    WriteOperand(base, PKA_INT_MUL_A, A, ASz);
    WriteOperand(base, PKA_INT_MUL_B, B, BSz);

    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_MODE_Msk,
                    (uint32_t)PKA_MODE_INT_MUL << PKA_CR_MODE_Pos);
    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_START_Msk, PKA_CR_START_Msk);

    err = WaitForProcEnd(base, cfg->timeout);
    if (err == WHAL_SUCCESS)
        ReadOperand(base, PKA_INT_MUL_RESULT, result, resultSz);

    whal_Reg_Write(base, PKA_INT_MUL_OP_LEN, 0);
    ZeroOperand(base, PKA_INT_MUL_A,      ASz);
    ZeroOperand(base, PKA_INT_MUL_B,      BSz);
    ZeroOperand(base, PKA_INT_MUL_RESULT, resultSz);
    return err;
}

whal_Error whal_Stm32wb_Pka_IntSub(const uint8_t *A, size_t ASz,
                                   const uint8_t *B, size_t BSz,
                                   uint8_t *result, size_t resultSz)
{
    size_t base = whal_Stm32wb_Pka_Dev.base;
    const whal_Stm32wb_Pka_Cfg *cfg =
        (const whal_Stm32wb_Pka_Cfg *)whal_Stm32wb_Pka_Dev.cfg;
    whal_Error err;

    if (A == NULL || B == NULL || result == NULL ||
        ASz == 0 || BSz == 0 || resultSz == 0)
        return WHAL_EINVAL;
    if (ASz * 8 > 3136 || ASz != BSz || resultSz != ASz)
        return WHAL_ENOTSUP;

    whal_Reg_Write(base, PKA_INT_SUB_OP_LEN, (uint32_t)(ASz * 8));
    WriteOperand(base, PKA_INT_SUB_A, A, ASz);
    WriteOperand(base, PKA_INT_SUB_B, B, BSz);

    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_MODE_Msk,
                    (uint32_t)PKA_MODE_INT_SUB << PKA_CR_MODE_Pos);
    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_START_Msk, PKA_CR_START_Msk);

    err = WaitForProcEnd(base, cfg->timeout);
    if (err == WHAL_SUCCESS)
        ReadOperand(base, PKA_INT_SUB_RESULT, result, resultSz);

    whal_Reg_Write(base, PKA_INT_SUB_OP_LEN, 0);
    ZeroOperand(base, PKA_INT_SUB_A,      ASz);
    ZeroOperand(base, PKA_INT_SUB_B,      BSz);
    ZeroOperand(base, PKA_INT_SUB_RESULT, resultSz);
    return err;
}

whal_Error whal_Stm32wb_Pka_RsaCrtExp(const uint8_t *A,    size_t ASz,
                                      const uint8_t *dP,   size_t dPSz,
                                      const uint8_t *dQ,   size_t dQSz,
                                      const uint8_t *qInv, size_t qInvSz,
                                      const uint8_t *p,    size_t pSz,
                                      const uint8_t *q,    size_t qSz,
                                      uint8_t *result, size_t resultSz)
{
    size_t base = whal_Stm32wb_Pka_Dev.base;
    const whal_Stm32wb_Pka_Cfg *cfg =
        (const whal_Stm32wb_Pka_Cfg *)whal_Stm32wb_Pka_Dev.cfg;
    whal_Error err;

    if (A == NULL || dP == NULL || dQ == NULL || qInv == NULL ||
        p == NULL || q == NULL || result == NULL ||
        ASz == 0 || dPSz == 0 || dQSz == 0 || qInvSz == 0 ||
        pSz == 0 || qSz == 0 || resultSz == 0)
        return WHAL_EINVAL;
    if ((pSz + qSz) * 8 > 3136 ||
        pSz != qSz || pSz != dPSz || pSz != dQSz || pSz != qInvSz ||
        ASz != pSz + qSz || resultSz != pSz + qSz)
        return WHAL_ENOTSUP;

    whal_Reg_Write(base, PKA_RSA_CRT_OP_LEN,
                   (uint32_t)((pSz + qSz) * 8));
    WriteOperand(base, PKA_RSA_CRT_DP,   dP,   dPSz);
    WriteOperand(base, PKA_RSA_CRT_DQ,   dQ,   dQSz);
    WriteOperand(base, PKA_RSA_CRT_QINV, qInv, qInvSz);
    WriteOperand(base, PKA_RSA_CRT_P,    p,    pSz);
    WriteOperand(base, PKA_RSA_CRT_Q,    q,    qSz);
    WriteOperand(base, PKA_RSA_CRT_A,    A,    ASz);

    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_MODE_Msk,
                    (uint32_t)PKA_MODE_RSA_CRT_EXP << PKA_CR_MODE_Pos);
    whal_Reg_Update(base, PKA_CR_REG, PKA_CR_START_Msk, PKA_CR_START_Msk);

    err = WaitForProcEnd(base, cfg->timeout);
    if (err == WHAL_SUCCESS)
        ReadOperand(base, PKA_RSA_CRT_RESULT, result, resultSz);

    whal_Reg_Write(base, PKA_RSA_CRT_OP_LEN, 0);
    ZeroOperand(base, PKA_RSA_CRT_DP,     dPSz);
    ZeroOperand(base, PKA_RSA_CRT_DQ,     dQSz);
    ZeroOperand(base, PKA_RSA_CRT_QINV,   qInvSz);
    ZeroOperand(base, PKA_RSA_CRT_P,      pSz);
    ZeroOperand(base, PKA_RSA_CRT_Q,      qSz);
    ZeroOperand(base, PKA_RSA_CRT_A,      ASz);
    ZeroOperand(base, PKA_RSA_CRT_RESULT, resultSz);
    return err;
}

/* ---- Vtable ---- */

#ifndef WHAL_CFG_STM32WB_PKA_INIT_DIRECT_API_MAPPING
const whal_CryptoDriver whal_Stm32wb_Pka_CryptoDriver = {
    .Init   = whal_Stm32wb_Pka_Init,
    .Deinit = whal_Stm32wb_Pka_Deinit,
};
#endif
