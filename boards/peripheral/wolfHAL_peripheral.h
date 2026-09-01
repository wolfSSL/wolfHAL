/* wolfHAL_peripheral.h
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

#ifndef WOLFHAL_PERIPHERAL_H
#define WOLFHAL_PERIPHERAL_H

#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/block/block.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/sensor/sensor.h>
#include <stddef.h>
#include <stdint.h>

/* Peripheral block device test configuration */
typedef struct {
    const char *name;
    whal_Block *dev;
    uint8_t *blockBuf;   /* Test buffer (must hold at least 2 blocks) */
    size_t blockBufSz;   /* Size of blockBuf in bytes */
    size_t blockSz;      /* Block size in bytes */
    uint8_t erasedByte;  /* Expected byte value after erase (0x00 or 0xFF) */
} whal_PeripheralBlock_Cfg;

/* Peripheral flash device test configuration */
typedef struct {
    const char *name;
    whal_Flash *dev;
    size_t sectorSz;     /* Sector (erase) size in bytes */
} whal_PeripheralFlash_Cfg;

/* Peripheral sensor device test configuration */
typedef struct {
    const char *name;
    whal_Sensor *dev;
} whal_PeripheralSensor_Cfg;

extern whal_PeripheralBlock_Cfg g_peripheralBlock[];
extern whal_PeripheralFlash_Cfg g_peripheralFlash[];

enum {
#ifdef PERIPHERAL_BMI270
    PERIPHERAL_SENSOR_BMI270,
#endif
    PERIPHERAL_SENSOR_COUNT,
};

extern whal_PeripheralSensor_Cfg g_peripheralSensor[];

whal_Error Peripheral_Init(void);
whal_Error Peripheral_Deinit(void);

#endif /* WOLFHAL_PERIPHERAL_H */
