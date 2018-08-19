
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


// This function append to the ARRAYTYPE,
// space for a single ARRAYKIND and return
// a pointer to the newly appended space.
ARRAYKIND* ARRAYTYPEAPPEND1 (ARRAYTYPE* a) {
	
	ARRAYKIND* aptr = a->ptr;
	
	uint asz = aptr ? mmsz(aptr) : 0;
	
	aptr = mmrealloc(aptr, asz+sizeof(ARRAYKIND));
	
	a->ptr = aptr;
	
	return (ARRAYKIND*)((void*)aptr+asz);
}


// This function append to the ARRAYTYPE,
// space for the count (given as argument)
// of ARRAYKIND and return a pointer
// to the newly appended space.
ARRAYKIND* ARRAYTYPEAPPEND2 (ARRAYTYPE* a, uint count) {
	
	ARRAYKIND* aptr = a->ptr;
	
	uint asz = aptr ? mmsz(aptr) : 0;
	
	aptr = mmrealloc(aptr, asz+(count*sizeof(ARRAYKIND)));
	
	a->ptr = aptr;
	
	return (ARRAYKIND*)((void*)aptr+asz);
}
