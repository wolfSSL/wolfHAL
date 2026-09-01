/* wolfHAL_peripheral.c
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

#include "wolfHAL_peripheral.h"

#ifdef PERIPHERAL_SDHC_SPI_SDCARD32GB
#include "block/sdhc_spi_sdcard32gb.h"
#endif

#ifdef PERIPHERAL_SPI_NOR_W25Q64
#include "flash/spi_nor_w25q64.h"
#endif

#ifdef PERIPHERAL_BMI270
#include "sensor/imu/bmi270.h"
#endif

whal_PeripheralBlock_Cfg g_peripheralBlock[] = {
#ifdef PERIPHERAL_SDHC_SPI_SDCARD32GB
    {
        .name = "sdhc_spi_sdcard32gb",
        .dev = &g_whalSdhcSpiSdcard32gb,
        .blockSz = WHAL_SDHC_SPI_BLOCK_SZ,
        .blockBuf = (uint8_t[WHAL_SDHC_SPI_BLOCK_SZ * 2]) {0},
        .blockBufSz = WHAL_SDHC_SPI_BLOCK_SZ * 2,
        .erasedByte = 0x00,
    },
#endif
    {0}, /* sentinel */
};

whal_PeripheralFlash_Cfg g_peripheralFlash[] = {
#ifdef PERIPHERAL_SPI_NOR_W25Q64
    {
        .name = "spi_nor_w25q64",
        .dev = &g_whalSpiNorW25q64,
        .sectorSz = 4096,
    },
#endif
    {0}, /* sentinel */
};

whal_PeripheralSensor_Cfg g_peripheralSensor[] = {
#ifdef PERIPHERAL_BMI270
    {
        .name = "bmi270",
        .dev = &g_whalBmi270,
    },
#endif
    {0}, /* sentinel */
};

whal_Error Peripheral_Init(void)
{
    whal_Error err;

    for (size_t i = 0; g_peripheralBlock[i].dev; i++) {
        err = whal_Block_Init(g_peripheralBlock[i].dev);
        if (err)
            return err;
    }

    for (size_t i = 0; g_peripheralFlash[i].dev; i++) {
        err = whal_Flash_Init(g_peripheralFlash[i].dev);
        if (err)
            return err;
    }

    for (size_t i = 0; g_peripheralSensor[i].dev; i++) {
        err = whal_Sensor_Init(g_peripheralSensor[i].dev);
        if (err)
            return err;
    }

    return WHAL_SUCCESS;
}

whal_Error Peripheral_Deinit(void)
{
    whal_Error err;

    for (size_t i = 0; g_peripheralSensor[i].dev; i++) {
        err = whal_Sensor_Deinit(g_peripheralSensor[i].dev);
        if (err)
            return err;
    }

    for (size_t i = 0; g_peripheralFlash[i].dev; i++) {
        err = whal_Flash_Deinit(g_peripheralFlash[i].dev);
        if (err)
            return err;
    }

    for (size_t i = 0; g_peripheralBlock[i].dev; i++) {
        err = whal_Block_Deinit(g_peripheralBlock[i].dev);
        if (err)
            return err;
    }

    return WHAL_SUCCESS;
}
