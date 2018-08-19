
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


// Copy the memory region pointed by src
// to the memory region pointed by dest,
// taking into account that they may overlap.
// If size is null, nothing is done.
void bytcpy (void* dst, void* src, uint size) {
	
	if (!size) return;
	
	void* limit1;
	void* limit2;
	
	if (dst < src) {
		
		limit2 = src + size;
		
		// Make sure src and dst are aligned to sizeof(uint) to do a uint copy.
		if (!(((uint)dst|(uint)src)&(sizeof(uint)-1))) {
			// Compute how many data can be copied sizeof(uint) at a time.
			limit1 = src + (size&-sizeof(uint));
			
			while (src < limit1) {
				*(volatile uint*)dst = *(volatile uint*)src;
				src += sizeof(uint);
				dst += sizeof(uint);
			}
		}
		
		// Copy the rest of the data sizeof(u8) at a time.
		while (src < limit2) {
			*(volatile u8*)dst = *(volatile u8*)src;
			++src;
			++dst;
		}
		
	} else if (dst > src) {
		
		limit2 = src;
		
		src += size;
		dst += size;
		
		// Make sure src and dst are aligned to sizeof(uint) to do a uint copy.
		if (!(((uint)dst|(uint)src)&(sizeof(uint)-1))) {
			// Compute how many data can be copied sizeof(uint) at a time.
			limit1 = src - (size&-sizeof(uint));
			
			while (src > limit1) {
				src -= sizeof(uint);
				dst -= sizeof(uint);
				*(volatile uint*)dst = *(volatile uint*)src;
			}
		}
		
		// Copy the rest of the data sizeof(u8) at a time.
		while (src > limit2) {
			--src;
			--dst;
			*(volatile u8*)dst = *(volatile u8*)src;
		}
	}
	
	// There is nothing to do if src == dst.
}
