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

/*
 * ESP32-C6 (RV32IMAC) Direct Boot layout.
 *
 * The ROM bootloader recognizes the 0xaedb041d magic (first two 32-bit words
 * of the flashed image), maps the external SPI flash into the address space at
 * 0x4200_0000 (unified instruction/data bus, TRM Table 5.3-1), and jumps to
 * 0x4200_0008 - the instruction right after the 8-byte magic.
 *
 * Code and read-only data execute in place (XIP) from the flash window.
 * Initialized data, zero-initialized data, and the stack live in HP SRAM,
 * 512 KB at 0x4080_0000. .data is copied from its flash load address into SRAM
 * by the startup code in ivt.c.
 */

/* ELF entry symbol (used by a debugger's `load`+run; the ROM ignores it and
 * jumps to the fixed 0x4200_0008 offset in Direct Boot). */
ENTRY(_start)

MEMORY
{
    /* External flash mapped by the ROM into the XIP window. 4 MB is the common
     * DevKitC-1 part; only what the image uses is consumed. */
    FLASH (rx)  : ORIGIN = 0x42000000, LENGTH = 4M
    /* HP SRAM: 512 KB, 0x4080_0000 - 0x4087_FFFF. */
    RAM   (rwx) : ORIGIN = 0x40800000, LENGTH = 512K
}

/* Top of HP SRAM. The stack grows down from here. 0x4088_0000 is 16-byte
 * aligned, satisfying the RV32 ILP32 stack-alignment requirement. */
_estack = ORIGIN(RAM) + LENGTH(RAM);
/* Link error if the stack cannot fit alongside .data/.bss. */
_Min_Stack_Size = 0x800;

SECTIONS
{
    /* Direct Boot header: the two magic words the ROM checks. Kept first so it
     * lands at flash offset 0x0; _start (below) then sits at offset 0x8. */
    .boot_hdr :
    {
        . = ALIGN(4);
        LONG(0xaedb041d)   /* Direct Boot magic, word 0 */
        LONG(0xaedb041d)   /* Direct Boot magic, word 1; ROM jumps to here + 4 */
    } > FLASH

    /* Program code, XIP from flash. _start is forced first so it is exactly at
     * 0x4200_0008, where the ROM transfers control. */
    .text :
    {
        . = ALIGN(4);
        KEEP(*(.text.entry))   /* _start */
        *(.text)
        *(.text*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Read-only data, also XIP from flash. */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        *(.srodata)
        *(.srodata*)
        . = ALIGN(4);
    } > FLASH

    /* Machine trap vector table. mtvec requires a 256-byte aligned base
     * (TRM Reg 1.9), so give it its own aligned output section. */
    .vectors : ALIGN(256)
    {
        KEEP(*(.vectors))
    } > FLASH

    /* Flash load address of .data, read by the startup copy loop. */
    _sidata = LOADADDR(.data);

    /* Initialized data: runs from SRAM (VMA), initial image stored in flash
     * (LMA) after the code and copied in by _start. */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        /* gp anchors gp-relative access to .sdata/.sbss within a +/-2 KB
         * window; place it 0x800 past the small-data start. */
        PROVIDE(__global_pointer$ = . + 0x800);
        *(.data)
        *(.data*)
        *(.sdata)
        *(.sdata*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH

    /* Zero-initialized data, cleared by _start. */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.sbss)
        *(.sbss*)
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Reserve and range-check the stack. */
    ._stack (NOLOAD) :
    {
        . = ALIGN(16);
        PROVIDE(_end = .);
        . = . + _Min_Stack_Size;
        . = ALIGN(16);
    } > RAM

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.eh_frame*)
        *(.comment)
        *(.riscv.attributes)
    }
}
