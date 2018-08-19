
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


#ifndef LIBARRAYUINT
#define LIBARRAYUINT

#define ARRAYKIND uint
#include "template/array/array.h"
#undef ARRAYKIND

// Because uint is a macro expanding
// to uintptr_t, these macros are created
// in order to get the right names.
#define arrayuint arrayuintptr_t
#define arrayuintduplicate arrayuintptr_tduplicate
#define arrayuintappend1 arrayuintptr_tappend1
#define arrayuintappend2 arrayuintptr_tappend2
#define arrayuintinsert1 arrayuintptr_tinsert1
#define arrayuintoverwrite1 arrayuintptr_toverwrite1
#define arrayuintoverwrite2 arrayuintptr_toverwrite2
#define arrayuintmoveup1 arrayuintptr_tmoveup1
#define arrayuintmoveup2 arrayuintptr_tmoveup2
#define arrayuintmovedown1 arrayuintptr_tmovedown1
#define arrayuintmovedown2 arrayuintptr_tmovedown2
#define arrayuintmovetotop1 arrayuintptr_tmovetotop1
#define arrayuintmovetotop2 arrayuintptr_tmovetotop2
#define arrayuintmovetobottom1 arrayuintptr_tmovetobottom1
#define arrayuintmovetobottom2 arrayuintptr_tmovetobottom2
#define arrayuintsplit arrayuintptr_tsplit
#define arrayuintremove arrayuintptr_tremove
#define arrayuinttruncate arrayuintptr_ttruncate
#define arrayuintchop arrayuintptr_tchop
#define arrayuintiterate arrayuintptr_titerate
#define arrayuintfree arrayuintptr_tfree

// Null value useful for initialization.
#define arrayuintnull ((arrayuint){.ptr = 0})

// Return the number of
// elements in the arrayuint.
#define arrayuintsz(a) (a.ptr?(mmsz(a.ptr)/sizeof(uint)):0)

#endif
