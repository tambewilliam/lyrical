
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


#ifndef LIBARRAYU32
#define LIBARRAYU32

#define ARRAYKIND u32
#include "template/array/array.h"
#undef ARRAYKIND

// Because u32 is a macro expanding
// to uint32_t, these macros are created
// in order to get the right names.
#define arrayu32 arrayuint32_t
#define arrayu32duplicate arrayuint32_tduplicate
#define arrayu32append1 arrayuint32_tappend1
#define arrayu32append2 arrayuint32_tappend2
#define arrayu32insert1 arrayuint32_tinsert1
#define arrayu32overwrite1 arrayuint32_toverwrite1
#define arrayu32overwrite2 arrayuint32_toverwrite2
#define arrayu32moveup1 arrayuint32_tmoveup1
#define arrayu32moveup2 arrayuint32_tmoveup2
#define arrayu32movedown1 arrayuint32_tmovedown1
#define arrayu32movedown2 arrayuint32_tmovedown2
#define arrayu32movetotop1 arrayuint32_tmovetotop1
#define arrayu32movetotop2 arrayuint32_tmovetotop2
#define arrayu32movetobottom1 arrayuint32_tmovetobottom1
#define arrayu32movetobottom2 arrayuint32_tmovetobottom2
#define arrayu32split arrayuint32_tsplit
#define arrayu32remove arrayuint32_tremove
#define arrayu32truncate arrayuint32_ttruncate
#define arrayu32chop arrayuint32_tchop
#define arrayu32iterate arrayuint32_titerate
#define arrayu32free arrayuint32_tfree

// Null value useful for initialization.
#define arrayu32null ((arrayu32){.ptr = 0})

// Return the number of
// elements in the arrayu32.
#define arrayu32sz(a) (a.ptr?(mmsz(a.ptr)/sizeof(u32)):0)

#endif
