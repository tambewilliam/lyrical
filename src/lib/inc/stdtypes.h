
// ---------------------------------------------------------------------
// Copyright (c) William Fonkou Tambe
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------


#ifndef LIBSTDTYPES
#define LIBSTDTYPES

#include <stdint.h>

// Signed types.
#define s8 int8_t
#if defined(__INT16_MAX__)
#define s16 int16_t
#endif
#if defined(__INT32_MAX__)
#define s32 int32_t
#endif
#if defined(__INT64_MAX__)
#define s64 int64_t
#endif
#define sint intptr_t

// Unsigned types.
#define u8 uint8_t
#if defined(__UINT16_MAX__)
#define u16 uint16_t
#endif
#if defined(__UINT32_MAX__)
#define u32 uint32_t
#endif
#if defined(__UINT64_MAX__)
#define u64 uint64_t
#endif
#define uint uintptr_t

#endif
