
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


#if !defined(ARRAYKIND)
#error ARRAYKIND not set
#endif

// Extra layers of indirection needed
// to get ARRAYTYPE concatenation,
// which is the array type name.
#define __ARRAYTYPE(x,y) x##y
#define _ARRAYTYPE(x,y) __ARRAYTYPE(x,y)
#define ARRAYTYPE _ARRAYTYPE(array,ARRAYKIND)

// Extra layers of indirection needed
// to get ARRAYTYPEDUPLICATE concatenation.
#define __ARRAYTYPEDUPLICATE(x,y,z) x##y##z
#define _ARRAYTYPEDUPLICATE(x,y,z) __ARRAYTYPEDUPLICATE(x,y,z)
#define ARRAYTYPEDUPLICATE _ARRAYTYPEDUPLICATE(array,ARRAYKIND, duplicate)

// Extra layers of indirection needed
// to get ARRAYTYPEAPPEND1 concatenation.
#define __ARRAYTYPEAPPEND1(x,y,z) x##y##z
#define _ARRAYTYPEAPPEND1(x,y,z) __ARRAYTYPEAPPEND1(x,y,z)
#define ARRAYTYPEAPPEND1 _ARRAYTYPEAPPEND1(array,ARRAYKIND, append1)

// Extra layers of indirection needed
// to get ARRAYTYPEAPPEND2 concatenation.
#define __ARRAYTYPEAPPEND2(x,y,z) x##y##z
#define _ARRAYTYPEAPPEND2(x,y,z) __ARRAYTYPEAPPEND2(x,y,z)
#define ARRAYTYPEAPPEND2 _ARRAYTYPEAPPEND2(array,ARRAYKIND, append2)

// Extra layers of indirection needed
// to get ARRAYTYPEINSERT1 concatenation.
#define __ARRAYTYPEINSERT1(x,y,z) x##y##z
#define _ARRAYTYPEINSERT1(x,y,z) __ARRAYTYPEINSERT1(x,y,z)
#define ARRAYTYPEINSERT1 _ARRAYTYPEINSERT1(array,ARRAYKIND, insert1)

// Extra layers of indirection needed
// to get ARRAYTYPEINSERT2 concatenation.
#define __ARRAYTYPEINSERT2(x,y,z) x##y##z
#define _ARRAYTYPEINSERT2(x,y,z) __ARRAYTYPEINSERT2(x,y,z)
#define ARRAYTYPEINSERT2 _ARRAYTYPEINSERT2(array,ARRAYKIND, insert2)

// Extra layers of indirection needed
// to get ARRAYTYPEOVERWRITE1 concatenation.
#define __ARRAYTYPEOVERWRITE1(x,y,z) x##y##z
#define _ARRAYTYPEOVERWRITE1(x,y,z) __ARRAYTYPEOVERWRITE1(x,y,z)
#define ARRAYTYPEOVERWRITE1 _ARRAYTYPEOVERWRITE1(array,ARRAYKIND, overwrite1)

// Extra layers of indirection needed
// to get ARRAYTYPEOVERWRITE2 concatenation.
#define __ARRAYTYPEOVERWRITE2(x,y,z) x##y##z
#define _ARRAYTYPEOVERWRITE2(x,y,z) __ARRAYTYPEOVERWRITE2(x,y,z)
#define ARRAYTYPEOVERWRITE2 _ARRAYTYPEOVERWRITE2(array,ARRAYKIND, overwrite2)

// Extra layers of indirection needed
// to get ARRAYTYPESPLIT concatenation.
#define __ARRAYTYPESPLIT(x,y,z) x##y##z
#define _ARRAYTYPESPLIT(x,y,z) __ARRAYTYPESPLIT(x,y,z)
#define ARRAYTYPESPLIT _ARRAYTYPESPLIT(array,ARRAYKIND, split)

// Extra layers of indirection needed
// to get ARRAYTYPEREMOVE concatenation.
#define __ARRAYTYPEREMOVE(x,y,z) x##y##z
#define _ARRAYTYPEREMOVE(x,y,z) __ARRAYTYPEREMOVE(x,y,z)
#define ARRAYTYPEREMOVE _ARRAYTYPEREMOVE(array,ARRAYKIND, remove)

// Extra layers of indirection needed
// to get ARRAYTYPETRUNCATE concatenation.
#define __ARRAYTYPETRUNCATE(x,y,z) x##y##z
#define _ARRAYTYPETRUNCATE(x,y,z) __ARRAYTYPETRUNCATE(x,y,z)
#define ARRAYTYPETRUNCATE _ARRAYTYPETRUNCATE(array,ARRAYKIND, truncate)

// Extra layers of indirection needed
// to get ARRAYTYPECHOP concatenation.
#define __ARRAYTYPECHOP(x,y,z) x##y##z
#define _ARRAYTYPECHOP(x,y,z) __ARRAYTYPECHOP(x,y,z)
#define ARRAYTYPECHOP _ARRAYTYPECHOP(array,ARRAYKIND, chop)

// Extra layers of indirection needed
// to get ARRAYTYPEITERATE concatenation.
#define __ARRAYTYPEITERATE(x,y,z) x##y##z
#define _ARRAYTYPEITERATE(x,y,z) __ARRAYTYPEITERATE(x,y,z)
#define ARRAYTYPEITERATE _ARRAYTYPEITERATE(array,ARRAYKIND, iterate)

// Extra layers of indirection needed
// to get ARRAYTYPEITERATECALLBACK concatenation.
#define __ARRAYTYPEITERATECALLBACK(x,y,z) x##y##z
#define _ARRAYTYPEITERATECALLBACK(x,y,z) __ARRAYTYPEITERATECALLBACK(x,y,z)
#define ARRAYTYPEITERATECALLBACK _ARRAYTYPEITERATECALLBACK(array,ARRAYKIND, iteratecallback)

// Extra layers of indirection needed
// to get ARRAYTYPEFREE concatenation.
#define __ARRAYTYPEFREE(x,y,z) x##y##z
#define _ARRAYTYPEFREE(x,y,z) __ARRAYTYPEFREE(x,y,z)
#define ARRAYTYPEFREE _ARRAYTYPEFREE(array,ARRAYKIND, free)

// Extra layers of indirection needed
// to get ARRAYTYPEFREECALLBACK concatenation.
#define __ARRAYTYPEFREECALLBACK(x,y,z) x##y##z
#define _ARRAYTYPEFREECALLBACK(x,y,z) __ARRAYTYPEFREECALLBACK(x,y,z)
#define ARRAYTYPEFREECALLBACK _ARRAYTYPEFREECALLBACK(array,ARRAYKIND, freecallback)

// Struct representing an array.
typedef struct {
	// Pointer to an array of ARRAYKIND
	// allocated through mmalloc().
	// It must be modified only through
	// ARRAYTYPE*() functions, and
	// freed only through mmrefdown()
	// as it may be multi-referenced.
	ARRAYKIND* ptr;
	
} ARRAYTYPE;

// Return a duplicate of the
// ARRAYTYPE given as argument.
ARRAYTYPE ARRAYTYPEDUPLICATE (ARRAYTYPE a);

// This function append to the ARRAYTYPE,
// space for a single ARRAYKIND and return
// a pointer to the newly appended space.
ARRAYKIND* ARRAYTYPEAPPEND1 (ARRAYTYPE* a);

// This function append to the ARRAYTYPE,
// space for the count (given as argument)
// of ARRAYKIND and return a pointer
// to the newly appended space.
ARRAYKIND* ARRAYTYPEAPPEND2 (ARRAYTYPE* a, uint count);

// This function insert into the ARRAYTYPE,
// at the given index, space for a single ARRAYKIND
// and return a pointer to the newly inserted space.
// Null is returned if the index is greater
// than the number of elements in the ARRAYTYPE.
ARRAYKIND* ARRAYTYPEINSERT1 (ARRAYTYPE* a, uint index);

// This function insert into the ARRAYTYPE,
// at the given index, space for the count (given as argument)
// of ARRAYKIND and return a pointer to the newly inserted space.
// Null is returned if the index is greater
// than the number of elements in the ARRAYTYPE.
ARRAYKIND* ARRAYTYPEINSERT2 (ARRAYTYPE* a, uint index, uint count);

// This function overwrite the ARRAYTYPE, at
// the given index, with space for a single ARRAYKIND
// and return a pointer to the newly overwritten space.
// Null is returned if the index is greater
// than the number of elements in the ARRAYTYPE.
ARRAYKIND* ARRAYTYPEOVERWRITE1 (ARRAYTYPE* a, uint index);

// This function overwrite the ARRAYTYPE,
// at the given index, with space for
// the count (given as argument) of ARRAYKIND
// and return a pointer to the newly overwritten space.
// Null is returned if the index is greater
// than the number of elements in the ARRAYTYPE.
ARRAYKIND* ARRAYTYPEOVERWRITE2 (ARRAYTYPE* a, uint index, uint count);

// TODO: Implement:
// void ARRAYTYPEMOVEUP1 (ARRAYTYPE* a, uint displacement, uint index);
// void ARRAYTYPEMOVEUP2 (ARRAYTYPE* a, uint displacement, uint index, uint count);
// void ARRAYTYPEMOVEDOWN1 (ARRAYTYPE* a, uint displacement, uint index);
// void ARRAYTYPEMOVEDOWN2 (ARRAYTYPE* a, uint displacement, uint index, uint count);
// void ARRAYTYPEMOVETOTOP1 (ARRAYTYPE* a, uint index);
// void ARRAYTYPEMOVETOTOP2 (ARRAYTYPE* a, uint index, uint count);
// void ARRAYTYPEMOVETOBOTTOM1 (ARRAYTYPE* a, uint index);
// void ARRAYTYPEMOVETOBOTTOM2 (ARRAYTYPE* a, uint index, uint count);

// This function split the ARRAYTYPE
// at the index given as argument,
// and return the bottom part of the split.
// ARRAYTYPENULL is returned if the index
// is greater than or equal to the number
// of elements in the ARRAYTYPE.
ARRAYTYPE ARRAYTYPESPLIT (ARRAYTYPE* a, uint index);

// This function remove from the ARRAYTYPE
// given as argument, the elements specified
// by the arguments index and count.
// Nothing is done if the argument count
// is null or if the range specified
// by the arguments index and count fall
// outside of the ARRAYTYPE given as argument.
void ARRAYTYPEREMOVE (ARRAYTYPE* a, uint index, uint count);

// This function truncate the ARRAYTYPE
// given as argument to the count
// of elements given as argument.
// Nothing is done if the argument count
// is greater than or equal to the number
// of elements in the ARRAYTYPE given as argument.
void ARRAYTYPETRUNCATE (ARRAYTYPE* a, uint count);

// This function remove the count of
// elements given as argument from the end
// of the ARRAYTYPE given as argument.
// Nothing is done if the argument count
// is null or greater than the number of
// elements in the ARRAYTYPE given as argument.
void ARRAYTYPECHOP (ARRAYTYPE* a, uint count);

// This function iterate through each element of the array,
// starting from its first element, and for each element
// of the array, ARRAYTYPEITERATECALLBACK is called,
// and a reference to element's memory location is passed,
// allowing the callback function to modify the element value.
// Iterating through the array is interrupted if the callback
// function return 0.
// The argument ARRAYTYPEITERATECALLBACK must not be null.
void ARRAYTYPEITERATE (ARRAYTYPE a, uint(* ARRAYTYPEITERATECALLBACK)(ARRAYKIND*));

// This function free the memory used by the array.
// It iterate through each element of the array,
// starting from its first element, and for each element
// of the array, ARRAYTYPEFREECALLBACK is called.
// The argument ARRAYTYPEFREECALLBACK must not be null.
// If iterating through each element of the array
// is not needed, mmfree(a.ptr) can simply be used.
void ARRAYTYPEFREE (ARRAYTYPE a, void(* ARRAYTYPEFREECALLBACK)(ARRAYKIND));
