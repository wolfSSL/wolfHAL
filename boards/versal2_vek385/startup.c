/* startup.c
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

#include <stdint.h>
#include <stddef.h>

/* No C library: tests and compiler-made struct copies call these. */
void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = dst;
    const uint8_t *s = src;

    while (n > 0) {
        *d++ = *s++;
        n--;
    }
    return dst;
}

void *memset(void *dst, int c, size_t n)
{
    uint8_t *d = dst;

    while (n > 0) {
        *d++ = (uint8_t)c;
        n--;
    }
    return dst;
}

extern uint64_t _sbss;
extern uint64_t _ebss;
extern void main(void);

void Reset_Handler(void);

/* Entry on core 0 with the MMU and caches off, as QEMU and a JTAG load
 * leave it: set the stack, then go to C. */
__asm__(
    "    .section .text.boot, \"ax\"\n"
    "    .global _start\n"
    "_start:\n"
    "    ldr x0, =_estack\n"
    "    mov sp, x0\n"
    "    bl Reset_Handler\n"
    "1:  wfi\n"
    "    b 1b\n"
);

void Reset_Handler(void)
{
    uint64_t *p;

    for (p = &_sbss; p < &_ebss; p++) {
        *p = 0;
    }

    main();
}
