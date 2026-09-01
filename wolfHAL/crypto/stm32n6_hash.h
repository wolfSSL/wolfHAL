/* stm32n6_hash.h
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

#ifndef WHAL_STM32N6_HASH_H
#define WHAL_STM32N6_HASH_H

/**
 * @file stm32n6_hash.h
 * @brief STM32N6 HASH driver (alias for STM32WBA HASH).
 *
 * The STM32N6 HASH peripheral is register-compatible with the STM32WBA HASH
 * (CR/DIN/STR/HRAx/IMR/SR/CSRx at identical offsets). This header re-exports
 * under STM32N6-specific names.
 */

#include <wolfHAL/crypto/stm32wba_hash.h>

typedef whal_Stm32wba_Hash_Cfg whal_Stm32n6_Hash_Cfg;

typedef whal_Stm32wba_HmacSha1_State   whal_Stm32n6_HmacSha1_State;
typedef whal_Stm32wba_HmacSha224_State  whal_Stm32n6_HmacSha224_State;
typedef whal_Stm32wba_HmacSha256_State  whal_Stm32n6_HmacSha256_State;

#define whal_Stm32n6_Hash_Dev       whal_Stm32wba_Hash_Dev
#define whal_Stm32n6_Sha1_Dev       whal_Stm32wba_Sha1_Dev
#define whal_Stm32n6_Sha224_Dev     whal_Stm32wba_Sha224_Dev
#define whal_Stm32n6_Sha256_Dev     whal_Stm32wba_Sha256_Dev
#define whal_Stm32n6_HmacSha1_Dev   whal_Stm32wba_HmacSha1_Dev
#define whal_Stm32n6_HmacSha224_Dev whal_Stm32wba_HmacSha224_Dev
#define whal_Stm32n6_HmacSha256_Dev whal_Stm32wba_HmacSha256_Dev

#define whal_Stm32n6_Hash_CryptoDriver    whal_Stm32wba_Hash_CryptoDriver

#define whal_Stm32n6_Hash_Sha1Driver      whal_Stm32wba_Hash_Sha1Driver
#define whal_Stm32n6_Hash_Sha224Driver    whal_Stm32wba_Hash_Sha224Driver
#define whal_Stm32n6_Hash_Sha256Driver    whal_Stm32wba_Hash_Sha256Driver
#define whal_Stm32n6_Hash_HmacSha1Driver  whal_Stm32wba_Hash_HmacSha1Driver
#define whal_Stm32n6_Hash_HmacSha224Driver whal_Stm32wba_Hash_HmacSha224Driver
#define whal_Stm32n6_Hash_HmacSha256Driver whal_Stm32wba_Hash_HmacSha256Driver

/* Config initializer macro aliases. The N6 wolfHAL_board.h supplies the bodies
 * under N6-prefixed names; the WBA driver source consumes the WBA names. */
#define WHAL_CFG_STM32WBA_HASH_DEV             WHAL_CFG_STM32N6_HASH_DEV
#define WHAL_CFG_STM32WBA_HASH_SHA1_DEV        WHAL_CFG_STM32N6_HASH_SHA1_DEV
#define WHAL_CFG_STM32WBA_HASH_SHA224_DEV      WHAL_CFG_STM32N6_HASH_SHA224_DEV
#define WHAL_CFG_STM32WBA_HASH_SHA256_DEV      WHAL_CFG_STM32N6_HASH_SHA256_DEV
#define WHAL_CFG_STM32WBA_HASH_HMAC_SHA1_DEV   WHAL_CFG_STM32N6_HASH_HMAC_SHA1_DEV
#define WHAL_CFG_STM32WBA_HASH_HMAC_SHA224_DEV WHAL_CFG_STM32N6_HASH_HMAC_SHA224_DEV
#define WHAL_CFG_STM32WBA_HASH_HMAC_SHA256_DEV WHAL_CFG_STM32N6_HASH_HMAC_SHA256_DEV

#endif /* WHAL_STM32N6_HASH_H */
