/* linker.ld
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

OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)

/* Low DDR (AM026 address map). The image and its stack sit in 8 MB at
 * 8 MB, clear of the first MB that boot firmware may use. */
MEMORY
{
    DDR (rwx) : ORIGIN = 0x00800000, LENGTH = 8M
}

SECTIONS
{
    .text : {
        KEEP(*(.text.boot))
        *(.text*)
    } > DDR

    .rodata : {
        *(.rodata*)
    } > DDR

    .data : {
        *(.data*)
    } > DDR

    .bss (NOLOAD) : {
        . = ALIGN(8);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(8);
        _ebss = .;
    } > DDR

    . = ALIGN(16);
    . = . + 0x10000;
    _estack = .;
}
