/* stm32n6_wwdg.h
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

#ifndef WHAL_STM32N6_WWDG_H
#define WHAL_STM32N6_WWDG_H

/**
 * @file stm32n6_wwdg.h
 * @brief STM32N6 WWDG driver (alias for STM32WB WWDG).
 *
 * The STM32N6 WWDG peripheral is register-compatible with the STM32WB WWDG
 * (CR/CFR/SR at identical offsets 0x000/0x004/0x008). This header re-exports
 * under STM32N6-specific names.
 */

#include <wolfHAL/watchdog/stm32wb_wwdg.h>

typedef whal_Stm32wb_Wwdg_Cfg whal_Stm32n6_Wwdg_Cfg;

#define whal_Stm32n6_Wwdg_Dev whal_Stm32wb_Wwdg_Dev

#ifndef WHAL_CFG_STM32N6_WWDG_DIRECT_API_MAPPING
#define whal_Stm32n6_Wwdg_Driver  whal_Stm32wb_Wwdg_Driver
#define whal_Stm32n6_Wwdg_Init    whal_Stm32wb_Wwdg_Init
#define whal_Stm32n6_Wwdg_Deinit  whal_Stm32wb_Wwdg_Deinit
#define whal_Stm32n6_Wwdg_Refresh whal_Stm32wb_Wwdg_Refresh
#endif /* !WHAL_CFG_STM32N6_WWDG_DIRECT_API_MAPPING */

#define WHAL_STM32N6_WWDG_TB_1   WHAL_STM32WB_WWDG_TB_1
#define WHAL_STM32N6_WWDG_TB_2   WHAL_STM32WB_WWDG_TB_2
#define WHAL_STM32N6_WWDG_TB_4   WHAL_STM32WB_WWDG_TB_4
#define WHAL_STM32N6_WWDG_TB_8   WHAL_STM32WB_WWDG_TB_8
#define WHAL_STM32N6_WWDG_TB_16  WHAL_STM32WB_WWDG_TB_16
#define WHAL_STM32N6_WWDG_TB_32  WHAL_STM32WB_WWDG_TB_32
#define WHAL_STM32N6_WWDG_TB_64  WHAL_STM32WB_WWDG_TB_64
#define WHAL_STM32N6_WWDG_TB_128 WHAL_STM32WB_WWDG_TB_128

/* Config initializer macro alias. The N6 wolfHAL_board.h supplies the body
 * under the N6-prefixed name; the WB driver source consumes the WB name. */
#define WHAL_CFG_STM32WB_WWDG_DEV WHAL_CFG_STM32N6_WWDG_DEV

#endif /* WHAL_STM32N6_WWDG_H */
