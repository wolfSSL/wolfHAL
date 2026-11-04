# Makefile
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

WHAL_DIR = $(CURDIR)/../..

INCLUDE = -I$(WHAL_DIR)
CFLAGS += -Wall -Werror $(INCLUDE) -g3

TEST_SRC = main.c test_bitops.c test_dispatch.c test_endian.c test_reg.c test_timeout.c \
           test_pl011_uart.c

WHAL_SRC = $(WHAL_DIR)/src/reg.c \
           $(WHAL_DIR)/src/gpio/gpio.c \
           $(WHAL_DIR)/src/uart/uart.c \
           $(WHAL_DIR)/src/uart/pl011_uart.c \
           $(WHAL_DIR)/src/flash/flash.c \
           $(WHAL_DIR)/src/timer/timer.c \
           $(WHAL_DIR)/src/spi/spi.c \
           $(WHAL_DIR)/src/rng/rng.c \
           $(WHAL_DIR)/src/block/block.c \
           $(WHAL_DIR)/src/pwm/pwm.c \
           $(WHAL_DIR)/src/display/display.c \
           $(WHAL_DIR)/src/crypto/crypto.c \
           $(WHAL_DIR)/src/crypto/aes_ecb.c \
           $(WHAL_DIR)/src/crypto/aes_cbc.c \
           $(WHAL_DIR)/src/crypto/aes_ctr.c \
           $(WHAL_DIR)/src/crypto/aes_gcm.c \
           $(WHAL_DIR)/src/crypto/aes_gmac.c \
           $(WHAL_DIR)/src/crypto/aes_ccm.c \
           $(WHAL_DIR)/src/crypto/sha1.c \
           $(WHAL_DIR)/src/crypto/sha224.c \
           $(WHAL_DIR)/src/crypto/sha256.c \
           $(WHAL_DIR)/src/crypto/hmac_sha1.c \
           $(WHAL_DIR)/src/crypto/hmac_sha224.c \
           $(WHAL_DIR)/src/crypto/hmac_sha256.c

SOURCE = $(TEST_SRC) $(WHAL_SRC)
OBJECTS = $(patsubst %.c,%.o,$(SOURCE))

TARGET = test_core

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

.PHONY: run clean

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)
