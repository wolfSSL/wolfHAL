/* bmi270_sensor.h
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

#ifndef WHAL_IMU_BMI270_H
#define WHAL_IMU_BMI270_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/sensor/sensor.h>
#include <wolfHAL/i2c/i2c.h>

/*
 * @file bmi270.h
 * @brief Bosch BMI270 6-axis IMU driver (accelerometer + gyroscope).
 *
 * The BMI270 communicates over I2C at address 0x68 (SDO low) or
 * 0x69 (SDO high). It requires a binary config blob to be loaded
 * during initialization before producing valid sensor data.
 */

/* Default I2C address (SDO pin low) */
#define WHAL_BMI270_ADDR_LOW   0x68
/* Alternate I2C address (SDO pin high) */
#define WHAL_BMI270_ADDR_HIGH  0x69

/* Expected CHIP_ID value */
#define WHAL_BMI270_CHIP_ID    0x24

/*
 * @brief BMI270 sensor data returned by Read.
 */
typedef struct {
    int16_t accelX;      /* Raw accelerometer X */
    int16_t accelY;      /* Raw accelerometer Y */
    int16_t accelZ;      /* Raw accelerometer Z */
    int16_t gyroX;       /* Raw gyroscope X */
    int16_t gyroY;       /* Raw gyroscope Y */
    int16_t gyroZ;       /* Raw gyroscope Z */
} whal_Bmi270_Data;

/*
 * @brief BMI270 driver configuration.
 */
typedef struct {
    whal_I2c *i2c;                /* I2C bus the sensor is connected to */
    whal_I2c_ComCfg *comCfg;     /* I2C session parameters (addr, freq) */
    const uint8_t *configData;    /* Bosch-provided config blob (8192 bytes) */
    size_t configDataSz;          /* Size of config blob */
    void (*DelayMs)(size_t ms);   /* Millisecond delay function */
} whal_Bmi270_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_BMI270_DEV initializer in wolfHAL_board.h.
 */
#ifdef WHAL_CFG_BMI270_SINGLE_INSTANCE
extern const whal_Sensor whal_Bmi270_Dev;
#endif

#ifndef WHAL_CFG_BMI270_SENSOR_DIRECT_API_MAPPING
/*
 * @brief Driver instance for the BMI270.
 */
extern const whal_SensorDriver whal_Bmi270_Driver;

/*
 * @brief Initialize the BMI270 sensor.
 *
 * Performs soft reset, verifies CHIP_ID, loads the config blob,
 * and enables the accelerometer and gyroscope.
 *
 * @param dev Sensor device instance.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_EHARDWARE CHIP_ID mismatch or config load failed.
 */
whal_Error whal_Bmi270_Init(whal_Sensor *dev);

/*
 * @brief Deinitialize the BMI270 sensor.
 *
 * Issues a soft reset to the device.
 *
 * @param dev Sensor device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Bmi270_Deinit(whal_Sensor *dev);

/*
 * @brief Read accelerometer and gyroscope data from the BMI270.
 *
 * Fetches a new sample from the hardware and fills a
 * whal_Bmi270_Data struct with raw axis values.
 *
 * @param dev  Sensor device instance.
 * @param data Pointer to a whal_Bmi270_Data struct to fill.
 *
 * @retval WHAL_SUCCESS Data read successfully.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Bmi270_Read(whal_Sensor *dev, void *data);
#endif /* !WHAL_CFG_BMI270_SENSOR_DIRECT_API_MAPPING */

#endif /* WHAL_IMU_BMI270_H */
