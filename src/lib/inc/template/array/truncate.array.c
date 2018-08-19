
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


// This function truncate the ARRAYTYPE
// given as argument to the count
// of elements given as argument.
// Nothing is done if the argument count
// is greater than or equal to the number
// of elements in the ARRAYTYPE given as argument.
void ARRAYTYPETRUNCATE (ARRAYTYPE* a, uint count) {
	
	uint sz = count*sizeof(ARRAYKIND);
	
	ARRAYKIND* aptr = a->ptr;
	
	uint asz = aptr ? mmsz(aptr) : 0;
	
	if (sz >= asz) return;
	
	a->ptr = mmrealloc(aptr, sz);
}
