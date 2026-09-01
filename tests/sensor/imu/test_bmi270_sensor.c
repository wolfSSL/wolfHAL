/* test_bmi270_sensor.c
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

#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/sensor/sensor.h>
#include <wolfHAL/sensor/imu/bmi270_sensor.h>
#include "wolfHAL_board.h"
#include "wolfHAL_peripheral.h"
#include "test.h"

/*
 * BMI270 IMU test suite.
 *
 * Requires a BMI270 connected to the board's I2C bus.
 * The board must provide g_whalBmi270 via the peripheral config.
 */

static void Test_Bmi270_Read(void)
{
    whal_Bmi270_Data data = {0};

    WHAL_ASSERT_EQ(whal_Sensor_Read(
                       g_peripheralSensor[PERIPHERAL_SENSOR_BMI270].dev,
                       &data),
                   WHAL_SUCCESS);

    /* At rest, at least one accel axis should be non-zero (gravity) */
    WHAL_ASSERT_NEQ(data.accelX | data.accelY | data.accelZ, 0);
}

static void Test_Bmi270_ReadMultiple(void)
{
    whal_Bmi270_Data data1 = {0};
    whal_Bmi270_Data data2 = {0};

    WHAL_ASSERT_EQ(whal_Sensor_Read(
                       g_peripheralSensor[PERIPHERAL_SENSOR_BMI270].dev,
                       &data1),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Sensor_Read(
                       g_peripheralSensor[PERIPHERAL_SENSOR_BMI270].dev,
                       &data2),
                   WHAL_SUCCESS);

    /* Both reads should have non-zero accel (gravity) */
    WHAL_ASSERT_NEQ(data1.accelX | data1.accelY | data1.accelZ, 0);
    WHAL_ASSERT_NEQ(data2.accelX | data2.accelY | data2.accelZ, 0);
}

void whal_Test_Bmi270_Sensor(void)
{
    WHAL_TEST_SUITE_START("bmi270");
    WHAL_TEST(Test_Bmi270_Read);
    WHAL_TEST(Test_Bmi270_ReadMultiple);
    WHAL_TEST_SUITE_END();
}
