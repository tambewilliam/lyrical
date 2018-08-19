
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


// This function split the ARRAYTYPE
// at the index given as argument,
// and return the bottom part of the split.
// ARRAYTYPENULL is returned if the index
// is greater than or equal to the number
// of elements in the ARRAYTYPE.
ARRAYTYPE ARRAYTYPESPLIT (ARRAYTYPE* a, uint index) {
	
	uint offset = index*sizeof(ARRAYKIND);
	
	ARRAYKIND* aptr = a->ptr;
	
	uint asz = aptr ? mmsz(aptr) : 0;
	
	if (offset >= asz) return ARRAYTYPENULL;
	
	uint retvarsz = (asz-offset);
	
	ARRAYTYPE retvar = {.ptr = mmalloc(retvarsz)};
	
	// Copy the bottom split to its new location.
	bytcpy(retvar.ptr, (void*)aptr+offset, retvarsz);
	
	// Truncate the top split.
	a->ptr = mmrealloc(aptr, offset);
	
	return retvar;
}
