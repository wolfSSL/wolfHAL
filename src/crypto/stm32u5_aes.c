/* stm32u5_aes.c
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

#ifdef WHAL_CFG_STM32U5_AES_INIT_DIRECT_API_MAPPING
#define WHAL_CFG_STM32WBA_AES_INIT_DIRECT_API_MAPPING
#endif
#ifdef WHAL_CFG_STM32U5_AES_ECB_DIRECT_API_MAPPING
#define WHAL_CFG_STM32WBA_AES_ECB_DIRECT_API_MAPPING
#endif
#ifdef WHAL_CFG_STM32U5_AES_CBC_DIRECT_API_MAPPING
#define WHAL_CFG_STM32WBA_AES_CBC_DIRECT_API_MAPPING
#endif
#ifdef WHAL_CFG_STM32U5_AES_CTR_DIRECT_API_MAPPING
#define WHAL_CFG_STM32WBA_AES_CTR_DIRECT_API_MAPPING
#endif
#ifdef WHAL_CFG_STM32U5_AES_GCM_DIRECT_API_MAPPING
#define WHAL_CFG_STM32WBA_AES_GCM_DIRECT_API_MAPPING
#endif
#ifdef WHAL_CFG_STM32U5_AES_GMAC_DIRECT_API_MAPPING
#define WHAL_CFG_STM32WBA_AES_GMAC_DIRECT_API_MAPPING
#endif
#ifdef WHAL_CFG_STM32U5_AES_CCM_DIRECT_API_MAPPING
#define WHAL_CFG_STM32WBA_AES_CCM_DIRECT_API_MAPPING
#endif
#include "stm32wba_aes.c"
