/* esp32c6_gpio.h
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

#ifndef WHAL_ESP32C6_GPIO_H
#define WHAL_ESP32C6_GPIO_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/bitops.h>

/*
 * @file esp32c6_gpio.h
 * @brief ESP32-C6 GPIO driver configuration types.
 */

/* Register field macros for composing the config words (prefix-less; register
 * offsets live in the driver .c). Bit positions per TRM chapter 7. */

/* IO_MUX_GPIOn_REG fields (PinCfg.ioMuxCfg). */
#define IO_MUX_FUN_WPD_Pos     7
#define IO_MUX_FUN_WPD_Msk     (1UL << IO_MUX_FUN_WPD_Pos)
#define IO_MUX_FUN_WPU_Pos     8
#define IO_MUX_FUN_WPU_Msk     (1UL << IO_MUX_FUN_WPU_Pos)
#define IO_MUX_FUN_IE_Pos      9
#define IO_MUX_FUN_IE_Msk      (1UL << IO_MUX_FUN_IE_Pos)
#define IO_MUX_FUN_DRV_Pos     10
#define IO_MUX_FUN_DRV_Msk     (WHAL_BITMASK(2) << IO_MUX_FUN_DRV_Pos)
#define IO_MUX_MCU_SEL_Pos     12
#define IO_MUX_MCU_SEL_Msk     (WHAL_BITMASK(3) << IO_MUX_MCU_SEL_Pos)
#define IO_MUX_FILTER_EN_Pos   15
#define IO_MUX_FILTER_EN_Msk   (1UL << IO_MUX_FILTER_EN_Pos)

/* GPIO_PINn_REG fields (PinCfg.gpioCfg). */
#define GPIO_PIN_SYNC2_BYPASS_Pos   0
#define GPIO_PIN_SYNC2_BYPASS_Msk   (WHAL_BITMASK(2) << GPIO_PIN_SYNC2_BYPASS_Pos)
#define GPIO_PIN_PAD_DRIVER_Pos     2
#define GPIO_PIN_PAD_DRIVER_Msk     (1UL << GPIO_PIN_PAD_DRIVER_Pos)
#define GPIO_PIN_SYNC1_BYPASS_Pos   3
#define GPIO_PIN_SYNC1_BYPASS_Msk   (WHAL_BITMASK(2) << GPIO_PIN_SYNC1_BYPASS_Pos)
#define GPIO_PIN_INT_TYPE_Pos       7
#define GPIO_PIN_INT_TYPE_Msk       (WHAL_BITMASK(3) << GPIO_PIN_INT_TYPE_Pos)
#define GPIO_PIN_WAKEUP_ENABLE_Pos  10
#define GPIO_PIN_WAKEUP_ENABLE_Msk  (1UL << GPIO_PIN_WAKEUP_ENABLE_Pos)
#define GPIO_PIN_INT_ENA_Pos        13
#define GPIO_PIN_INT_ENA_Msk        (WHAL_BITMASK(5) << GPIO_PIN_INT_ENA_Pos)

/* GPIO_FUNCn_IN_SEL_CFG_REG fields (Cfg.inputCfg values). */
#define GPIO_FUNC_IN_SEL_Pos       0
#define GPIO_FUNC_IN_SEL_Msk       (WHAL_BITMASK(6) << GPIO_FUNC_IN_SEL_Pos)
#define GPIO_FUNC_IN_INV_SEL_Pos   6
#define GPIO_FUNC_IN_INV_SEL_Msk   (1UL << GPIO_FUNC_IN_INV_SEL_Pos)
#define GPIO_SIG_IN_SEL_Pos        7
#define GPIO_SIG_IN_SEL_Msk        (1UL << GPIO_SIG_IN_SEL_Pos)

/* GPIO_FUNCn_OUT_SEL_CFG_REG fields (Cfg.outputCfg values). */
#define GPIO_FUNC_OUT_SEL_Pos      0
#define GPIO_FUNC_OUT_SEL_Msk      (WHAL_BITMASK(9) << GPIO_FUNC_OUT_SEL_Pos)
#define GPIO_FUNC_OUT_INV_SEL_Pos  9
#define GPIO_FUNC_OUT_INV_SEL_Msk  (1UL << GPIO_FUNC_OUT_INV_SEL_Pos)
#define GPIO_FUNC_OEN_SEL_Pos      10
#define GPIO_FUNC_OEN_SEL_Msk      (1UL << GPIO_FUNC_OEN_SEL_Pos)
#define GPIO_FUNC_OEN_INV_SEL_Pos  11
#define GPIO_FUNC_OEN_INV_SEL_Msk  (1UL << GPIO_FUNC_OEN_INV_SEL_Pos)

/* MCU_SEL value selecting the GPIO (matrix) function; OUT_SEL value driving a
 * pin from GPIO_OUT_REG. */
#define IO_MUX_FUNC_GPIO           1
#define GPIO_SIG_GPIO              128

/* GPIO_PINn INT_TYPE trigger types. */
#define GPIO_INT_DISABLE           0
#define GPIO_INT_RISING            1
#define GPIO_INT_FALLING           2
#define GPIO_INT_ANY_EDGE          3
#define GPIO_INT_LOW_LEVEL         4
#define GPIO_INT_HIGH_LEVEL        5

/*
 * @brief Compose an IO_MUX_GPIOn_REG value (PinCfg.ioMuxCfg).
 * @param func     IO MUX function (IO_MUX_FUNC_GPIO for matrix routing).
 * @param inputEn  1 to enable the input buffer (FUN_IE).
 * @param pull     0, IO_MUX_FUN_WPU_Msk, or IO_MUX_FUN_WPD_Msk.
 * @param drive    Drive strength 0-3 (reset default 2).
 */
#define WHAL_ESP32C6_GPIO_IOMUX(func, inputEn, pull, drive) ( \
    whal_SetBits(IO_MUX_MCU_SEL_Msk, IO_MUX_MCU_SEL_Pos, (func)) | \
    ((inputEn) ? IO_MUX_FUN_IE_Msk : 0u) | \
    (pull) | \
    whal_SetBits(IO_MUX_FUN_DRV_Msk, IO_MUX_FUN_DRV_Pos, (drive)))

/*
 * @brief Compose a GPIO_PINn_REG value (PinCfg.gpioCfg).
 * @param openDrain 1 for open-drain output (PAD_DRIVER).
 * @param intType   GPIO_INT_* trigger type.
 * @param intEna    5-bit CPU-target interrupt-enable mask.
 * @param wakeup    1 to enable GPIO wake-up.
 */
#define WHAL_ESP32C6_GPIO_PINREG(openDrain, intType, intEna, wakeup) ( \
    ((openDrain) ? GPIO_PIN_PAD_DRIVER_Msk : 0u) | \
    whal_SetBits(GPIO_PIN_INT_TYPE_Msk, GPIO_PIN_INT_TYPE_Pos, (intType)) | \
    whal_SetBits(GPIO_PIN_INT_ENA_Msk, GPIO_PIN_INT_ENA_Pos, (intEna)) | \
    ((wakeup) ? GPIO_PIN_WAKEUP_ENABLE_Msk : 0u))

/*
 * @brief Compose a GPIO_FUNCn_IN_SEL_CFG_REG value (Cfg.inputCfg[signal]):
 * route a peripheral input signal from @p srcPin via the GPIO matrix. OR
 * GPIO_FUNC_IN_INV_SEL_Msk to invert.
 */
#define WHAL_ESP32C6_GPIO_IN(srcPin) ( \
    whal_SetBits(GPIO_FUNC_IN_SEL_Msk, GPIO_FUNC_IN_SEL_Pos, (srcPin)) | \
    GPIO_SIG_IN_SEL_Msk)

/*
 * @brief Compose a GPIO_FUNCn_OUT_SEL_CFG_REG value (Cfg.outputCfg[pin]): drive
 * @p signal onto the pin. Output enable is peripheral-controlled; OR
 * GPIO_FUNC_OEN_SEL_Msk to drive it from GPIO_ENABLE instead.
 */
#define WHAL_ESP32C6_GPIO_OUT(signal) \
    whal_SetBits(GPIO_FUNC_OUT_SEL_Msk, GPIO_FUNC_OUT_SEL_Pos, (signal))

/*
 * @brief Per-pin GPIO configuration.
 */
typedef struct {
    uint8_t pin;
    uint8_t outputEnable;
    uint32_t gpioCfg;
    uint32_t ioMuxCfg;
} whal_Esp32c6_Gpio_PinCfg;

/*
 * @brief Per-signal input routing (value for GPIO_FUNCn_IN_SEL_CFG_REG).
 */
typedef struct {
    uint8_t signalNum;
    uint32_t cfg;
} whal_Esp32c6_Gpio_InputCfg;

/*
 * @brief Per-pin output routing (value for GPIO_FUNCn_OUT_SEL_CFG_REG).
 */
typedef struct {
    uint8_t gpioNum;
    uint32_t cfg;
} whal_Esp32c6_Gpio_OutputCfg;

/*
 * @brief GPIO device configuration: an array of per-pin configurations.
 */
typedef struct {
    const whal_Esp32c6_Gpio_PinCfg *pinCfg; /* Array of pin configurations */
    size_t pinCount;                        /* Number of pins to configure */

    const whal_Esp32c6_Gpio_InputCfg *inputCfg;
    size_t inputCount;

    const whal_Esp32c6_Gpio_OutputCfg *outputCfg;
    size_t outputCount;
} whal_Esp32c6_Gpio_Cfg;

/*
 * @brief Platform-owned GPIO device singleton. Defined in the driver TU
 * from the WHAL_CFG_ESP32C6_GPIO_DEV initializer in board.h.
 */
extern const whal_Gpio whal_Esp32c6_Gpio_Dev;

#ifndef WHAL_CFG_ESP32C6_GPIO_DIRECT_API_MAPPING
/*
 * @brief Driver instance for ESP32-C6 GPIO.
 */
extern const whal_GpioDriver whal_Esp32c6_Gpio_Driver;

/*
 * @brief Initialize the ESP32-C6 GPIO peripheral and configured pins.
 *
 * @param gpioDev GPIO device instance.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Not yet implemented.
 */
whal_Error whal_Esp32c6_Gpio_Init(whal_Gpio *gpioDev);
/*
 * @brief Deinitialize the ESP32-C6 GPIO peripheral.
 *
 * @param gpioDev GPIO device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Not yet implemented.
 */
whal_Error whal_Esp32c6_Gpio_Deinit(whal_Gpio *gpioDev);
/*
 * @brief Read a GPIO pin value.
 *
 * @param gpioDev GPIO device instance.
 * @param pin     Pin index in the configured pin table.
 * @param value   Output for the sampled pin value.
 *
 * @retval WHAL_SUCCESS Pin value read.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Not yet implemented.
 */
whal_Error whal_Esp32c6_Gpio_Get(whal_Gpio *gpioDev, size_t pin, size_t *value);
/*
 * @brief Set a GPIO pin value.
 *
 * @param gpioDev GPIO device instance.
 * @param pin     Pin index in the configured pin table.
 * @param value   Value to drive.
 *
 * @retval WHAL_SUCCESS Pin updated.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Not yet implemented.
 */
whal_Error whal_Esp32c6_Gpio_Set(whal_Gpio *gpioDev, size_t pin, size_t value);
#endif /* !WHAL_CFG_ESP32C6_GPIO_DIRECT_API_MAPPING */

#endif /* WHAL_ESP32C6_GPIO_H */
