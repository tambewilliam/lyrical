
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


// This function iterate through each element of the array,
// starting from its first element, and for each element
// of the array, ARRAYTYPEITERATECALLBACK is called,
// and a reference to element's memory location is passed,
// allowing the callback function to modify the element value.
// Iterating through the array is interrupted if the callback
// function return 0.
// The argument ARRAYTYPEITERATECALLBACK must not be null.
void ARRAYTYPEITERATE (ARRAYTYPE a, uint(* ARRAYTYPEITERATECALLBACK)(ARRAYKIND*)) {
	
	uint asz = a.ptr ? mmsz(a.ptr) : 0;
	
	if (asz) {
		
		uint i = 0;
		
		do {
			if (!ARRAYTYPEITERATECALLBACK(&a.ptr[i])) return;
		} while (++i < asz);
	}
}
