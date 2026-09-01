/* stm32u5_hash.h
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

#ifndef WHAL_STM32U5_HASH_H
#define WHAL_STM32U5_HASH_H

/**
 * @file stm32u5_hash.h
 * @brief STM32U5 HASH driver (alias for STM32WBA HASH).
 *
 * The STM32U5 HASH peripheral is register-compatible with the STM32WBA HASH.
 * This header re-exports the STM32WBA HASH driver types and symbols under
 * STM32U5-specific names.
 */

#include <wolfHAL/crypto/stm32wba_hash.h>

typedef whal_Stm32wba_Hash_Cfg whal_Stm32u5_Hash_Cfg;

typedef whal_Stm32wba_HmacSha1_State   whal_Stm32u5_HmacSha1_State;
typedef whal_Stm32wba_HmacSha224_State whal_Stm32u5_HmacSha224_State;
typedef whal_Stm32wba_HmacSha256_State whal_Stm32u5_HmacSha256_State;

/* Singleton aliases */
#define whal_Stm32u5_Hash_Dev       whal_Stm32wba_Hash_Dev
#define whal_Stm32u5_Sha1_Dev       whal_Stm32wba_Sha1_Dev
#define whal_Stm32u5_Sha224_Dev     whal_Stm32wba_Sha224_Dev
#define whal_Stm32u5_Sha256_Dev     whal_Stm32wba_Sha256_Dev
#define whal_Stm32u5_HmacSha1_Dev   whal_Stm32wba_HmacSha1_Dev
#define whal_Stm32u5_HmacSha224_Dev whal_Stm32wba_HmacSha224_Dev
#define whal_Stm32u5_HmacSha256_Dev whal_Stm32wba_HmacSha256_Dev

/* Vtable aliases */
#define whal_Stm32u5_Hash_CryptoDriver     whal_Stm32wba_Hash_CryptoDriver
#define whal_Stm32u5_Hash_Sha1Driver       whal_Stm32wba_Hash_Sha1Driver
#define whal_Stm32u5_Hash_Sha224Driver     whal_Stm32wba_Hash_Sha224Driver
#define whal_Stm32u5_Hash_Sha256Driver     whal_Stm32wba_Hash_Sha256Driver
#define whal_Stm32u5_Hash_HmacSha1Driver   whal_Stm32wba_Hash_HmacSha1Driver
#define whal_Stm32u5_Hash_HmacSha224Driver whal_Stm32wba_Hash_HmacSha224Driver
#define whal_Stm32u5_Hash_HmacSha256Driver whal_Stm32wba_Hash_HmacSha256Driver

/* Function aliases */
#define whal_Stm32u5_Hash_Init     whal_Stm32wba_Hash_Init
#define whal_Stm32u5_Hash_Deinit   whal_Stm32wba_Hash_Deinit

#define whal_Stm32u5_Sha1_Oneshot   whal_Stm32wba_Sha1_Oneshot
#define whal_Stm32u5_Sha1_Start     whal_Stm32wba_Sha1_Start
#define whal_Stm32u5_Sha1_Process   whal_Stm32wba_Sha1_Process
#define whal_Stm32u5_Sha1_Finalize  whal_Stm32wba_Sha1_Finalize

#define whal_Stm32u5_Sha224_Oneshot  whal_Stm32wba_Sha224_Oneshot
#define whal_Stm32u5_Sha224_Start    whal_Stm32wba_Sha224_Start
#define whal_Stm32u5_Sha224_Process  whal_Stm32wba_Sha224_Process
#define whal_Stm32u5_Sha224_Finalize whal_Stm32wba_Sha224_Finalize

#define whal_Stm32u5_Sha256_Oneshot  whal_Stm32wba_Sha256_Oneshot
#define whal_Stm32u5_Sha256_Start    whal_Stm32wba_Sha256_Start
#define whal_Stm32u5_Sha256_Process  whal_Stm32wba_Sha256_Process
#define whal_Stm32u5_Sha256_Finalize whal_Stm32wba_Sha256_Finalize

#define whal_Stm32u5_HmacSha1_Oneshot  whal_Stm32wba_HmacSha1_Oneshot
#define whal_Stm32u5_HmacSha1_Start    whal_Stm32wba_HmacSha1_Start
#define whal_Stm32u5_HmacSha1_Process  whal_Stm32wba_HmacSha1_Process
#define whal_Stm32u5_HmacSha1_Finalize whal_Stm32wba_HmacSha1_Finalize

#define whal_Stm32u5_HmacSha224_Oneshot  whal_Stm32wba_HmacSha224_Oneshot
#define whal_Stm32u5_HmacSha224_Start    whal_Stm32wba_HmacSha224_Start
#define whal_Stm32u5_HmacSha224_Process  whal_Stm32wba_HmacSha224_Process
#define whal_Stm32u5_HmacSha224_Finalize whal_Stm32wba_HmacSha224_Finalize

#define whal_Stm32u5_HmacSha256_Oneshot  whal_Stm32wba_HmacSha256_Oneshot
#define whal_Stm32u5_HmacSha256_Start    whal_Stm32wba_HmacSha256_Start
#define whal_Stm32u5_HmacSha256_Process  whal_Stm32wba_HmacSha256_Process
#define whal_Stm32u5_HmacSha256_Finalize whal_Stm32wba_HmacSha256_Finalize

/* Config initializer macro aliases. The U5 wolfHAL_board.h supplies the bodies
 * under U5-prefixed names; the WBA driver source consumes the WBA names. */
#define WHAL_CFG_STM32WBA_HASH_DEV             WHAL_CFG_STM32U5_HASH_DEV
#define WHAL_CFG_STM32WBA_HASH_SHA1_DEV        WHAL_CFG_STM32U5_HASH_SHA1_DEV
#define WHAL_CFG_STM32WBA_HASH_SHA224_DEV      WHAL_CFG_STM32U5_HASH_SHA224_DEV
#define WHAL_CFG_STM32WBA_HASH_SHA256_DEV      WHAL_CFG_STM32U5_HASH_SHA256_DEV
#define WHAL_CFG_STM32WBA_HASH_HMAC_SHA1_DEV   WHAL_CFG_STM32U5_HASH_HMAC_SHA1_DEV
#define WHAL_CFG_STM32WBA_HASH_HMAC_SHA224_DEV WHAL_CFG_STM32U5_HASH_HMAC_SHA224_DEV
#define WHAL_CFG_STM32WBA_HASH_HMAC_SHA256_DEV WHAL_CFG_STM32U5_HASH_HMAC_SHA256_DEV

#endif /* WHAL_STM32U5_HASH_H */
