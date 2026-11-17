/* wolfHAL_board.h
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

#ifndef WOLFHAL_BOARD_H
#define WOLFHAL_BOARD_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/platform/amd/versal2.h>
#include <wolfHAL/dma/versal2_ospi_dma.h>
#include <wolfHAL/flash/spimem_nor_flash.h>
#include <wolfHAL/flash/spimem_nor_mt35x.h>

/* UART reference clock and console baud rate. The clock is the VEK385
 * device tree's xlnx,clock-freq for the LPD UARTs. */
#define BOARD_UART_CLK_HZ 99999001
#define BOARD_UART_BAUD   115200

/* MT35XU02G OSPI flash: JEDEC ID (2C 5B 1C, datasheet Table 18) and the
 * 1-bit 4-byte address opcodes it is used with (Table 21). */
#define BOARD_FLASH_JEDEC_ID 0x1C5B2CUL
#define BOARD_FLASH_READ4    0x13
#define BOARD_FLASH_PROG4    0x12
#define BOARD_FLASH_ERASE4   0xDC /* 128 KB sector */

/* OSPI: clock /32, CS 0, read capture 1 (VEK385 board probe). */
#define BOARD_OSPI_BAUD_DIV   0xF
#define BOARD_OSPI_CS_LINES   0x0
#define BOARD_OSPI_RD_CAPTURE 0x1

extern whal_Uart g_whalUart;
extern whal_SpiMem g_whalOspi;
extern whal_Flash g_whalFlash;
extern whal_Timeout g_whalTimeout;
extern whal_Dma g_whalOspiDma;

/* No LED driver: the test runner's whal_Gpio_Set calls return
 * WHAL_EINVAL and do nothing. */
#define BOARD_LED_PIN 0

/* The OSPI NOR flash (256 MB). Tests use the last sectors. */
#define BOARD_FLASH_START_ADDR 0x00000000
#define BOARD_FLASH_SIZE       0x10000000
#define BOARD_FLASH_TEST_ADDR  0x0FFC0000
#define BOARD_FLASH_SECTOR_SZ  0x20000

/* BOARD_*_DEV: how this board reaches each peripheral. UART1 is the
 * console (the VEK385 FTDI port, QEMU's second serial). */
#define BOARD_GPIO_DEV  WHAL_INTERNAL_DEV
#define BOARD_UART_DEV  (&g_whalUart)
#define BOARD_FLASH_DEV (&g_whalFlash)

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
void Board_WaitMs(size_t ms);

#endif /* WOLFHAL_BOARD_H */
