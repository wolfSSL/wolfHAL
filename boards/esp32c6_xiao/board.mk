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

PLATFORM = esp32c6
TESTS ?= gpio uart flash

GCC_PREFIX ?= riscv-none-elf-
GCC     = $(GCC_PATH)$(GCC_PREFIX)gcc
LD      = $(GCC_PATH)$(GCC_PREFIX)gcc
OBJCOPY = $(GCC_PATH)$(GCC_PREFIX)objcopy

CFLAGS += -Wall -Werror $(INCLUDE) -g3 -Os -ffunction-sections -fdata-sections \
 -ffreestanding -nostdlib -march=rv32imac_zicsr -mabi=ilp32 \
 -DPLATFORM_ESP32C6 -MMD -MP \
 -DWHAL_CFG_ESP32C6_GPIO_DIRECT_API_MAPPING \
 -DWHAL_CFG_ESP32C6_INTC_DIRECT_API_MAPPING
LDFLAGS = -march=rv32imac_zicsr -mabi=ilp32 -ffreestanding -nostartfiles -nostdlib \
 -Wl,--gc-sections -static

LINKER_SCRIPT ?= $(_BOARD_DIR)/linker.ld

INCLUDE += -I$(_BOARD_DIR) -I$(WHAL_DIR)/boards/peripheral

BOARD_SOURCE = $(_BOARD_DIR)/ivt.c
BOARD_SOURCE += $(_BOARD_DIR)/wolfHAL_board.c
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/uart.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/flash.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/block.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/sensor.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/timer.c)
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*/watchdog.c)
BOARD_SOURCE += $(filter-out %/esp32c6_ieee802154.c,$(wildcard $(WHAL_DIR)/src/*/esp32c6_*.c))

ifneq ($(RADIO),)
ESP32C6_BLOB_DIR ?= $(WHAL_DIR)/tools/esp32c6_radio/blobs
ifeq ($(wildcard $(ESP32C6_BLOB_DIR)/libphy.a),)
$(error RADIO=1 needs the Espressif PHY libraries; run tools/esp32c6_radio/fetch_blobs.sh)
endif
CFLAGS += -DBOARD_RADIO
TESTS += ieee802154
BOARD_SOURCE += $(WHAL_DIR)/src/ieee802154/ieee802154.c
BOARD_SOURCE += $(WHAL_DIR)/src/ieee802154/esp32c6_ieee802154.c
LDLIBS += -L$(ESP32C6_BLOB_DIR) -lphy -lbtbb \
 $(ESP32C6_BLOB_DIR)/esp32c6.rom.ld \
 $(ESP32C6_BLOB_DIR)/esp32c6.rom.phy.ld \
 $(ESP32C6_BLOB_DIR)/esp32c6.rom.libgcc.ld
endif

# Peripheral devices
include $(WHAL_DIR)/boards/peripheral/board.mk

# Flash via openocd: make flash BOARD=<board> IMAGE=<path/to/image>
OPENOCD ?= /opt/openocd-esp32/bin/openocd-esp32
OPENOCD_BOARD ?= board/esp32c6-builtin.cfg

.PHONY: flash
flash:
	@test -n "$(IMAGE)" || { echo "IMAGE=<path/to/image> required" >&2; exit 1; }
	$(OPENOCD) -f $(OPENOCD_BOARD) \
	 -c "program_esp $(IMAGE) 0x0 verify exit"
