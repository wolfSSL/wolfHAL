/* i2c.h
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

#ifndef WHAL_I2C_H
#define WHAL_I2C_H

#include <wolfHAL/error.h>
#include <stdint.h>
#include <stddef.h>

/*
 * @file i2c.h
 * @brief Generic I2C abstraction and driver interface.
 */

typedef struct whal_I2c whal_I2c;

/* I2C message flags */
#define WHAL_I2C_MSG_WRITE   0
#define WHAL_I2C_MSG_READ    (1 << 0)
#define WHAL_I2C_MSG_STOP    (1 << 1)
#define WHAL_I2C_MSG_START   (1 << 2)

/*
 * @brief I2C transfer message descriptor.
 *
 * Each message represents a single segment of an I2C bus transaction.
 * The caller must set the appropriate flags to control START, STOP,
 * and data direction for each segment.
 */
typedef struct {
    void *data;          /* Data buffer */
    size_t dataSz;       /* Number of bytes to transfer */
    uint8_t flags;       /* Combination of WHAL_I2C_MSG_* flags */
} whal_I2c_Msg;

/*
 * @brief I2C communication session parameters.
 *
 * Platform-independent configuration applied when starting a communication
 * session via StartCom and kept in effect until EndCom. The platform driver
 * translates these to hardware register values.
 */
typedef struct {
    uint32_t freq;       /* Bus frequency in Hz */
    uint16_t addr;       /* Target device address */
    uint8_t addrSz;      /* Address size in bits (7 or 10) */
} whal_I2c_ComCfg;

/*
 * @brief Driver vtable for I2C devices.
 */
typedef struct {
    /* Initialize the I2C hardware. */
    whal_Error (*Init)(whal_I2c *i2cDev);
    /* Deinitialize the I2C hardware. */
    whal_Error (*Deinit)(whal_I2c *i2cDev);
    /* Begin a communication session with the given parameters. */
    whal_Error (*StartCom)(whal_I2c *i2cDev, whal_I2c_ComCfg *comCfg);
    /* End the current communication session. */
    whal_Error (*EndCom)(whal_I2c *i2cDev);
    /* Execute a sequence of I2C messages on the bus. */
    whal_Error (*Transfer)(whal_I2c *i2cDev, whal_I2c_Msg *msgs,
                           size_t numMsgs);
} whal_I2cDriver;

/*
 * @brief I2C device instance with a base address, driver vtable, and platform-specific cfg.
 */
struct whal_I2c {
    const uintptr_t base;
    const whal_I2cDriver *driver;
    void *cfg;
};

/*
 * @brief Initializes an I2C device and its driver.
 *
 * @param i2cDev Pointer to the I2C instance to initialize.
 *
 * @retval WHAL_SUCCESS Driver-specific init completed.
 * @retval WHAL_EINVAL  Null pointer or driver rejected configuration.
 */
whal_Error whal_I2c_Init(whal_I2c *i2cDev);
/*
 * @brief Deinitializes an I2C device and releases resources.
 *
 * @param i2cDev Pointer to the I2C instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Driver-specific deinit completed.
 * @retval WHAL_EINVAL  Null pointer or driver refused to deinit.
 */
whal_Error whal_I2c_Deinit(whal_I2c *i2cDev);
/*
 * @brief Begin a communication session with the given parameters.
 *
 * Configures the I2C peripheral for the specified frequency and target
 * address. Must be called before any Transfer calls.
 *
 * @param i2cDev  Pointer to the I2C instance.
 * @param comCfg  Per-session communication parameters.
 *
 * @retval WHAL_SUCCESS Communication session started.
 * @retval WHAL_EINVAL  Null pointer or invalid parameters.
 */
whal_Error whal_I2c_StartCom(whal_I2c *i2cDev, whal_I2c_ComCfg *comCfg);
/*
 * @brief End the current communication session.
 *
 * @param i2cDev Pointer to the I2C instance.
 *
 * @retval WHAL_SUCCESS Communication session ended.
 * @retval WHAL_EINVAL  Null pointer or driver error.
 */
whal_Error whal_I2c_EndCom(whal_I2c *i2cDev);
/*
 * @brief Execute a sequence of I2C messages on the bus.
 *
 * Each message in the array describes one segment of a bus transaction.
 * The caller controls START, STOP, and data direction explicitly via
 * the flags field of each message.
 *
 * @param i2cDev  Pointer to the I2C instance.
 * @param msgs    Array of message descriptors.
 * @param numMsgs Number of messages in the array.
 *
 * @retval WHAL_SUCCESS All messages transferred successfully.
 * @retval WHAL_EINVAL  Null pointer or invalid parameters.
 */
whal_Error whal_I2c_Transfer(whal_I2c *i2cDev, whal_I2c_Msg *msgs,
                             size_t numMsgs);

/*
 * @brief Write to a register on an I2C device.
 *
 * Sends START, addr+W, reg address byte, data bytes, STOP.
 *
 * @param i2cDev Pointer to the I2C instance.
 * @param reg    Register address.
 * @param data   Buffer of bytes to write after the register address.
 * @param dataSz Number of bytes to write.
 *
 * @retval WHAL_SUCCESS Register written.
 * @retval WHAL_EINVAL  Null pointer.
 */
static inline whal_Error whal_I2c_WriteReg(whal_I2c *i2cDev, uint8_t reg,
                                           const void *data, size_t dataSz)
{
    whal_I2c_Msg msgs[2] = {
        { .data = &reg,        .dataSz = 1,
          .flags = WHAL_I2C_MSG_START | WHAL_I2C_MSG_WRITE },
        { .data = (void *)data, .dataSz = dataSz,
          .flags = WHAL_I2C_MSG_WRITE | WHAL_I2C_MSG_STOP },
    };

    return whal_I2c_Transfer(i2cDev, msgs, 2);
}

/*
 * @brief Read one or more bytes from a register on an I2C device.
 *
 * Sends START, addr+W, reg address byte, RESTART, addr+R, data, STOP.
 *
 * @param i2cDev Pointer to the I2C instance.
 * @param reg    Register address.
 * @param data   Buffer to receive data into.
 * @param dataSz Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Register read.
 * @retval WHAL_EINVAL  Null pointer.
 */
static inline whal_Error whal_I2c_ReadReg(whal_I2c *i2cDev, uint8_t reg,
                                          void *data, size_t dataSz)
{
    whal_I2c_Msg msgs[2] = {
        { .data = &reg,  .dataSz = 1,
          .flags = WHAL_I2C_MSG_START | WHAL_I2C_MSG_WRITE },
        { .data = data,  .dataSz = dataSz,
          .flags = WHAL_I2C_MSG_START | WHAL_I2C_MSG_READ | WHAL_I2C_MSG_STOP },
    };

    return whal_I2c_Transfer(i2cDev, msgs, 2);
}

#endif /* WHAL_I2C_H */
