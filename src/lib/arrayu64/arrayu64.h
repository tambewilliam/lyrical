
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


#ifndef LIBARRAYU64
#define LIBARRAYU64

#define ARRAYKIND u64
#include "template/array/array.h"
#undef ARRAYKIND

// Because u64 is a macro expanding
// to uint64_t, these macros are created
// in order to get the right names.
#define arrayu64 arrayuint64_t
#define arrayu64duplicate arrayuint64_tduplicate
#define arrayu64append1 arrayuint64_tappend1
#define arrayu64append2 arrayuint64_tappend2
#define arrayu64insert1 arrayuint64_tinsert1
#define arrayu64overwrite1 arrayuint64_toverwrite1
#define arrayu64overwrite2 arrayuint64_toverwrite2
#define arrayu64moveup1 arrayuint64_tmoveup1
#define arrayu64moveup2 arrayuint64_tmoveup2
#define arrayu64movedown1 arrayuint64_tmovedown1
#define arrayu64movedown2 arrayuint64_tmovedown2
#define arrayu64movetotop1 arrayuint64_tmovetotop1
#define arrayu64movetotop2 arrayuint64_tmovetotop2
#define arrayu64movetobottom1 arrayuint64_tmovetobottom1
#define arrayu64movetobottom2 arrayuint64_tmovetobottom2
#define arrayu64split arrayuint64_tsplit
#define arrayu64remove arrayuint64_tremove
#define arrayu64truncate arrayuint64_ttruncate
#define arrayu64chop arrayuint64_tchop
#define arrayu64iterate arrayuint64_titerate
#define arrayu64free arrayuint64_tfree

// Null value useful for initialization.
#define arrayu64null ((arrayu64){.ptr = 0})

// Return the number of
// elements in the arrayu64.
#define arrayu64sz(a) (a.ptr?(mmsz(a.ptr)/sizeof(u64)):0)

#endif
