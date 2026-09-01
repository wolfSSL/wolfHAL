/* stm32wba_iwdg.h
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

#ifndef WHAL_STM32WBA_IWDG_H
#define WHAL_STM32WBA_IWDG_H

/*
 * @file stm32wba_iwdg.h
 * @brief STM32WBA IWDG driver (alias for STM32WB IWDG).
 *
 * The STM32WBA IWDG peripheral is register-compatible with the STM32WB IWDG.
 * This header re-exports the STM32WB IWDG driver types and symbols under
 * STM32WBA-specific names.
 */

#include <wolfHAL/watchdog/stm32wb_iwdg.h>

typedef whal_Stm32wb_Iwdg_Cfg whal_Stm32wba_Iwdg_Cfg;

#define whal_Stm32wba_Iwdg_Dev whal_Stm32wb_Iwdg_Dev

#ifndef WHAL_CFG_STM32WBA_IWDG_DIRECT_API_MAPPING
#define whal_Stm32wba_Iwdg_Driver  whal_Stm32wb_Iwdg_Driver
#define whal_Stm32wba_Iwdg_Init    whal_Stm32wb_Iwdg_Init
#define whal_Stm32wba_Iwdg_Deinit  whal_Stm32wb_Iwdg_Deinit
#define whal_Stm32wba_Iwdg_Refresh whal_Stm32wb_Iwdg_Refresh
#endif /* !WHAL_CFG_STM32WBA_IWDG_DIRECT_API_MAPPING */

/*
 * @brief Prescaler values (re-exported from STM32WB).
 */
#define WHAL_STM32WBA_IWDG_PR_4   WHAL_STM32WB_IWDG_PR_4
#define WHAL_STM32WBA_IWDG_PR_8   WHAL_STM32WB_IWDG_PR_8
#define WHAL_STM32WBA_IWDG_PR_16  WHAL_STM32WB_IWDG_PR_16
#define WHAL_STM32WBA_IWDG_PR_32  WHAL_STM32WB_IWDG_PR_32
#define WHAL_STM32WBA_IWDG_PR_64  WHAL_STM32WB_IWDG_PR_64
#define WHAL_STM32WBA_IWDG_PR_128 WHAL_STM32WB_IWDG_PR_128
#define WHAL_STM32WBA_IWDG_PR_256 WHAL_STM32WB_IWDG_PR_256

/* Config initializer macro alias. The WBA wolfHAL_board.h supplies the body
 * under the WBA-prefixed name; the WB driver source consumes the WB name. */
#define WHAL_CFG_STM32WB_IWDG_DEV WHAL_CFG_STM32WBA_IWDG_DEV

#endif /* WHAL_STM32WBA_IWDG_H */
