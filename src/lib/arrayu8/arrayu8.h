
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


#ifndef LIBARRAYU8
#define LIBARRAYU8

#define ARRAYKIND u8
#include "template/array/array.h"
#undef ARRAYKIND

// Because u8 is a macro expanding
// to uint8_t, these macros are created
// in order to get the right names.
#define arrayu8 arrayuint8_t
#define arrayu8duplicate arrayuint8_tduplicate
#define arrayu8append1 arrayuint8_tappend1
#define arrayu8append2 arrayuint8_tappend2
#define arrayu8insert1 arrayuint8_tinsert1
#define arrayu8overwrite1 arrayuint8_toverwrite1
#define arrayu8overwrite2 arrayuint8_toverwrite2
#define arrayu8moveup1 arrayuint8_tmoveup1
#define arrayu8moveup2 arrayuint8_tmoveup2
#define arrayu8movedown1 arrayuint8_tmovedown1
#define arrayu8movedown2 arrayuint8_tmovedown2
#define arrayu8movetotop1 arrayuint8_tmovetotop1
#define arrayu8movetotop2 arrayuint8_tmovetotop2
#define arrayu8movetobottom1 arrayuint8_tmovetobottom1
#define arrayu8movetobottom2 arrayuint8_tmovetobottom2
#define arrayu8split arrayuint8_tsplit
#define arrayu8remove arrayuint8_tremove
#define arrayu8truncate arrayuint8_ttruncate
#define arrayu8chop arrayuint8_tchop
#define arrayu8iterate arrayuint8_titerate
#define arrayu8free arrayuint8_tfree

// Null value useful for initialization.
#define arrayu8null ((arrayu8){.ptr = 0})

// Return the number of
// elements in the arrayu8.
#define arrayu8sz(a) (a.ptr?(mmsz(a.ptr)/sizeof(u8)):0)

#endif
