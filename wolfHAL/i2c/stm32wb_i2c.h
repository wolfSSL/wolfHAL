/* stm32wb_i2c.h
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

#ifndef WHAL_STM32WB_I2C_H
#define WHAL_STM32WB_I2C_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/i2c/i2c.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32wb_i2c.h
 * @brief STM32WB I2C driver configuration.
 *
 * The STM32WB I2C peripheral provides:
 * - Controller and target modes (this driver supports controller only)
 * - Standard-mode (up to 100 kHz), Fast-mode (up to 400 kHz),
 *   Fast-mode Plus (up to 1 MHz)
 * - 7-bit and 10-bit addressing
 * - Programmable setup and hold timings via I2C_TIMINGR
 * - Hardware byte counter with automatic STOP/RESTART generation
 */

/*
 * @brief I2C device configuration.
 */
typedef struct whal_Stm32wb_I2c_Cfg {
    uint32_t pclk;        /* I2C kernel clock frequency in Hz */
    whal_Timeout *timeout;
} whal_Stm32wb_I2c_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_STM32WB_I2C_DEV initializer in wolfHAL_board.h.
 */
#if defined(WHAL_CFG_STM32WB_I2C_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32F0_I2C_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32F3_I2C_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32N6_I2C_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32WB0_I2C_SINGLE_INSTANCE)
extern const whal_I2c whal_Stm32wb_I2c_Dev;
#endif

#ifndef WHAL_CFG_STM32WB_I2C_DIRECT_API_MAPPING
/*
 * @brief Driver instance for STM32WB I2C peripheral.
 */
extern const whal_I2cDriver whal_Stm32wb_I2c_Driver;

/*
 * @brief Initialize the STM32WB I2C peripheral.
 *
 * Configures noise filters and enables the peripheral.
 *
 * @param i2cDev I2C device instance to initialize.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_I2c_Init(whal_I2c *i2cDev);

/*
 * @brief Deinitialize the STM32WB I2C peripheral.
 *
 * Disables the peripheral.
 *
 * @param i2cDev I2C device instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_I2c_Deinit(whal_I2c *i2cDev);

/*
 * @brief Begin a communication session on the STM32WB I2C peripheral.
 *
 * Computes I2C_TIMINGR from the configured pclk and the requested
 * frequency, then writes the target address into CR2.
 *
 * @param i2cDev  I2C device instance.
 * @param comCfg  Per-session communication parameters.
 *
 * @retval WHAL_SUCCESS Communication session started.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_I2c_StartCom(whal_I2c *i2cDev, whal_I2c_ComCfg *comCfg);

/*
 * @brief End the current communication session on the STM32WB I2C peripheral.
 *
 * Clears the target address from CR2.
 *
 * @param i2cDev  I2C device instance.
 *
 * @retval WHAL_SUCCESS Communication session ended.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_I2c_EndCom(whal_I2c *i2cDev);

/*
 * @brief Execute a sequence of I2C messages on the bus.
 *
 * @param i2cDev  I2C device instance.
 * @param msgs    Array of message descriptors.
 * @param numMsgs Number of messages in the array.
 *
 * @retval WHAL_SUCCESS All messages transferred successfully.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ETIMEOUT Hardware did not respond in time.
 * @retval WHAL_EHARDWARE NACK or bus error detected.
 */
whal_Error whal_Stm32wb_I2c_Transfer(whal_I2c *i2cDev, whal_I2c_Msg *msgs,
                                     size_t numMsgs);
#endif /* !WHAL_CFG_STM32WB_I2C_DIRECT_API_MAPPING */

#endif /* WHAL_STM32WB_I2C_H */
