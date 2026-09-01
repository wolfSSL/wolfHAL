/* wolfHAL.h
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

#ifndef WOLFHAL_H
#define WOLFHAL_H

/*
 * @file wolfHAL.h
 * @brief Convenience umbrella header that pulls in all core wolfHAL modules.
 */

#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>

/* Pass WHAL_INTERNAL_DEV as the dev argument when the driver has direct
 * API mapping enabled AND is single-instance. The driver reads its dev
 * struct (e.g. whal_Stm32wb_Iwdg_Dev) from wolfHAL_board.h and ignores whatever
 * gets passed in. */
#define WHAL_INTERNAL_DEV  ((void *)0)

/* Initializer for a dispatcher stub. Use when two drivers share the same
 * generic type on one board (e.g. on-chip flash + SPI NOR). The stub
 * carries only the vtable so the generic whal_<Type>_* dispatch can route
 * to the right driver; the driver itself reads .base and .cfg from its
 * own singleton in wolfHAL_board.h, so they're left unset here. */
#define WHAL_DISPATCH_STUB(driver_ptr)  { .driver = (driver_ptr) }

#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/block/block.h>
#include <wolfHAL/rng/rng.h>
#include <wolfHAL/spi/spi.h>
#include <wolfHAL/i2c/i2c.h>
#include <wolfHAL/timer/timer.h>
#include <wolfHAL/ipc/ipc.h>
#include <wolfHAL/timeout.h>
#include <wolfHAL/crypto/crypto.h>
#include <wolfHAL/dma/dma.h>
#include <wolfHAL/irq/irq.h>
#include <wolfHAL/watchdog/watchdog.h>
#include <wolfHAL/eth/eth.h>
#include <wolfHAL/eth_phy/eth_phy.h>
#include <wolfHAL/sensor/sensor.h>
#include <wolfHAL/pwm/pwm.h>
#include <wolfHAL/display/display.h>

#endif /* WOLFHAL_H */
