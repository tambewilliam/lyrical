
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


// This function insert into the ARRAYTYPE,
// at the given index, space for a single ARRAYKIND
// and return a pointer to the newly inserted space.
// Null is returned if the index is greater
// than the number of elements in the ARRAYTYPE.
ARRAYKIND* ARRAYTYPEINSERT1 (ARRAYTYPE* a, uint index) {
	
	uint offset = index*sizeof(ARRAYKIND);
	
	ARRAYKIND* aptr = a->ptr;
	
	uint asz = aptr ? mmsz(aptr) : 0;
	
	if (offset == asz) return ARRAYTYPEAPPEND1(a);
	
	if (offset > asz) return 0;
	
	aptr = mmrealloc(aptr, asz+sizeof(ARRAYKIND));
	
	ARRAYKIND* aptrindexed = (void*)aptr+offset;
	
	// Move to the right, the block on the right
	// of the position where insertion will be done.
	bytcpy((void*)aptrindexed+sizeof(ARRAYKIND), aptrindexed, asz-offset);
	
	a->ptr = aptr;
	
	return aptrindexed;
}


// This function insert into the ARRAYTYPE,
// at the given index, space for the count (given as argument)
// of ARRAYKIND and return a pointer to the newly inserted space.
// Null is returned if the index is greater
// than the number of elements in the ARRAYTYPE.
ARRAYKIND* ARRAYTYPEINSERT2 (ARRAYTYPE* a, uint index, uint count) {
	
	uint offset = index*sizeof(ARRAYKIND);
	
	ARRAYKIND* aptr = a->ptr;
	
	uint asz = aptr ? mmsz(aptr) : 0;
	
	if (offset == asz) return ARRAYTYPEAPPEND2(a, count);
	
	if (offset > asz) return 0;
	
	uint size = count*sizeof(ARRAYKIND);
	
	aptr = mmrealloc(aptr, asz+size);
	
	ARRAYKIND* aptrindexed = (void*)aptr+offset;
	
	// Move to the right, the block on the right
	// of the position where insertion will be done.
	bytcpy((void*)aptrindexed+size, aptrindexed, asz-offset);
	
	a->ptr = aptr;
	
	return aptrindexed;
}
