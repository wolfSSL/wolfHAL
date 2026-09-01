/* stm32wb_pka.h
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

#ifndef WHAL_STM32WB_PKA_H
#define WHAL_STM32WB_PKA_H

#include <wolfHAL/crypto/crypto.h>
#include <wolfHAL/timeout.h>

/**
 * @file stm32wb_pka.h
 * @brief STM32WB public-key accelerator (PKA) driver.
 *
 * Owns the PKA's whal_Crypto lifecycle (Init/Deinit) and provides the
 * six math primitive implementations. With direct-API mapping enabled,
 * the platform-prefixed math functions become the whal_Pka_* generic
 * names declared in <wolfHAL/crypto/pka.h>.
 */

/**
 * @brief PKA device configuration.
 */
typedef struct {
    whal_Timeout *timeout;
} whal_Stm32wb_Pka_Cfg;

/* ---- Direct API mapping ---- */

#ifdef WHAL_CFG_STM32WB_PKA_INIT_DIRECT_API_MAPPING
#define whal_Stm32wb_Pka_Init   whal_Crypto_Init
#define whal_Stm32wb_Pka_Deinit whal_Crypto_Deinit
#endif

/* The whal_Stm32wb_Pka_* math-primitive → whal_Pka_* alias macros live in
 * src/crypto/stm32wb_pka.c (gated on WHAL_CFG_STM32WB_PKA_DIRECT_API_MAPPING).
 * Callers should use the generic whal_Pka_* names declared in
 * <wolfHAL/crypto/pka.h>. */

/* ---- Init / Deinit ---- */

/**
 * @brief Initialize the STM32WB PKA peripheral.
 *
 * @param dev Crypto device instance.
 *
 * @retval WHAL_SUCCESS PKA peripheral enabled.
 */
whal_Error whal_Stm32wb_Pka_Init(whal_Crypto *dev);

/**
 * @brief Deinitialize the STM32WB PKA peripheral.
 *
 * @param dev Crypto device instance.
 *
 * @retval WHAL_SUCCESS PKA peripheral disabled.
 */
whal_Error whal_Stm32wb_Pka_Deinit(whal_Crypto *dev);

/* ---- Math primitives ---- */

/**
 * @brief Modular exponentiation: result = A^e mod N.
 *
 * @param A         Base, big-endian.
 * @param ASz       Size of A in bytes.
 * @param e         Exponent, big-endian.
 * @param eSz       Size of e in bytes.
 * @param N         Modulus, big-endian (odd).
 * @param NSz       Size of N in bytes.
 * @param result    Output buffer (must be at least NSz bytes).
 * @param resultSz  Output size in bytes.
 *
 * @retval WHAL_SUCCESS    Result written to @p result.
 * @retval WHAL_EINVAL      Null pointer or zero-length operand.
 * @retval WHAL_ENOTSUP    Operand size exceeds the 3136-bit PKA limit, or operand sizes are not all equal (incl. resultSz).
 * @retval WHAL_EHARDWARE  PKA reported ADDRERR or RAMERR.
 * @retval WHAL_ETIMEOUT   Polling timeout expired waiting for PROCENDF.
 */
whal_Error whal_Stm32wb_Pka_ModExp(const uint8_t *A, size_t ASz,
                                   const uint8_t *e, size_t eSz,
                                   const uint8_t *N, size_t NSz,
                                   uint8_t *result, size_t resultSz);

/**
 * @brief Modular inversion: result = A^-1 mod N.
 *
 * @param A         Operand, big-endian.
 * @param ASz       Size of A in bytes.
 * @param N         Modulus, big-endian (odd).
 * @param NSz       Size of N in bytes.
 * @param result    Output buffer.
 * @param resultSz  Output size in bytes.
 *
 * @retval WHAL_SUCCESS    Result written to @p result.
 * @retval WHAL_EINVAL      Null pointer or zero-length operand.
 * @retval WHAL_ENOTSUP    Operand size exceeds the 3136-bit PKA limit, or operand sizes are not all equal (incl. resultSz).
 * @retval WHAL_EHARDWARE  PKA reported ADDRERR or RAMERR.
 * @retval WHAL_ETIMEOUT   Polling timeout expired waiting for PROCENDF.
 */
whal_Error whal_Stm32wb_Pka_ModInv(const uint8_t *A, size_t ASz,
                                   const uint8_t *N, size_t NSz,
                                   uint8_t *result, size_t resultSz);

/**
 * @brief Modular reduction: result = A mod N.
 *
 * @param A         Operand, big-endian.
 * @param ASz       Size of A in bytes.
 * @param N         Modulus, big-endian.
 * @param NSz       Size of N in bytes.
 * @param result    Output buffer.
 * @param resultSz  Output size in bytes.
 *
 * @retval WHAL_SUCCESS    Result written to @p result.
 * @retval WHAL_EINVAL      Null pointer or zero-length operand.
 * @retval WHAL_ENOTSUP    Operand size exceeds the 3136-bit PKA limit, or operand sizes are not all equal (incl. resultSz).
 * @retval WHAL_EHARDWARE  PKA reported ADDRERR or RAMERR.
 * @retval WHAL_ETIMEOUT   Polling timeout expired waiting for PROCENDF.
 */
whal_Error whal_Stm32wb_Pka_ModReduce(const uint8_t *A, size_t ASz,
                                      const uint8_t *N, size_t NSz,
                                      uint8_t *result, size_t resultSz);

/**
 * @brief Big-integer multiplication: result = A * B (non-modular).
 *
 * @param A         Operand A, big-endian.
 * @param ASz       Size of A in bytes.
 * @param B         Operand B, big-endian.
 * @param BSz       Size of B in bytes.
 * @param result    Output buffer (sized for A*B).
 * @param resultSz  Output size in bytes.
 *
 * @retval WHAL_SUCCESS    Result written to @p result.
 * @retval WHAL_EINVAL      Null pointer or zero-length operand.
 * @retval WHAL_ENOTSUP    Operand size exceeds the 3136-bit PKA limit, or
 *                         ASz != BSz, or resultSz != 2 * ASz.
 * @retval WHAL_EHARDWARE  PKA reported ADDRERR or RAMERR.
 * @retval WHAL_ETIMEOUT   Polling timeout expired waiting for PROCENDF.
 */
whal_Error whal_Stm32wb_Pka_IntMul(const uint8_t *A, size_t ASz,
                                   const uint8_t *B, size_t BSz,
                                   uint8_t *result, size_t resultSz);

/**
 * @brief Big-integer subtraction: result = A - B (non-modular).
 *
 * @param A         Operand A, big-endian.
 * @param ASz       Size of A in bytes.
 * @param B         Operand B, big-endian.
 * @param BSz       Size of B in bytes.
 * @param result    Output buffer.
 * @param resultSz  Output size in bytes.
 *
 * @retval WHAL_SUCCESS    Result written to @p result.
 * @retval WHAL_EINVAL      Null pointer or zero-length operand.
 * @retval WHAL_ENOTSUP    Operand size exceeds the 3136-bit PKA limit, or operand sizes are not all equal (incl. resultSz).
 * @retval WHAL_EHARDWARE  PKA reported ADDRERR or RAMERR.
 * @retval WHAL_ETIMEOUT   Polling timeout expired waiting for PROCENDF.
 */
whal_Error whal_Stm32wb_Pka_IntSub(const uint8_t *A, size_t ASz,
                                   const uint8_t *B, size_t BSz,
                                   uint8_t *result, size_t resultSz);

/**
 * @brief RSA CRT exponentiation: result = A^d mod (p*q) via CRT decomposition.
 *
 * @param A         Operand (ciphertext or message), big-endian.
 * @param ASz       Size of A in bytes.
 * @param dP        d mod (p-1), big-endian.
 * @param dPSz      Size of dP in bytes.
 * @param dQ        d mod (q-1), big-endian.
 * @param dQSz      Size of dQ in bytes.
 * @param qInv      q^-1 mod p, big-endian.
 * @param qInvSz    Size of qInv in bytes.
 * @param p         Prime p, big-endian.
 * @param pSz       Size of p in bytes.
 * @param q         Prime q, big-endian.
 * @param qSz       Size of q in bytes.
 * @param result    Output buffer (must be at least pSz + qSz bytes).
 * @param resultSz  Output size in bytes.
 *
 * @retval WHAL_SUCCESS    Result written to @p result.
 * @retval WHAL_EINVAL      Null pointer or zero-length operand.
 * @retval WHAL_ENOTSUP    Combined modulus size exceeds the 3136-bit PKA limit, or CRT operand size layout violated (pSz == qSz == dPSz == dQSz == qInvSz, ASz == resultSz == pSz + qSz).
 * @retval WHAL_EHARDWARE  PKA reported ADDRERR or RAMERR.
 * @retval WHAL_ETIMEOUT   Polling timeout expired waiting for PROCENDF.
 */
whal_Error whal_Stm32wb_Pka_RsaCrtExp(const uint8_t *A,    size_t ASz,
                                      const uint8_t *dP,   size_t dPSz,
                                      const uint8_t *dQ,   size_t dQSz,
                                      const uint8_t *qInv, size_t qInvSz,
                                      const uint8_t *p,    size_t pSz,
                                      const uint8_t *q,    size_t qSz,
                                      uint8_t *result, size_t resultSz);

/* ---- Single-instance device extern ---- */

/*
 * @brief Platform-owned PKA peripheral device. Defined in the driver TU
 * from the WHAL_CFG_STM32WB_PKA_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Crypto whal_Stm32wb_Pka_Dev;

/* ---- Vtable extern ---- */

#ifndef WHAL_CFG_STM32WB_PKA_INIT_DIRECT_API_MAPPING
extern const whal_CryptoDriver whal_Stm32wb_Pka_CryptoDriver;
#endif

#endif /* WHAL_STM32WB_PKA_H */
