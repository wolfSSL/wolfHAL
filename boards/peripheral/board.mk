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

_PERIPHERAL_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

BOARD_SOURCE += $(_PERIPHERAL_DIR)/wolfHAL_peripheral.c

ifneq ($(filter sdhc_spi_sdcard32gb,$(PERIPHERALS)),)
CFLAGS += -DPERIPHERAL_SDHC_SPI_SDCARD32GB
BOARD_SOURCE += $(_PERIPHERAL_DIR)/block/sdhc_spi_sdcard32gb.c
BOARD_SOURCE += $(WHAL_DIR)/src/block/sdhc_spi_block.c
endif

ifneq ($(filter spi_nor_w25q64,$(PERIPHERALS)),)
CFLAGS += -DPERIPHERAL_SPI_NOR_W25Q64
BOARD_SOURCE += $(_PERIPHERAL_DIR)/flash/spi_nor_w25q64.c
BOARD_SOURCE += $(WHAL_DIR)/src/flash/spi_nor_flash.c
endif

ifneq ($(filter bmi270,$(PERIPHERALS)),)
CFLAGS += -DPERIPHERAL_BMI270
BOARD_SOURCE += $(_PERIPHERAL_DIR)/sensor/imu/bmi270.c
BOARD_SOURCE += $(WHAL_DIR)/src/sensor/imu/bmi270_sensor.c
endif

ifneq ($(filter sharp_ls013b7dh03,$(PERIPHERALS)),)
CFLAGS += -DPERIPHERAL_SHARP_LS013B7DH03
BOARD_SOURCE += $(_PERIPHERAL_DIR)/display/sharp_ls013b7dh03.c
BOARD_SOURCE += $(WHAL_DIR)/src/display/sharp_memory_display.c
BOARD_SOURCE += $(WHAL_DIR)/src/display/display.c
endif
