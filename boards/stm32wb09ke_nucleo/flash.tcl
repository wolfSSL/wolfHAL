# flash.tcl
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

# Program a raw .bin into STM32WB09 main flash by driving the flash
# controller registers directly (RM0505 section 9.3), bypassing the
# OpenOCD bluenrg-x flash driver.
#
# Usage:
#   openocd -f interface/stlink-dap.cfg -f target/bluenrg-x.cfg \
#           -c "set IMAGE <path/to/image.bin>" -f flash.tcl
#
# The image is staged through SRAM in 32 KB chunks: each chunk is loaded
# with load_image into a 0xFF-filled RAM window, its pages are erased, and
# its non-blank words are programmed and read back for verification.

set FC        0x40001000
set COMMAND   [expr {$FC + 0x00}]
set IRQSTAT   [expr {$FC + 0x08}]
set IRQRAW    [expr {$FC + 0x10}]
set ADDRESS   [expr {$FC + 0x18}]
set PAGEPROT0 [expr {$FC + 0x34}]
set PAGEPROT1 [expr {$FC + 0x38}]
set DATA0     [expr {$FC + 0x40}]

set FLASH_BASE 0x10040000
set FLASH_SIZE 0x80000
set PAGE_SIZE  0x800
set RAM_WIN    0x20008000
set CHUNK      0x8000

proc rd {a} { return [lindex [read_memory $a 32 1] 0] }
proc wr {a v} { write_memory $a 32 [list $v] }

# Issue one controller command and wait for CMDSTART then CMDDONE.
proc run_cmd {cmd what} {
    global IRQSTAT IRQRAW COMMAND
    wr $IRQSTAT 0xF
    wr $COMMAND $cmd
    foreach {bit name} {2 CMDSTART 1 CMDDONE} {
        set ok 0
        for {set i 0} {$i < 2000} {incr i} {
            set irq [rd $IRQRAW]
            if {$irq & 0xC} {
                wr $IRQSTAT 0xF
                error [format "%s: flash error IRQRAW=0x%08x" $what $irq]
            }
            if {$irq & $bit} { set ok 1; break }
        }
        if {!$ok} {
            error [format "%s: timeout waiting for %s IRQRAW=0x%08x" \
                   $what $name [rd $IRQRAW]]
        }
        wr $IRQSTAT $bit
    }
}

if {![info exists IMAGE]} { error "set IMAGE <path/to/image.bin> first" }
if {[file extension $IMAGE] ne ".bin"} { error "IMAGE must be a raw .bin" }
set size [file size $IMAGE]
if {$size == 0 || $size > $FLASH_SIZE} {
    error [format "image size %d out of range" $size]
}

init
reset halt
halt

wr $PAGEPROT0 0
wr $PAGEPROT1 0

for {set off 0} {$off < $size} {incr off $CHUNK} {
    set len [expr {$size - $off}]
    if {$len > $CHUNK} { set len $CHUNK }
    set nwords [expr {($len + 3) / 4}]
    set npages [expr {($len + $PAGE_SIZE - 1) / $PAGE_SIZE}]

    mww $RAM_WIN 0xFFFFFFFF [expr {$CHUNK / 4}]
    load_image $IMAGE [expr {$RAM_WIN - $off}] bin $RAM_WIN $CHUNK
    set words [read_memory $RAM_WIN 32 $nwords]

    set firstPage [expr {$off / $PAGE_SIZE}]
    for {set p $firstPage} {$p < $firstPage + $npages} {incr p} {
        wr $ADDRESS [expr {$p * $PAGE_SIZE / 4}]
        run_cmd 0x11 "erase page $p"
    }

    set base [expr {$off / 4}]
    for {set i 0} {$i < $nwords} {incr i} {
        set w [lindex $words $i]
        if {$w != 0xFFFFFFFF} {
            wr $ADDRESS [expr {$base + $i}]
            wr $DATA0 $w
            run_cmd 0x33 [format "write word %d" [expr {$base + $i}]]
        }
    }

    set rb [read_memory [expr {$FLASH_BASE + $off}] 32 $nwords]
    for {set i 0} {$i < $nwords} {incr i} {
        if {[lindex $rb $i] != [lindex $words $i]} {
            error [format "verify failed @0x%08x: got 0x%08x want 0x%08x" \
                   [expr {$FLASH_BASE + $off + $i * 4}] \
                   [lindex $rb $i] [lindex $words $i]]
        }
    }
    echo [format "programmed 0x%08x-0x%08x" [expr {$FLASH_BASE + $off}] \
          [expr {$FLASH_BASE + $off + $len - 1}]]
}

echo "verify ok"
reset run
shutdown
