
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


// This function overwrite the ARRAYTYPE, at
// the given index, with space for a single ARRAYKIND
// and return a pointer to the newly overwritten space.
// Null is returned if the index is greater
// than the number of elements in the ARRAYTYPE.
ARRAYKIND* ARRAYTYPEOVERWRITE1 (ARRAYTYPE* a, uint index) {
	
	uint offset = index*sizeof(ARRAYKIND);
	
	ARRAYKIND* aptr = a->ptr;
	
	uint asz = aptr ? mmsz(aptr) : 0;
	
	if (offset == asz) return ARRAYTYPEAPPEND1(a);
	
	if (offset > asz) return 0;
	
	// mmrealloc() is used so that
	// the memory block get duplicated
	// if it was multi-referenced.
	aptr =  mmrealloc(aptr, -1);
	
	a->ptr = aptr;
	
	return (ARRAYKIND*)((void*)aptr+offset);
}


// This function overwrite the ARRAYTYPE,
// at the given index, with space for
// the count (given as argument) of ARRAYKIND
// and return a pointer to the newly overwritten space.
// Null is returned if the index is greater
// than the number of elements in the ARRAYTYPE.
ARRAYKIND* ARRAYTYPEOVERWRITE2 (ARRAYTYPE* a, uint index, uint count) {
	
	uint offset = index*sizeof(ARRAYKIND);
	
	ARRAYKIND* aptr = a->ptr;
	
	uint asz = aptr ? mmsz(aptr) : 0;
	
	if (offset == asz) return ARRAYTYPEAPPEND2(a, count);
	
	if (offset > asz) return 0;
	
	uint extrasz = (offset+(count*sizeof(ARRAYKIND)));
	
	extrasz = (extrasz > asz) ? (extrasz-asz) : 0;
	
	if (extrasz) {
		
		asz += extrasz;
		
		aptr = mmrealloc(aptr, asz);
		
	} else {
		// mmrealloc() is used so that
		// the memory block get duplicated
		// if it was multi-referenced.
		aptr =  mmrealloc(aptr, -1);
	}
	
	a->ptr = aptr;
	
	return (ARRAYKIND*)((void*)aptr+offset);
}
