#!/bin/sh
#
# fetch_blobs.sh
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

# Download the Espressif binaries the ESP32-C6 IEEE 802.15.4 driver links
# against: the closed PHY and baseband libraries, and the ROM linker scripts
# that resolve the ROM routines they call. Nothing here is committed to
# wolfHAL; the files land in ./blobs next to this script.

set -eu

PHY_LIB_REV=20f1db053a0e6cb9f1c09d255c43bf42483041d0
IDF_REV=d445e947156f276dfe0db5804e4edf6ae90fe6fb

PHY_LIB_URL=https://raw.githubusercontent.com/espressif/esp-phy-lib/$PHY_LIB_REV/esp32c6
IDF_ROM_URL=https://raw.githubusercontent.com/espressif/esp-idf/$IDF_REV/components/esp_rom/esp32c6/ld

OUT_DIR=${1:-$(dirname "$0")/blobs}
mkdir -p "$OUT_DIR"

fetch() {
    url=$1
    name=$2
    sum=$3

    if [ -f "$OUT_DIR/$name" ] && \
       echo "$sum  $OUT_DIR/$name" | sha256sum -c - >/dev/null 2>&1; then
        return
    fi

    curl -sSfL -o "$OUT_DIR/$name.tmp" "$url/$name"
    if ! echo "$sum  $OUT_DIR/$name.tmp" | sha256sum -c - >/dev/null 2>&1; then
        rm -f "$OUT_DIR/$name.tmp"
        echo "checksum mismatch: $name" >&2
        exit 1
    fi
    mv "$OUT_DIR/$name.tmp" "$OUT_DIR/$name"
    echo "fetched $name"
}

fetch "$PHY_LIB_URL" libphy.a \
    5ebda577864f5e90a34d90317360a5957d6c5ba133446461d697d65297caf04e
fetch "$PHY_LIB_URL" libbtbb.a \
    1843c31dbd6abfabe33439882bf1ad4ffa5ac60d885da98040c0d909872187e2
fetch "$IDF_ROM_URL" esp32c6.rom.ld \
    e2174360047d1df6a963d3bf49271755fd23b72a65ac85b9ed14aee16035e2f3
fetch "$IDF_ROM_URL" esp32c6.rom.phy.ld \
    7332fd77b2aeb34d5d9881e86e26a7521f2b315b3ad4116082f92eb1b6a19f35
fetch "$IDF_ROM_URL" esp32c6.rom.libgcc.ld \
    6f601e0d26b1c9e46160edab23fad21d73fe3a1a6e0deb616906604609a0c680
