/* stm32wb0_pka.h
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

#ifndef WHAL_STM32WB0_PKA_H
#define WHAL_STM32WB0_PKA_H

/**
 * @file stm32wb0_pka.h
 * @brief STM32WB0 public-key accelerator driver (alias for STM32WB PKA).
 *
 * The STM32WB0 PKA peripheral is register-compatible with the STM32WB
 * PKA:
 *   - PKA_CR/SR/CLRFR at offsets 0x00/0x04/0x08 with matching bit
 *     positions (EN=0, START=1, MODE[13:8], PROCENDIE/F=17,
 *     RAMERRIE/F=19, ADDRERRIE/F=20).
 *   - Identical opcode list and operating modes (RM0529 sections
 *     13.4.1–13.4.18).
 *   - Identical PKA RAM layout (PKA_RAM at +0x400). Modular
 *     exponentiation operand offsets at 0x400/0x404/0x594/0xA44/0xBD0/
 *     0xD5C are verified against RM0529 Table 38.
 * This header re-exports the STM32WB PKA driver symbols under
 * STM32WB0-specific names.
 */

#include <wolfHAL/crypto/stm32wb_pka.h>

typedef whal_Stm32wb_Pka_Cfg whal_Stm32wb0_Pka_Cfg;

#define whal_Stm32wb0_Pka_Dev whal_Stm32wb_Pka_Dev

#ifndef WHAL_CFG_STM32WB0_PKA_INIT_DIRECT_API_MAPPING
#define whal_Stm32wb0_Pka_Init           whal_Stm32wb_Pka_Init
#define whal_Stm32wb0_Pka_Deinit         whal_Stm32wb_Pka_Deinit
#define whal_Stm32wb0_Pka_CryptoDriver   whal_Stm32wb_Pka_CryptoDriver
#endif /* !WHAL_CFG_STM32WB0_PKA_INIT_DIRECT_API_MAPPING */

#ifndef WHAL_CFG_STM32WB0_PKA_DIRECT_API_MAPPING
#define whal_Stm32wb0_Pka_ModExp    whal_Stm32wb_Pka_ModExp
#define whal_Stm32wb0_Pka_ModInv    whal_Stm32wb_Pka_ModInv
#define whal_Stm32wb0_Pka_ModReduce whal_Stm32wb_Pka_ModReduce
#define whal_Stm32wb0_Pka_IntMul    whal_Stm32wb_Pka_IntMul
#define whal_Stm32wb0_Pka_IntSub    whal_Stm32wb_Pka_IntSub
#define whal_Stm32wb0_Pka_RsaCrtExp whal_Stm32wb_Pka_RsaCrtExp
#endif /* !WHAL_CFG_STM32WB0_PKA_DIRECT_API_MAPPING */

/* Config initializer macro alias. The WB0 wolfHAL_board.h supplies the body
 * under the WB0-prefixed name; the WB driver source consumes the WB name. */
#define WHAL_CFG_STM32WB_PKA_DEV WHAL_CFG_STM32WB0_PKA_DEV

#endif /* WHAL_STM32WB0_PKA_H */
