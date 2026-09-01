# board.mk
#
# Copyright (C) 2026 wolfSSL Inc.
#
# This file is part of wolfHAL.
#
# wolfHAL is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# wolfHAL is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
#

_BOARD_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

PLATFORM = stm32h5
TESTS ?= gpio timer flash rng eth uart

GCC = $(GCC_PATH)arm-none-eabi-gcc
LD = $(GCC_PATH)arm-none-eabi-ld
OBJCOPY = $(GCC_PATH)arm-none-eabi-objcopy

CFLAGS += -Wall -Werror $(INCLUDE) -g3 -Os -ffunction-sections -fdata-sections \
 -ffreestanding -nostdlib -mcpu=cortex-m33 \
 -DPLATFORM_STM32H5 -MMD -MP \
 -DWHAL_CFG_STM32H5_GPIO_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32H5_RCC_PLL_DRIVER \
 -DWHAL_CFG_STM32H5_RCC_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32H5_UART_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32H5_SPI_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32H5_RNG_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32H5_ETH_DIRECT_API_MAPPING \
 -DWHAL_CFG_LAN8742A_ETH_PHY_DIRECT_API_MAPPING \
 -DWHAL_CFG_SYSTICK_TIMER_DIRECT_API_MAPPING
LDFLAGS = --omagic -static --gc-sections

LINKER_SCRIPT ?= $(_BOARD_DIR)/linker.ld

INCLUDE += -I$(_BOARD_DIR) -I$(WHAL_DIR)/boards/peripheral

BOARD_SOURCE = $(_BOARD_DIR)/ivt.c
BOARD_SOURCE += $(_BOARD_DIR)/wolfHAL_board.c
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/flash.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/crypto.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/block.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/sensor.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/stm32h5_*.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/lan8742a_*.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/systick.c)

# Peripheral devices
include $(WHAL_DIR)/boards/peripheral/board.mk

# Flash: STM32H5 isn't supported by mainline openocd. Use STM32_Programmer_CLI:
# STM32_Programmer_CLI -c port=SWD -w <image> -rst
.PHONY: flash
flash:
	@echo "STM32H5 not supported by mainline openocd; use STM32_Programmer_CLI" >&2
	@exit 1
