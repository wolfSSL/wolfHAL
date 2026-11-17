/* spimem_nor_mt35x.h
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

#ifndef WHAL_SPIMEM_NOR_MT35X_H
#define WHAL_SPIMEM_NOR_MT35X_H

/*
 * @file spimem_nor_mt35x.h
 * @brief whal_SpiMemNor_Flash_Cfg values for Micron MT35X octal NOR flash
 * (MT35XU02G datasheet).
 */

/* Flag status register (70h): erase (5), program (4) and protection (1)
 * errors, cleared with 50h (Table 5) */
#define WHAL_SPIMEM_NOR_MT35X_ERR_OP       0x70
#define WHAL_SPIMEM_NOR_MT35X_ERR_MASK     0x32
#define WHAL_SPIMEM_NOR_MT35X_ERR_CLEAR_OP 0x50

/* After a reset: volatile configuration (81h) I/O mode at 00h to FFh,
 * 1-bit (Table 7). 3 address bytes: the nonvolatile default after reset. */
#define WHAL_SPIMEM_NOR_MT35X_RST_REG_OP         0x81
#define WHAL_SPIMEM_NOR_MT35X_RST_REG_ADDR_BYTES 3
#define WHAL_SPIMEM_NOR_MT35X_RST_REG_ADDR       0x00
#define WHAL_SPIMEM_NOR_MT35X_RST_REG_VAL        0xFF

#endif /* WHAL_SPIMEM_NOR_MT35X_H */
