
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


// This function remove from the ARRAYTYPE
// given as argument, the elements specified
// by the arguments index and count.
// Nothing is done if the argument count
// is null or if the range specified
// by the arguments index and count fall
// outside of the ARRAYTYPE given as argument.
void ARRAYTYPEREMOVE (ARRAYTYPE* a, uint index, uint count) {
	
	if (!count) return;
	
	uint offset = index*sizeof(ARRAYKIND);
	
	uint toremovesz = count*sizeof(ARRAYKIND);
	
	ARRAYKIND* aptr = a->ptr;
	
	uint asz = aptr ? mmsz(aptr) : 0;
	
	uint offsetplustoremovesz = offset+toremovesz;
	
	if (!asz || ((offsetplustoremovesz) > asz)) return;
	
	ARRAYKIND* oldaptr = aptr;
	
	// mmrealloc() is used so that
	// the memory block get duplicated
	// if it was multi-referenced.
	aptr = mmrealloc(aptr, -1);
	
	if (aptr != oldaptr && offset) bytcpy(aptr, oldaptr, offset);
	
	uint newasz = asz-toremovesz;
	
	bytcpy((void*)aptr+offset, (void*)oldaptr+offsetplustoremovesz, newasz-offset);
	
	a->ptr = mmrealloc(aptr, newasz);
}
