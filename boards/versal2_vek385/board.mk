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

PLATFORM = versal2
TESTS ?= uart pl011_uart

GCC = $(GCC_PATH)aarch64-linux-gnu-gcc
LD = $(GCC_PATH)aarch64-linux-gnu-gcc
OBJCOPY = $(GCC_PATH)aarch64-linux-gnu-objcopy

# The MMU is off, so all memory is Device memory: no unaligned accesses.
CFLAGS += -Wall -Werror $(INCLUDE) -g3 -Os -ffunction-sections -fdata-sections \
 -ffreestanding -nostdlib -mcpu=cortex-a78 -mstrict-align -mgeneral-regs-only \
 -DPLATFORM_VERSAL2 -MMD -MP
LDFLAGS = -ffreestanding -nostdlib -nostartfiles -static \
 -Wl,--gc-sections -Wl,--build-id=none -Wl,--no-warn-rwx-segments

LINKER_SCRIPT ?= $(_BOARD_DIR)/linker.ld

INCLUDE += -I$(_BOARD_DIR)

BOARD_SOURCE = $(_BOARD_DIR)/startup.c
BOARD_SOURCE += $(_BOARD_DIR)/wolfHAL_board.c
BOARD_SOURCE += $(wildcard $(WHAL_DIR)/src/*.c)
BOARD_SOURCE += $(WHAL_DIR)/src/gpio/gpio.c
BOARD_SOURCE += $(WHAL_DIR)/src/uart/uart.c
BOARD_SOURCE += $(WHAL_DIR)/src/uart/pl011_uart.c

# Run the tests on QEMU (10.1 or later, amd-versal2-virt machine):
#   make BOARD=versal2_vek385 qemu
# UART0 (loopback tests) goes nowhere, UART1 (results) to qemu.out.
# The tests never exit, so QEMU is stopped once the results are out.
QEMU ?= qemu-system-aarch64
QEMU_TIMEOUT ?= 60
QEMU_OUT = $(BUILD_DIR)/qemu.out

.PHONY: qemu
qemu: $(BUILD_DIR)/test_hw.elf
	@rm -f $(QEMU_OUT)
	@$(QEMU) -M amd-versal2-virt -m 2G -display none -monitor none \
	 -serial null -serial file:$(QEMU_OUT) \
	 -device loader,file=$<,cpu-num=0 & pid=$$!; \
	 i=0; \
	 while [ $$i -lt $(QEMU_TIMEOUT) ] && \
	       ! grep -q 'Failed: [0-9]' $(QEMU_OUT) 2>/dev/null; do \
	     sleep 1; i=$$((i + 1)); \
	 done; \
	 kill $$pid 2>/dev/null; wait $$pid 2>/dev/null; \
	 tr -d '\r' < $(QEMU_OUT); \
	 grep -q 'Failed: 0$$' $(QEMU_OUT)
