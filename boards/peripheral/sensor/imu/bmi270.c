/* bmi270.c
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

#include "bmi270.h"
#include <wolfHAL/sensor/imu/bmi270_sensor.h>
#include "wolfHAL_board.h"

/*
 * Bosch BMI270 — 6-axis IMU (accelerometer + gyroscope)
 *
 * - I2C address: 0x68 (SDO low)
 * - Standard mode (100 kHz) or Fast mode (400 kHz)
 * - Requires 8192-byte config blob upload during init
 *
 * The config blob is not bundled with wolfHAL. Obtain it from the Bosch
 * Sensortec BMI270_SensorAPI repository and provide it from your wolfHAL_board.h:
 *
 *     extern const uint8_t whal_bmi270_config_data[];
 *     #define WHAL_BMI270_CONFIG_DATA_SZ 8192
 *
 * with a matching definition compiled into your application.
 */

whal_I2c_ComCfg g_bmi270ComCfg = {
    .freq = 400000, /* 400 kHz fast mode */
    .addr = WHAL_BMI270_ADDR_LOW,
    .addrSz = 7,
};

whal_Sensor g_whalBmi270 = {
    .driver = &whal_Bmi270_Driver,
    .cfg = &(whal_Bmi270_Cfg) {
        .i2c = BOARD_I2C_DEV,
        .comCfg = &g_bmi270ComCfg,
        .configData = whal_bmi270_config_data,
        .configDataSz = WHAL_BMI270_CONFIG_DATA_SZ,
        .DelayMs = Board_WaitMs,
    },
};
