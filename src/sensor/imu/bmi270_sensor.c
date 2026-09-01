/* bmi270_sensor.c
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

#include <stdint.h>
#ifdef WHAL_CFG_BMI270_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides whal_Bmi270_Dev singleton */
#endif
#include <wolfHAL/sensor/imu/bmi270_sensor.h>
#include <wolfHAL/sensor/sensor.h>
#include <wolfHAL/i2c/i2c.h>
#include <wolfHAL/error.h>

/* BMI270 Register Addresses */
#define BMI270_REG_CHIP_ID          0x00
#define BMI270_REG_DATA_ACC_X_LSB   0x0C  /* Accel X/Y/Z: 0x0C-0x11 */
#define BMI270_REG_DATA_GYR_X_LSB   0x12  /* Gyro X/Y/Z: 0x12-0x17 */
#define BMI270_REG_INTERNAL_STATUS  0x21
#define BMI270_REG_ACC_CONF         0x40
#define BMI270_REG_ACC_RANGE        0x41
#define BMI270_REG_GYR_CONF         0x42
#define BMI270_REG_GYR_RANGE        0x43
#define BMI270_REG_INIT_CTRL        0x59
#define BMI270_REG_INIT_DATA        0x5E
#define BMI270_REG_PWR_CONF         0x7C
#define BMI270_REG_PWR_CTRL         0x7D
#define BMI270_REG_CMD              0x7E

/* BMI270 Commands and Values */
#define BMI270_CMD_SOFT_RESET       0xB6
#define BMI270_PWR_CTRL_EN          0x0E  /* Enable accel + gyro + temp */
#define BMI270_INTERNAL_STATUS_OK   0x01
#define BMI270_INTERNAL_STATUS_Msk  0x0F
#define BMI270_PWR_CONF_NORMAL      0x02  /* adv_power_save off, fifo_self_wakeup on */
#define BMI270_ACC_CONF_VAL         0xA8  /* Filter perf, OSR2, ODR 100 Hz */
#define BMI270_ACC_RANGE_16G        0x03  /* +/- 16g */
#define BMI270_GYR_CONF_VAL         0xA9  /* Filter perf, OSR2, ODR 200 Hz */
#define BMI270_GYR_RANGE_2000DPS    0x08  /* +/- 2000 deg/s */

/* Number of raw data bytes: accel (6) + gyro (6) */
#define BMI270_DATA_LEN  12

#ifdef WHAL_CFG_BMI270_SENSOR_DIRECT_API_MAPPING
#define whal_Bmi270_Init   whal_Sensor_Init
#define whal_Bmi270_Deinit whal_Sensor_Deinit
#define whal_Bmi270_Read   whal_Sensor_Read
#endif /* WHAL_CFG_BMI270_SENSOR_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_BMI270_SINGLE_INSTANCE
const whal_Sensor whal_Bmi270_Dev = WHAL_CFG_BMI270_DEV;
#endif

whal_Error whal_Bmi270_Init(whal_Sensor *dev)
{
    whal_Bmi270_Cfg *cfg;
    whal_Error err;
    uint8_t val;

#ifdef WHAL_CFG_BMI270_SINGLE_INSTANCE
    cfg = (whal_Bmi270_Cfg *)whal_Bmi270_Dev.cfg;
    (void)dev;
#else
    if (!dev || !dev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Bmi270_Cfg *)dev->cfg;
#endif

    if (!cfg->i2c || !cfg->comCfg || !cfg->configData ||
        cfg->configDataSz == 0 || !cfg->DelayMs) {
        return WHAL_EINVAL;
    }

    err = whal_I2c_StartCom(cfg->i2c, cfg->comCfg);
    if (err)
        return err;

    /* Soft reset */
    val = BMI270_CMD_SOFT_RESET;
    err = whal_I2c_WriteReg(cfg->i2c, BMI270_REG_CMD, &val, 1);
    if (err)
        goto cleanup;

    cfg->DelayMs(2);

    /* Verify CHIP_ID */
    err = whal_I2c_ReadReg(cfg->i2c, BMI270_REG_CHIP_ID, &val, 1);
    if (err)
        goto cleanup;

    if (val != WHAL_BMI270_CHIP_ID) {
        err = WHAL_EHARDWARE;
        goto cleanup;
    }

    /* Disable advanced power save for config upload */
    val = 0x00;
    err = whal_I2c_WriteReg(cfg->i2c, BMI270_REG_PWR_CONF, &val, 1);
    if (err)
        goto cleanup;

    cfg->DelayMs(1);

    /* Prepare config load */
    val = 0x00;
    err = whal_I2c_WriteReg(cfg->i2c, BMI270_REG_INIT_CTRL, &val, 1);
    if (err)
        goto cleanup;

    /* Burst write config blob */
    err = whal_I2c_WriteReg(cfg->i2c, BMI270_REG_INIT_DATA,
                             cfg->configData, cfg->configDataSz);
    if (err)
        goto cleanup;

    /* Trigger config load */
    val = 0x01;
    err = whal_I2c_WriteReg(cfg->i2c, BMI270_REG_INIT_CTRL, &val, 1);
    if (err)
        goto cleanup;

    cfg->DelayMs(20);
    err = whal_I2c_ReadReg(cfg->i2c, BMI270_REG_INTERNAL_STATUS, &val, 1);
    if (err)
        goto cleanup;

    if ((val & BMI270_INTERNAL_STATUS_Msk) != BMI270_INTERNAL_STATUS_OK) {
        err = WHAL_EHARDWARE;
        goto cleanup;
    }

    /* Enable accelerometer, gyroscope, and temperature sensor */
    val = BMI270_PWR_CTRL_EN;
    err = whal_I2c_WriteReg(cfg->i2c, BMI270_REG_PWR_CTRL, &val, 1);
    if (err)
        goto cleanup;

    /* Configure accelerometer: filter perf, normal BWP, 100 Hz ODR, 16g */
    val = BMI270_ACC_CONF_VAL;
    err = whal_I2c_WriteReg(cfg->i2c, BMI270_REG_ACC_CONF, &val, 1);
    if (err)
        goto cleanup;

    val = BMI270_ACC_RANGE_16G;
    err = whal_I2c_WriteReg(cfg->i2c, BMI270_REG_ACC_RANGE, &val, 1);
    if (err)
        goto cleanup;

    /* Configure gyroscope: filter perf, normal BWP, 200 Hz ODR, 2000 dps */
    val = BMI270_GYR_CONF_VAL;
    err = whal_I2c_WriteReg(cfg->i2c, BMI270_REG_GYR_CONF, &val, 1);
    if (err)
        goto cleanup;

    val = BMI270_GYR_RANGE_2000DPS;
    err = whal_I2c_WriteReg(cfg->i2c, BMI270_REG_GYR_RANGE, &val, 1);
    if (err)
        goto cleanup;

    /* Normal power mode */
    val = BMI270_PWR_CONF_NORMAL;
    err = whal_I2c_WriteReg(cfg->i2c, BMI270_REG_PWR_CONF, &val, 1);
    if (err)
        goto cleanup;

    /* Wait for first sample to be ready */
    cfg->DelayMs(100);

cleanup:
    whal_I2c_EndCom(cfg->i2c);
    return err;
}

whal_Error whal_Bmi270_Deinit(whal_Sensor *dev)
{
    whal_Bmi270_Cfg *cfg;
    whal_Error err;
    uint8_t val = BMI270_CMD_SOFT_RESET;

#ifdef WHAL_CFG_BMI270_SINGLE_INSTANCE
    cfg = (whal_Bmi270_Cfg *)whal_Bmi270_Dev.cfg;
    (void)dev;
#else
    if (!dev || !dev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Bmi270_Cfg *)dev->cfg;
#endif

    err = whal_I2c_StartCom(cfg->i2c, cfg->comCfg);
    if (err)
        return err;

    err = whal_I2c_WriteReg(cfg->i2c, BMI270_REG_CMD, &val, 1);
    whal_I2c_EndCom(cfg->i2c);

    return err;
}

whal_Error whal_Bmi270_Read(whal_Sensor *dev, void *data)
{
    whal_Bmi270_Cfg *cfg;
    whal_Bmi270_Data *out;
    uint8_t raw[BMI270_DATA_LEN];
    whal_Error err;

#ifdef WHAL_CFG_BMI270_SINGLE_INSTANCE
    cfg = (whal_Bmi270_Cfg *)whal_Bmi270_Dev.cfg;
    (void)dev;

    if (!data) {
        return WHAL_EINVAL;
    }
#else
    if (!dev || !dev->cfg || !data) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Bmi270_Cfg *)dev->cfg;
#endif
    out = (whal_Bmi270_Data *)data;

    err = whal_I2c_StartCom(cfg->i2c, cfg->comCfg);
    if (err)
        return err;

    /* Burst read accel + gyro data (0x0C through 0x17) */
    err = whal_I2c_ReadReg(cfg->i2c, BMI270_REG_DATA_ACC_X_LSB,
                            raw, BMI270_DATA_LEN);

    whal_I2c_EndCom(cfg->i2c);

    if (err)
        return err;

    /* Parse little-endian 16-bit values */
    out->accelX = (int16_t)(raw[0]  | (raw[1]  << 8));
    out->accelY = (int16_t)(raw[2]  | (raw[3]  << 8));
    out->accelZ = (int16_t)(raw[4]  | (raw[5]  << 8));
    out->gyroX  = (int16_t)(raw[6]  | (raw[7]  << 8));
    out->gyroY  = (int16_t)(raw[8]  | (raw[9]  << 8));
    out->gyroZ  = (int16_t)(raw[10] | (raw[11] << 8));

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_BMI270_SENSOR_DIRECT_API_MAPPING
const whal_SensorDriver whal_Bmi270_Driver = {
    .Init = whal_Bmi270_Init,
    .Deinit = whal_Bmi270_Deinit,
    .Read = whal_Bmi270_Read,
};
#endif /* !WHAL_CFG_BMI270_SENSOR_DIRECT_API_MAPPING */
