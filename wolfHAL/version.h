/* version.h
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

#ifndef WHAL_VERSION_H
#define WHAL_VERSION_H

/**
 * @file version.h
 * @brief wolfHAL release version.
 */

/** @brief Major version. */
#define WHAL_VERSION_MAJOR  1
/** @brief Minor version. */
#define WHAL_VERSION_MINOR  0
/** @brief Patch version. */
#define WHAL_VERSION_PATCH  0

/** @brief Version as a string, e.g. "1.0.0". */
#define WHAL_VERSION_STRING  "1.0.0"

/** @brief Version as 0xMMmmpppp (major, minor, patch) for compile-time comparison. */
#define WHAL_VERSION_HEX  ((WHAL_VERSION_MAJOR << 24) | \
                           (WHAL_VERSION_MINOR << 16) | \
                           (WHAL_VERSION_PATCH))

#endif /* WHAL_VERSION_H */
