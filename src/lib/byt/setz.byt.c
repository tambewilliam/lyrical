
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


// Fill with zero-valued bytes the memory location
// having the given size and pointed by ptr.
void bytsetz (void* ptr, uint size) {
	// Proceed a byte at a time until the memory
	// location in ptr is aligned to sizeof(uint).
	while (size && ((uint)ptr&(sizeof(uint)-1))) {
		*(u8*)ptr = 0;
		++ptr;
		--size;
	}
	
	// When I get here, the memory location
	// in ptr is aligned to sizeof(uint).
	
	// Take advantage of integer division to find
	// how far I can go sizeof(uint) at a time.
	void* limit1 = ptr + ((size/sizeof(uint))*sizeof(uint));
	
	void* limit2 = ptr + size;
	
	while (ptr < limit1) {
		*(uint*)ptr = 0;
		ptr += sizeof(uint);
	}
	
	// I set the rest of the data
	// going a byte at a time.
	while (ptr < limit2) {
		*(u8*)ptr = 0;
		++ptr;
	}
}
