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

PLATFORM = stm32n6
TESTS ?= gpio timer rng eth uart spi i2c aes_ecb aes_cbc aes_ctr aes_gcm aes_gmac aes_ccm sha1 sha224 sha256 hmac_sha1 hmac_sha224 hmac_sha256

GCC = $(GCC_PATH)arm-none-eabi-gcc
LD = $(GCC_PATH)arm-none-eabi-ld
OBJCOPY = $(GCC_PATH)arm-none-eabi-objcopy

CFLAGS += -Wall -Werror $(INCLUDE) -g3 -Os -ffunction-sections -fdata-sections \
 -ffreestanding -nostdlib -mcpu=cortex-m55 -mthumb \
 -DPLATFORM_STM32N6 -MMD -MP \
 $(if $(DMA),-DBOARD_DMA) \
 $(if $(filter iwdg,$(WATCHDOG)),-DBOARD_WATCHDOG_IWDG) \
 $(if $(filter wwdg,$(WATCHDOG)),-DBOARD_WATCHDOG_WWDG) \
 -DWHAL_CFG_STM32N6_GPIO_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_RCC_HSI_DRIVER \
 -DWHAL_CFG_STM32N6_RCC_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_UART_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_SPI_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_I2C_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_RNG_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_GPDMA_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_ETH_DIRECT_API_MAPPING \
 -DWHAL_CFG_LAN8742A_ETH_PHY_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_CRYP_ECB_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_CRYP_CBC_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_CRYP_CTR_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_CRYP_GCM_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_CRYP_GMAC_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_CRYP_CCM_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_HASH_SHA1_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_HASH_SHA224_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_HASH_SHA256_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_HASH_HMAC_SHA1_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_HASH_HMAC_SHA224_DIRECT_API_MAPPING \
 -DWHAL_CFG_STM32N6_HASH_HMAC_SHA256_DIRECT_API_MAPPING \
 -DWHAL_CFG_SYSTICK_TIMER_DIRECT_API_MAPPING \
 -DWHAL_CFG_NVIC_IRQ_DIRECT_API_MAPPING
LDFLAGS = --omagic -static --gc-sections

LINKER_SCRIPT ?= $(_BOARD_DIR)/linker.ld

INCLUDE += -I$(_BOARD_DIR) -I$(WHAL_DIR)/boards/peripheral

BOARD_SOURCE = $(_BOARD_DIR)/ivt.c
BOARD_SOURCE += $(_BOARD_DIR)/wolfHAL_board.c
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/sensor.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/watchdog.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/crypto.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/flash.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/block.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/irq/cortex_m4_nvic.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/stm32n6_*.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/lan8742a_*.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/systick.c)

# Peripheral devices
include $(WHAL_DIR)/boards/peripheral/board.mk

# Flash via openocd: make flash BOARD=<board> IMAGE=<path/to/image>
# N6 has no internal flash — load the ELF into SRAM, fetch SP/PC from the
# IVT at 0x34000000, and resume. Requires the board's BOOT pins set to
# development boot mode.
OPENOCD ?= /opt/openocd/bin/openocd
OPENOCD_INTERFACE ?= interface/stlink.cfg
OPENOCD_TARGET ?= target/stm32n6x.cfg

.PHONY: flash
flash:
	@test -n "$(IMAGE)" || { echo "IMAGE=<path/to/image> required" >&2; exit 1; }
	$(OPENOCD) -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) \
	 -c "init; reset halt; load_image $(IMAGE); resume"
